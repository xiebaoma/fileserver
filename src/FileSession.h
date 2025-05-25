/**
 * @file FileSession.h
 * @brief File transfer session management
 * @author xiebaoma
 * @date 2025-05-25
 **/

#pragma once
#include "../net/ByteBuffer.h"
#include "TcpSession.h"

/**
 * @class FileSession
 * @brief Handles file upload and download operations over TCP
 *
 * Extends TcpSession to provide file transfer functionality including
 * upload and download operations with progress tracking.
 */
class FileSession : public TcpSession
{
public:
    /**
     * @brief Constructor
     * @param conn Shared pointer to the TCP connection
     * @param filebasedir Base directory for file operations
     */
    FileSession(const std::shared_ptr<TcpConnection> &conn, const char *filebasedir);

    /**
     * @brief Virtual destructor
     */
    virtual ~FileSession();

    /**
     * @brief Copy constructor (deleted)
     * @param rhs Source object
     */
    FileSession(const FileSession &rhs) = delete;

    /**
     * @brief Assignment operator (deleted)
     * @param rhs Source object
     * @return Reference to this object
     */
    FileSession &operator=(const FileSession &rhs) = delete;

    /**
     * @brief Callback for handling incoming data
     *
     * Called by multiple worker loops when data is available to read
     *
     * @param conn Shared pointer to the TCP connection
     * @param pBuffer Buffer containing the received data
     * @param receivTime Timestamp when the data was received
     */
    void onRead(const std::shared_ptr<TcpConnection> &conn, ByteBuffer *pBuffer, Timestamp receivTime);

private:
    /**
     * @brief Process received data
     *
     * Note: On 64-bit machines, size_t is 8 bytes
     *
     * @param conn Shared pointer to the TCP connection
     * @param inbuf Input buffer containing the data
     * @param length Length of the data
     * @return true if processing succeeded, false otherwise
     */
    bool process(const std::shared_ptr<TcpConnection> &conn, const char *inbuf, size_t length);

    /**
     * @brief Handle file upload response
     * @param filemd5 MD5 hash of the file
     * @param offset Current file offset
     * @param filesize Total file size
     * @param filedata File data content
     * @param conn Shared pointer to the TCP connection
     * @return true if handling succeeded, false otherwise
     */
    bool onUploadFileResponse(const std::string &filemd5, int64_t offset, int64_t filesize, const std::string &filedata, const std::shared_ptr<TcpConnection> &conn);

    /**
     * @brief Handle file download response
     * @param filemd5 MD5 hash of the file
     * @param clientNetType Network type of the client
     * @param conn Shared pointer to the TCP connection
     * @return true if handling succeeded, false otherwise
     */
    bool onDownloadFileResponse(const std::string &filemd5, int32_t clientNetType, const std::shared_ptr<TcpConnection> &conn);

    /**
     * @brief Reset file state
     *
     * Cleans up file resources and resets state variables
     */
    void resetFile();

private:
    int32_t m_id;  /**< Session ID */
    int32_t m_seq; /**< Current session data packet sequence number */

    // Current file information
    FILE *m_fp{};                          /**< File pointer for current operation */
    int64_t m_currentDownloadFileOffset{}; /**< Current offset in the file being downloaded */
    int64_t m_currentDownloadFileSize{};   /**< Size of the file being downloaded (should be reset to 0 after completion) */
    std::string m_strFileBaseDir;          /**< Base directory for file operations */
    bool m_bFileUploading;                 /**< Flag indicating whether a file is currently being uploaded */
};