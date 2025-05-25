/**
 * @file FileServer.cpp
 * @brief Main class implementation for the file transfer server.
 * @author xiebaoma
 * @date 2025-05-25
 */

#include "FileServer.h"
#include "../net/InetAddress.h"
#include "../base/AsyncLog.h"
#include "../base/Singleton.h"
#include "FileSession.h"

/**
 * @brief Initializes the file server with IP, port, and base file directory.
 *
 * @param ip            IP address to bind.
 * @param port          Port number to listen on.
 * @param loop          Event loop instance to handle IO.
 * @param fileBaseDir   Optional base directory for file storage.
 * @return true         Initialization successful.
 * @return false        Initialization failed.
 */
bool FileServer::init(const char *ip, short port, EventLoop *loop, const char *fileBaseDir /* = "filecache/" */)
{
    m_strFileBaseDir = fileBaseDir;

    InetAddress addr(ip, port);
    m_server = std::make_unique<TcpServer>(loop, addr, "MYFileServer", TcpServer::kReusePort);
    m_server->setConnectionCallback(std::bind(&FileServer::onConnected, this, std::placeholders::_1));

    // Start listening with 6 threads
    m_server->start(6);
    return true;
}

/**
 * @brief Shuts down the server and releases resources.
 */
void FileServer::uninit()
{
    if (m_server)
    {
        m_server->stop();
    }
}

/**
 * @brief Called when a new client connection is established or closed.
 *
 * @param conn Shared pointer to the TcpConnection.
 */
void FileServer::onConnected(std::shared_ptr<TcpConnection> conn)
{
    if (conn->connected())
    {
        LOGI("Client connected: %s", conn->peerAddress().toIpPort().c_str());

        // Create new file session for the connection
        auto session = std::make_shared<FileSession>(conn, m_strFileBaseDir.c_str());
        conn->setMessageCallback(std::bind(&FileSession::onRead, session.get(),
                                           std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        // Store the session safely
        std::lock_guard<std::mutex> guard(m_sessionMutex);
        m_sessions.push_back(session);
    }
    else
    {
        onDisconnected(conn);
    }
}

/**
 * @brief Handles logic when a client disconnects.
 *        Removes the corresponding session from the session list.
 *
 * @param conn The connection that was closed.
 */
void FileServer::onDisconnected(const std::shared_ptr<TcpConnection> &conn)
{
    std::lock_guard<std::mutex> guard(m_sessionMutex);

    auto it = std::find_if(m_sessions.begin(), m_sessions.end(),
                           [&conn](const std::shared_ptr<FileSession> &session)
                           {
                               return session->getConnectionPtr() == conn;
                           });

    if (it != m_sessions.end())
    {
        LOGI("Client disconnected: %s", conn->peerAddress().toIpPort().c_str());
        m_sessions.erase(it);
    }
}
