/**
 * @file TcpSession.h
 * @brief TCP session management class
 * @author xiebaoma
 * @date 2025-05-25
 **/

#pragma once

#include <memory>
#include "../net/TcpConnection.h"

using namespace net;

// To separate business logic from implementation, a subclass should inherit from TcpSession.
// TcpSession should contain only logical code, while its subclass contains business code.
/**
 * @class TcpSession
 * @brief Manages a TCP connection session
 */
class TcpSession
{
public:
    /**
     * @brief Constructor
     * @param tmpconn Weak pointer to the TCP connection
     */
    TcpSession(const std::weak_ptr<TcpConnection> &tmpconn);

    /**
     * @brief Destructor
     */
    ~TcpSession();

    /**
     * @brief Copy constructor (deleted)
     */
    TcpSession(const TcpSession &rhs) = delete;

    /**
     * @brief Assignment operator (deleted)
     */
    TcpSession &operator=(const TcpSession &rhs) = delete;

    /**
     * @brief Get the shared pointer to the TCP connection
     * @return Shared pointer to the TCP connection, or NULL if expired
     */
    std::shared_ptr<TcpConnection> getConnectionPtr()
    {
        if (tmpConn_.expired())
            return NULL;

        return tmpConn_.lock();
    }

    /**
     * @brief Send file data to the client
     * @param cmd Command type
     * @param seq Sequence number
     * @param errorcode Error code
     * @param filemd5 MD5 hash of the file
     * @param offset File offset position
     * @param filesize Total file size
     * @param filedata File data content
     */
    void send(int32_t cmd, int32_t seq, int32_t errorcode, const std::string &filemd5, int64_t offset, int64_t filesize, const std::string &filedata);

private:
    /**
     * @brief Send a data package
     * @param body Pointer to the data body
     * @param bodylength Length of the data body (supports large files using int64_t)
     */
    void sendPackage(const char *body, int64_t bodylength);

protected:
    // TcpSession must use a weak pointer to reference TcpConnection because TcpConnection
    // may self-destruct due to network errors, in which case TcpSession should also be destroyed
    std::weak_ptr<TcpConnection> tmpConn_; /**< Weak pointer to the TCP connection */
};
