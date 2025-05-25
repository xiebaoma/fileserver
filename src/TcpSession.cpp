/**
 * @file TcpSession.cpp
 * @brief Implementation of TCP session management
 * @author xiebaoma
 * @date 2025-05-25
 **/
#include "TcpSession.h"
#include "../base/AsyncLog.h"
#include "../net/ProtocolStream.h"
#include "FileMsg.h"

/**
 * @brief Constructor for TcpSession
 * @param tmpconn Weak pointer to the TCP connection
 */
TcpSession::TcpSession(const std::weak_ptr<TcpConnection> &tmpconn) : tmpConn_(tmpconn)
{
}

/**
 * @brief Destructor for TcpSession
 */
TcpSession::~TcpSession()
{
}

/**
 * @brief Send file data to the client
 * 
 * Serializes command, sequence number, error code, file metadata and content
 * into a binary stream and sends it through the TCP connection.
 * 
 * @param cmd Command type
 * @param seq Sequence number
 * @param errorcode Error code
 * @param filemd5 MD5 hash of the file
 * @param offset File offset position
 * @param filesize Total file size
 * @param filedata File data content
 */
void TcpSession::send(int32_t cmd, int32_t seq, int32_t errorcode,
                      const std::string &filemd5, int64_t offset,
                      int64_t filesize, const std::string &filedata)
{
    try
    {
        std::string outbuf;
        net::BinaryStreamWriter writeStream(&outbuf);

        writeStream.WriteInt32(cmd);
        writeStream.WriteInt32(seq);
        writeStream.WriteInt32(errorcode);
        writeStream.WriteString(filemd5);
        writeStream.WriteInt64(offset);
        writeStream.WriteInt64(filesize);
        writeStream.WriteString(filedata);

        writeStream.Flush();
        sendPackage(outbuf.c_str(), static_cast<int64_t>(outbuf.length()));
    }
    catch (const std::exception &ex)
    {
        LOGE("TcpSession::send - Exception during serialization: %s", ex.what());
    }
}

/**
 * @brief Send a data package through the TCP connection
 * 
 * Prepends a header with the body length to the data and sends the complete
 * package through the TCP connection. Handles connection validity checks and
 * error reporting.
 * 
 * @param body Pointer to the data body
 * @param bodylength Length of the data body
 */
void TcpSession::sendPackage(const char *body, int64_t bodylength)
{
    if (!body || bodylength <= 0)
    {
        LOGE("TcpSession::sendPackage - Invalid body or length.");
        return;
    }

    std::string strPackageData;
    file_msg_header header = {bodylength};

    strPackageData.reserve(sizeof(header) + bodylength); // Avoid multiple realloc operations
    strPackageData.append(reinterpret_cast<const char *>(&header), sizeof(header));
    strPackageData.append(body, bodylength);

    std::shared_ptr<TcpConnection> conn = tmpConn_.lock();
    if (!conn)
    {
        LOGE("TcpSession::sendPackage - TcpConnection expired. Session may be leaked.");
        return;
    }

    LOGI("Sending data: total package length = %zu, body length = %lld",
         strPackageData.length(), bodylength);
    // Optional: Dump binary data in debug mode
    // LOG_DEBUG_BIN(reinterpret_cast<const unsigned char*>(strPackageData.data()), strPackageData.length());

    try
    {
        conn->send(strPackageData.c_str(), static_cast<int64_t>(strPackageData.length()));
    }
    catch (const std::exception &ex)
    {
        LOGE("TcpSession::sendPackage - Exception during send: %s", ex.what());
    }
}
