#include "TcpConnection.h"

#include <functional>
#include <thread>
#include <sstream>
#include <errno.h>
#include "../base/Platform.h"
#include "../base/AsyncLog.h"
#include "Sockets.h"
#include "EventLoop.h"
#include "Channel.h"

using namespace net;

void net::defaultConnectionCallback(const TcpConnectionPtr &conn)
{
    LOGD("%s -> is %s",
         conn->localAddress().toIpPort().c_str(),
         conn->peerAddress().toIpPort().c_str(),
         (conn->connected() ? "UP" : "DOWN"));
    // do not call conn->forceClose(), because some users want to register message callback only.
}

void net::defaultMessageCallback(const TcpConnectionPtr &, ByteBuffer *buf, Timestamp)
{
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop *loop, const string &nameArg, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr)
    : m_loop(loop),
      m_name(nameArg),
      m_state(kConnecting),
      m_socket(new Socket(sockfd)),
      m_channel(new Channel(loop, sockfd)),
      m_localAddr(localAddr),
      m_peerAddr(peerAddr),
      m_highWaterMark(64 * 1024 * 1024)
{
    m_channel->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    m_channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    m_channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    m_channel->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOGD("TcpConnection::ctor[%s] at 0x%x fd=%d", m_name.c_str(), this, sockfd);
    m_socket->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOGD("TcpConnection::dtor[%s] at 0x%x fd=%d state=%s",
         m_name.c_str(), this, m_channel->fd(), stateToString());
    // assert(state_ == kDisconnected);
}

void TcpConnection::send(const void *data, int len)
{
    if (m_state == kConnected)
    {
        if (m_loop->isInLoopThread())
        {
            sendInLoop(data, len);
        }
        else
        {
            string message(static_cast<const char *>(data), len);
            m_loop->runInLoop(
                std::bind(static_cast<void (TcpConnection::*)(const string &)>(&TcpConnection::sendInLoop),
                          this, // FIXME
                          message));
        }
    }
}

void TcpConnection::send(const string &message)
{
    if (m_state == kConnected)
    {
        if (m_loop->isInLoopThread())
        {
            sendInLoop(message);
        }
        else
        {
            m_loop->runInLoop(
                std::bind(static_cast<void (TcpConnection::*)(const string &)>(&TcpConnection::sendInLoop),
                          this, // FIXME
                          message));
            // std::forward<string>(message)));
        }
    }
}

// FIXME efficiency!!!
void TcpConnection::send(ByteBuffer *buf)
{
    if (m_state == kConnected)
    {
        if (m_loop->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else
        {
            m_loop->runInLoop(
                std::bind(static_cast<void (TcpConnection::*)(const string &)>(&TcpConnection::sendInLoop),
                          this, // FIXME
                          buf->retrieveAllAsString()));
            // std::forward<string>(message)));
        }
    }
}

void TcpConnection::sendInLoop(const string &message)
{
    sendInLoop(message.c_str(), message.size());
}

/**
 * @brief Send data in the event loop thread. Called internally by TcpConnection::send.
 *
 * This function attempts to write the data directly to the socket if:
 *   1. The connection is still active.
 *   2. There is no pending data in the output buffer.
 *   3. The channel is not currently monitoring for write events.
 *
 * If not all data can be written immediately (e.g., socket buffer is full),
 * the remaining data will be appended to the output buffer, and the channel
 * will start monitoring for EPOLLOUT events to complete the write asynchronously.
 *
 * This function must be called in the IO thread that owns the connection.
 *
 * @param data Pointer to the data to send.
 * @param len  Length of the data in bytes.
 */
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    m_loop->assertInLoopThread(); // Ensure this runs in the loop thread

    int32_t nwrote = 0;      // Number of bytes actually written to the socket
    size_t remaining = len;  // Bytes left to write
    bool faultError = false; // Indicates whether a fatal socket error occurred

    // If the connection has been closed, discard the write request
    if (m_state == kDisconnected)
    {
        LOGW("disconnected, give up writing");
        return;
    }

    // Attempt direct write if output buffer is empty and not already writing
    if (!m_channel->isWriting() && m_outputBuffer.readableBytes() == 0)
    {
        nwrote = sockets::write(m_channel->fd(), data, len);

        if (nwrote >= 0)
        {
            remaining = len - nwrote;

            // If all data was sent immediately and a write complete callback is set,
            // queue the callback to be executed in the loop
            if (remaining == 0 && m_writeCompleteCallback)
            {
                m_loop->queueInLoop(std::bind(m_writeCompleteCallback, shared_from_this()));
            }
        }
        else // Write failed
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK) // Not just a temporary full buffer
            {
                LOGSYSE("TcpConnection::sendInLoop");

                // Check for common connection-related errors
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    // Safety check: remaining should not be greater than len
    if (remaining > len)
        return;

    // If there's still data left and the socket is okay, buffer it
    if (!faultError && remaining > 0)
    {
        size_t oldLen = m_outputBuffer.readableBytes();

        // Trigger high water mark callback if threshold is crossed
        if (oldLen + remaining >= m_highWaterMark &&
            oldLen < m_highWaterMark &&
            m_highWaterMarkCallback)
        {
            m_loop->queueInLoop(
                std::bind(m_highWaterMarkCallback, shared_from_this(), oldLen + remaining));
        }

        // Append remaining data to output buffer
        m_outputBuffer.append(static_cast<const char *>(data) + nwrote, remaining);

        // Ensure EPOLLOUT is enabled so we can continue sending when socket becomes writable
        if (!m_channel->isWriting())
        {
            m_channel->enableWriting();
        }
    }
}

void TcpConnection::shutdown()
{
    // FIXME: use compare and swap
    if (m_state == kConnected)
    {
        setState(kDisconnecting);
        // FIXME: shared_from_this()?
        m_loop->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    m_loop->assertInLoopThread();
    if (!m_channel->isWriting())
    {
        // we are not writing
        m_socket->shutdownWrite();
    }
}

// void TcpConnection::shutdownAndForceCloseAfter(double seconds)
// {
//   // FIXME: use compare and swap
//   if (state_ == kConnected)
//   {
//     setState(kDisconnecting);
//     loop_->runInLoop(boost::bind(&TcpConnection::shutdownAndForceCloseInLoop, this, seconds));
//   }
// }

// void TcpConnection::shutdownAndForceCloseInLoop(double seconds)
// {
//   loop_->assertInLoopThread();
//   if (!channel_->isWriting())
//   {
//     // we are not writing
//     socket_->shutdownWrite();
//   }
//   loop_->runAfter(
//       seconds,
//       makeWeakCallback(shared_from_this(),
//                        &TcpConnection::forceCloseInLoop));
// }

void TcpConnection::forceClose()
{
    // FIXME: use compare and swap
    if (m_state == kConnected || m_state == kDisconnecting)
    {
        setState(kDisconnecting);
        m_loop->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseInLoop()
{
    m_loop->assertInLoopThread();
    if (m_state == kConnected || m_state == kDisconnecting)
    {
        // as if we received 0 byte in handleRead();
        handleClose();
    }
}

const char *TcpConnection::stateToString() const
{
    switch (m_state)
    {
    case kDisconnected:
        return "kDisconnected";
    case kConnecting:
        return "kConnecting";
    case kConnected:
        return "kConnected";
    case kDisconnecting:
        return "kDisconnecting";
    default:
        return "unknown state";
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    m_socket->setTcpNoDelay(on);
}

void TcpConnection::connectEstablished()
{
    m_loop->assertInLoopThread();
    if (m_state != kConnecting)
    {
        // 一定不能走这个分支
        return;
    }

    setState(kConnected);

    // 假如正在执行这行代码时，对端关闭了连接
    if (!m_channel->enableReading())
    {
        LOGE("enableReading failed.");
        // setState(kDisconnected);
        handleClose();
        return;
    }

    // connectionCallback_指向void XXServer::OnConnection(const std::shared_ptr<TcpConnection>& conn)
    m_connectionCallback(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    m_loop->assertInLoopThread();
    if (m_state == kConnected)
    {
        setState(kDisconnected);
        m_channel->disableAll();

        m_connectionCallback(shared_from_this());
    }
    m_channel->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    m_loop->assertInLoopThread();
    int savedErrno = 0;
    int32_t n = m_inputBuffer.readFd(m_channel->fd(), &savedErrno);
    if (n > 0)
    {
        // messageCallback_指向CTcpSession::OnRead(const std::shared_ptr<TcpConnection>& conn, Buffer* pBuffer, Timestamp receiveTime)
        m_messageCallback(shared_from_this(), &m_inputBuffer, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOGSYSE("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    m_loop->assertInLoopThread();
    if (m_channel->isWriting())
    {
        int32_t n = sockets::write(m_channel->fd(), m_outputBuffer.peek(), m_outputBuffer.readableBytes());
        if (n > 0)
        {
            m_outputBuffer.retrieve(n);
            if (m_outputBuffer.readableBytes() == 0)
            {
                m_channel->disableWriting();
                if (m_writeCompleteCallback)
                {
                    m_loop->queueInLoop(std::bind(m_writeCompleteCallback, shared_from_this()));
                }
                if (m_state == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOGSYSE("TcpConnection::handleWrite");
            // if (state_ == kDisconnecting)
            // {
            //   shutdownInLoop();
            // }
            // added by zhangyl 2019.05.06
            handleClose();
        }
    }
    else
    {
        LOGD("Connection fd = %d  is down, no more writing", m_channel->fd());
    }
}

void TcpConnection::handleClose()
{
    // 在Linux上当一个链接出了问题，会同时触发handleError和handleClose
    // 为了避免重复关闭链接，这里判断下当前状态
    // 已经关闭了，直接返回
    if (m_state == kDisconnected)
        return;

    m_loop->assertInLoopThread();
    LOGD("fd = %d  state = %s", m_channel->fd(), stateToString());
    // assert(state_ == kConnected || state_ == kDisconnecting);
    //  we don't close fd, leave it to dtor, so we can find leaks easily.
    setState(kDisconnected);
    m_channel->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    m_connectionCallback(guardThis);
    // must be the last line
    m_closeCallback(guardThis);

    // 只处理业务上的关闭，真正的socket fd在TcpConnection析构函数中关闭
    // if (socket_)
    //{
    //     sockets::close(socket_->fd());
    // }
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(m_channel->fd());
    LOGE("TcpConnection::%s handleError [%d] - SO_ERROR = %s", m_name.c_str(), err, strerror(err));

    // 调用handleClose()关闭连接，回收Channel和fd
    handleClose();
}
