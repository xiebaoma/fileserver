/*
 * Author: xiebaoma
 * Date: 2025-06-04
 *
 * Description:
 * This header defines the Socket class and a collection of utility functions
 * in the sockets namespace for managing TCP sockets in a cross-platform way.
 */

#pragma once

#include <stdint.h>
#include "../base/Platform.h"

// Forward declaration of tcp_info (defined in <netinet/tcp.h>)
struct tcp_info;

namespace net
{
    class InetAddress;

    /**
     * @brief A wrapper class for a TCP socket file descriptor.
     *
     * Provides RAII-style management and utility methods to manipulate
     * socket options, perform bind/listen/accept, and configure flags.
     */
    class Socket
    {
    public:
        /**
         * @brief Constructs the socket wrapper with a file descriptor.
         * @param sockfd The socket file descriptor.
         */
        explicit Socket(int sockfd);

        /**
         * @brief Destructor: closes the socket file descriptor.
         */
        ~Socket();

        /**
         * @brief Returns the underlying socket file descriptor.
         */
        SOCKET fd() const { return m_sockfd; }

        /**
         * @brief Binds the socket to a local address.
         */
        void bindAddress(const InetAddress &localaddr);

        /**
         * @brief Marks the socket as listening for incoming connections.
         */
        void listen();

        /**
         * @brief Accepts a new incoming connection.
         * @param peeraddr Output parameter storing the remote peer address.
         * @return The new connected socket file descriptor.
         */
        int accept(InetAddress *peeraddr);

        /**
         * @brief Shuts down the write half of the socket.
         */
        void shutdownWrite();

        /**
         * @brief Enables or disables TCP_NODELAY (Nagle’s algorithm).
         */
        void setTcpNoDelay(bool on);

        /**
         * @brief Enables or disables SO_REUSEADDR.
         */
        void setReuseAddr(bool on);

        /**
         * @brief Enables or disables SO_REUSEPORT.
         */
        void setReusePort(bool on);

        /**
         * @brief Enables or disables SO_KEEPALIVE.
         */
        void setKeepAlive(bool on);

    private:
        const SOCKET m_sockfd; ///< Underlying socket file descriptor
    };

    /**
     * @brief Namespace containing low-level socket utility functions.
     */
    namespace sockets
    {
        SOCKET createOrDie();
        SOCKET createNonblockingOrDie();

        void setNonBlockAndCloseOnExec(SOCKET sockfd);

        void setReuseAddr(SOCKET sockfd, bool on);
        void setReusePort(SOCKET sockfd, bool on);

        /**
         * @brief Connects the given socket to the specified address.
         */
        SOCKET connect(SOCKET sockfd, const struct sockaddr_in &addr);

        /**
         * @brief Binds the socket to an address or terminates the process on failure.
         */
        void bindOrDie(SOCKET sockfd, const struct sockaddr_in &addr);

        /**
         * @brief Marks the socket as a passive socket, or exits on error.
         */
        void listenOrDie(SOCKET sockfd);

        /**
         * @brief Accepts a connection on a listening socket.
         */
        SOCKET accept(SOCKET sockfd, struct sockaddr_in *addr);

        /**
         * @brief Reads data from the socket into a buffer.
         */
        int32_t read(SOCKET sockfd, void *buf, int32_t count);

#ifndef WIN32
        /**
         * @brief Performs a vectorized read (readv) on the socket.
         */
        ssize_t readv(SOCKET sockfd, const struct iovec *iov, int iovcnt);
#endif

        /**
         * @brief Writes data to the socket from a buffer.
         */
        int32_t write(SOCKET sockfd, const void *buf, int32_t count);

        /**
         * @brief Closes the socket.
         */
        void close(SOCKET sockfd);

        /**
         * @brief Shuts down the write half of the socket.
         */
        void shutdownWrite(SOCKET sockfd);

        /**
         * @brief Converts a sockaddr_in to a human-readable "ip:port" string.
         */
        void toIpPort(char *buf, size_t size, const struct sockaddr_in &addr);

        /**
         * @brief Converts a sockaddr_in to a human-readable IP string.
         */
        void toIp(char *buf, size_t size, const struct sockaddr_in &addr);

        /**
         * @brief Populates a sockaddr_in from IP string and port.
         */
        void fromIpPort(const char *ip, uint16_t port, struct sockaddr_in *addr);

        /**
         * @brief Returns the last socket error.
         */
        int getSocketError(SOCKET sockfd);

        // Utility functions for safe casting between sockaddr types.
        const struct sockaddr *sockaddr_cast(const struct sockaddr_in *addr);
        struct sockaddr *sockaddr_cast(struct sockaddr_in *addr);
        const struct sockaddr_in *sockaddr_in_cast(const struct sockaddr *addr);
        struct sockaddr_in *sockaddr_in_cast(struct sockaddr *addr);

        /**
         * @brief Returns the local address of the given socket.
         */
        struct sockaddr_in getLocalAddr(SOCKET sockfd);

        /**
         * @brief Returns the peer (remote) address of the given socket.
         */
        struct sockaddr_in getPeerAddr(SOCKET sockfd);

        /**
         * @brief Checks whether the socket is connected to itself.
         */
        bool isSelfConnect(SOCKET sockfd);
    }
}
