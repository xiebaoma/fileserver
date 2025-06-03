/*
 * Author: xiebaoma
 * Date: 2025-06-03
 * Description: TcpServer is a non-copyable, reference-counted object designed to be used with shared_ptr.
 * This class manages the connection lifecycle, IO events, read/write buffers,
 * and application-level callbacks.
 */

#pragma once

#include <memory>

#include "Callbacks.h"
#include "ByteBuffer.h"
#include "InetAddress.h"

// Forward declaration for TCP connection information struct from <netinet/tcp.h>
struct tcp_info;

namespace net
{
    class EventLoop;
    class Channel;
    class Socket;

    /**
     * @brief TcpConnection represents a single TCP connection (client or server side).
     *
     * It is a non-copyable, reference-counted object designed to be used with shared_ptr.
     * This class manages the connection lifecycle, IO events, read/write buffers,
     * and application-level callbacks.
     */
    class TcpConnection : public std::enable_shared_from_this<TcpConnection>
    {
    public:
        /**
         * @brief Constructs a TcpConnection with all relevant information.
         *
         * @param loop       The EventLoop this connection belongs to.
         * @param name       A unique name for the connection (usually conn_id).
         * @param sockfd     The socket file descriptor.
         * @param localAddr  Local address bound to the socket.
         * @param peerAddr   Remote peer address of the connection.
         */
        TcpConnection(EventLoop *loop,
                      const std::string &name,
                      int sockfd,
                      const InetAddress &localAddr,
                      const InetAddress &peerAddr);
        ~TcpConnection();

        // Accessors
        EventLoop *getLoop() const { return m_loop; }
        const std::string &name() const { return m_name; }
        const InetAddress &localAddress() const { return m_localAddr; }
        const InetAddress &peerAddress() const { return m_peerAddr; }
        bool connected() const { return m_state == kConnected; }

        // Send data over the connection (thread-safe).
        void send(const void *message, int len);
        void send(const std::string &message);
        void send(ByteBuffer *message); // Efficient send via buffer swap.

        // Initiates a graceful shutdown (write then close).
        void shutdown();

        // Forces the connection to close immediately.
        void forceClose();

        // Enables/disables the TCP_NODELAY option.
        void setTcpNoDelay(bool on);

        // Set user-defined callbacks for various connection events.
        void setConnectionCallback(const ConnectionCallback &cb)
        {
            m_connectionCallback = cb;
        }

        void setMessageCallback(const MessageCallback &cb)
        {
            m_messageCallback = cb;
        }

        /**
         * @brief Sets a callback to be called when all data is written to socket.
         */
        void setWriteCompleteCallback(const WriteCompleteCallback &cb)
        {
            m_writeCompleteCallback = cb;
        }

        /**
         * @brief Sets a high water mark callback and threshold.
         *
         * This is useful for flow control when too much data is buffered.
         */
        void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
        {
            m_highWaterMarkCallback = cb;
            m_highWaterMark = highWaterMark;
        }

        // Accessors to internal input/output buffers.
        ByteBuffer *inputBuffer() { return &m_inputBuffer; }
        ByteBuffer *outputBuffer() { return &m_outputBuffer; }

        // Internal use only: set when connection is to be closed.
        void setCloseCallback(const CloseCallback &cb)
        {
            m_closeCallback = cb;
        }

        // Called once when connection is fully established (after accept/connect).
        void connectEstablished();

        // Called once when connection is to be destroyed.
        void connectDestroyed();

    private:
        // Connection state
        enum StateE
        {
            kDisconnected,
            kConnecting,
            kConnected,
            kDisconnecting
        };

        // Event handlers bound to underlying Channel
        void handleRead(Timestamp receiveTime);
        void handleWrite();
        void handleClose();
        void handleError();

        // Internal send helpers (executed in loop thread).
        void sendInLoop(const std::string &message);
        void sendInLoop(const void *message, size_t len);

        // Internal shutdown/close helpers
        void shutdownInLoop();
        void forceCloseInLoop();

        // Set the internal state
        void setState(StateE s) { m_state = s; }

        // Return string representation of current state (for debugging)
        const char *stateToString() const;

    private:
        EventLoop *m_loop;                             ///< The EventLoop this connection belongs to.
        const std::string m_name;                      ///< Unique name for this connection.
        StateE m_state;                                ///< Connection state.
        std::unique_ptr<Socket> m_socket;              ///< Managed socket object.
        std::unique_ptr<Channel> m_channel;            ///< Channel for IO events.
        const InetAddress m_localAddr;                 ///< Local socket address.
        const InetAddress m_peerAddr;                  ///< Remote peer address.
        ConnectionCallback m_connectionCallback;       ///< Callback when connection is established/closed.
        MessageCallback m_messageCallback;             ///< Callback when a message is received.
        WriteCompleteCallback m_writeCompleteCallback; ///< Callback when write completes.
        HighWaterMarkCallback m_highWaterMarkCallback; ///< Callback when output buffer exceeds high watermark.
        CloseCallback m_closeCallback;                 ///< Internal close callback.
        size_t m_highWaterMark;                        ///< Threshold for high water mark callback.
        ByteBuffer m_inputBuffer;                      ///< Input buffer (read data).
        ByteBuffer m_outputBuffer;                     ///< Output buffer (pending writes).
    };

    // Alias for shared pointer to TcpConnection
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

} // namespace net
