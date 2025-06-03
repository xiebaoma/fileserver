/*
 * Author: xiebaoma
 * Date: 2025-06-03
 * Description: TcpServer is a high-level abstraction for a TCP server.
 * It manages incoming connections, dispatches them to worker threads,
 * and provides callbacks for connection, message handling, and completion events.
 */

#pragma once

#include <atomic>
#include <map>
#include <memory>

#include "TcpConnection.h"

namespace net
{
    class Acceptor;
    class EventLoop;
    class EventLoopThreadPool;

    /**
     * @brief TcpServer is a multi-threaded, event-driven TCP server.
     *
     * It supports configurable thread pools and customizable callbacks for
     * connection lifecycle, message reception, and write completion.
     */
    class TcpServer
    {
    public:
        using ThreadInitCallback = std::function<void(EventLoop *)>;

        /**
         * @brief Socket option for reusing ports.
         */
        enum Option
        {
            kNoReusePort, ///< Do not reuse port.
            kReusePort    ///< Reuse port (default).
        };

        /**
         * @brief Constructs a TcpServer object.
         *
         * @param loop        The main event loop (acceptor runs in this loop).
         * @param listenAddr  Address to listen for incoming connections.
         * @param nameArg     A human-readable name for this server.
         * @param option      Port reuse option (default: kReusePort).
         */
        TcpServer(EventLoop *loop,
                  const InetAddress &listenAddr,
                  const std::string &nameArg,
                  Option option = kReusePort);

        ~TcpServer();

        /**
         * @brief Returns the host:port string.
         */
        const std::string &hostport() const { return m_hostport; }

        /**
         * @brief Returns the server's name.
         */
        const std::string &name() const { return m_name; }

        /**
         * @brief Returns the main EventLoop.
         */
        EventLoop *getLoop() const { return m_loop; }

        /**
         * @brief Sets the thread initialization callback.
         *
         * This callback is called in each worker thread before starting the loop.
         */
        void setThreadInitCallback(const ThreadInitCallback &cb)
        {
            m_threadInitCallback = cb;
        }

        /**
         * @brief Starts the server and worker thread pool.
         *
         * @param workerThreadCount Number of threads in the thread pool.
         */
        void start(int workerThreadCount = 4);

        /**
         * @brief Stops the server and closes all connections.
         */
        void stop();

        /**
         * @brief Sets the connection lifecycle callback.
         *
         * Called when a new connection is established or destroyed.
         * Not thread-safe: should be called before `start()`.
         */
        void setConnectionCallback(const ConnectionCallback &cb)
        {
            m_connectionCallback = cb;
        }

        /**
         * @brief Sets the message reception callback.
         *
         * Called when data is received on a connection.
         * Not thread-safe: should be called before `start()`.
         */
        void setMessageCallback(const MessageCallback &cb)
        {
            m_messageCallback = cb;
        }

        /**
         * @brief Sets the write-completion callback.
         *
         * Called when a pending write operation is completed.
         * Not thread-safe: should be called before `start()`.
         */
        void setWriteCompleteCallback(const WriteCompleteCallback &cb)
        {
            m_writeCompleteCallback = cb;
        }

        /**
         * @brief Removes a connection from the server (thread-safe).
         *
         * Should be used to explicitly close and clean up a connection.
         */
        void removeConnection(const TcpConnectionPtr &conn);

    private:
        /**
         * @brief Called in the main loop when a new connection is accepted.
         *
         * Creates a TcpConnection object and assigns it to a worker thread.
         *
         * @param sockfd    The socket file descriptor for the new connection.
         * @param peerAddr  The remote peer address.
         */
        void newConnection(int sockfd, const InetAddress &peerAddr);

        /**
         * @brief Removes a connection from the internal map.
         *
         * Must be called in the owning loop thread.
         */
        void removeConnectionInLoop(const TcpConnectionPtr &conn);

        using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

    private:
        EventLoop *m_loop;                                          ///< The main event loop (acceptor runs here).
        const std::string m_hostport;                               ///< The listening address (host:port).
        const std::string m_name;                                   ///< Server instance name.
        std::unique_ptr<Acceptor> m_acceptor;                       ///< Responsible for accepting new connections.
        std::unique_ptr<EventLoopThreadPool> m_eventLoopThreadPool; ///< Worker thread pool.
        ConnectionCallback m_connectionCallback;                    ///< Callback on connection established/closed.
        MessageCallback m_messageCallback;                          ///< Callback on message received.
        WriteCompleteCallback m_writeCompleteCallback;              ///< Callback on write completion.
        ThreadInitCallback m_threadInitCallback;                    ///< Callback before thread loop starts.
        std::atomic<int> m_started;                                 ///< Atomic flag indicating server start state.
        int m_nextConnId;                                           ///< Next connection ID used to generate unique names.
        ConnectionMap m_connections;                                ///< Active TCP connections.
    };

} // namespace net
