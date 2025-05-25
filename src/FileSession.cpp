/**
 * @file FileSession.cpp
 * @brief Implementation of file transfer session management
 * @author xiebaoma
 * @date 2025-05-25
 **/
#include "FileSession.h"
#include <string.h>
#include <sstream>
#include <list>
#include "../net/TcpConnection.h"
#include "../net/ProtocolStream.h"
#include "../base/AsyncLog.h"
#include "../base/Singleton.h"
#include "FileMsg.h"
#include "FileManager.h"

using namespace net;

/**
 * @brief Maximum package size for file server (50MB)
 */
#define MAX_PACKAGE_SIZE 50 * 1024 * 1024

/**
 * @brief Constructor for FileSession
 * @param conn Shared pointer to the TCP connection
 * @param filebasedir Base directory for file operations
 */
FileSession::FileSession(const std::shared_ptr<TcpConnection> &conn, const char *filebasedir) : TcpSession(conn),
                                                                                                m_id(0),
                                                                                                m_seq(0),
                                                                                                m_strFileBaseDir(filebasedir),
                                                                                                m_bFileUploading(false)
{
}

/**
 * @brief Destructor for FileSession
 */
FileSession::~FileSession()
{
}

/**
 * @brief Callback function triggered when data is received on the TCP connection.
 *        This function extracts complete application-level packets from the ByteBuffer,
 *        validates them, and delegates them for processing.
 *
 * @param conn         Shared pointer to the TcpConnection that received data.
 * @param pBuffer      Pointer to the ByteBuffer containing the received raw bytes.
 * @param receivTime   Timestamp when the data was received (can be used for timeout/lag detection).
 */
void FileSession::onRead(const std::shared_ptr<TcpConnection> &conn, ByteBuffer *pBuffer, Timestamp receivTime)
{
    while (true)
    {
        // Step 1: Check if buffer contains enough data for a full header
        if (pBuffer->readableBytes() < (size_t)sizeof(file_msg_header))
        {
            // Incomplete header, wait for more data to arrive
            return;
        }

        // Step 2: Peek and decode the message header without removing it from the buffer
        file_msg_header header;
        memcpy(&header, pBuffer->peek(), sizeof(file_msg_header));

        // Step 3: Validate the header - check for illegal or oversized package
        if (header.packagesize <= 0 || header.packagesize > MAX_PACKAGE_SIZE)
        {
            // Received an invalid or potentially malicious packet header
            LOGE("Illegal package header size: %lld, close TcpConnection, client: %s",
                 header.packagesize, conn->peerAddress().toIpPort().c_str());
            LOG_DEBUG_BIN((unsigned char *)&header, sizeof(header));
            conn->forceClose(); // Immediately close the connection
            return;
        }

        // Step 4: Check if buffer contains the full package (header + body)
        if (pBuffer->readableBytes() < (size_t)header.packagesize + sizeof(file_msg_header))
        {
            // Not enough data yet for a full package
            return;
        }

        // Step 5: Extract and process the package
        pBuffer->retrieve(sizeof(file_msg_header)); // Discard the header bytes
        std::string inbuf;
        inbuf.append(pBuffer->peek(), header.packagesize); // Copy the package body
        pBuffer->retrieve(header.packagesize);             // Remove the package body from the buffer

        // Step 6: Delegate the message to the business logic processor
        if (!process(conn, inbuf.c_str(), inbuf.length()))
        {
            // If processing failed, consider the connection compromised or invalid
            LOGE("Process error, close TcpConnection, client: %s",
                 conn->peerAddress().toIpPort().c_str());
            conn->forceClose();
        }

        // Loop continues to check if more complete packages are available in the buffer
    }
}

/**
 * @brief Process a complete data packet
 *
 * Deserializes the packet data and routes to the appropriate handler based on command type.
 *
 * @param conn Shared pointer to the TCP connection
 * @param inbuf Input buffer containing the data
 * @param length Length of the data
 * @return true if processing succeeded, false otherwise
 */
bool FileSession::process(const std::shared_ptr<TcpConnection> &conn, const char *inbuf, size_t length)
{
    BinaryStreamReader readStream(inbuf, length);
    int32_t cmd;
    if (!readStream.ReadInt32(cmd))
    {
        LOGE("read cmd error, client: %s", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    // int seq;
    if (!readStream.ReadInt32(m_seq))
    {
        LOGE("read seq error, client: %s", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    std::string filemd5;
    size_t md5length;
    if (!readStream.ReadString(&filemd5, 0, md5length) || md5length == 0)
    {
        LOGE("read filemd5 error, client: ", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    int64_t offset;
    if (!readStream.ReadInt64(offset))
    {
        LOGE("read offset error, client: %s", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    int64_t filesize;
    if (!readStream.ReadInt64(filesize))
    {
        LOGE("read filesize error, client: %s", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    string filedata;
    size_t filedatalength;
    if (!readStream.ReadString(&filedata, 0, filedatalength))
    {
        LOGE("read filedata error, client: %s", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    LOGI("Request from client: cmd: %d, seq: %d, filemd5: %s, md5length: %d, offset: %lld, filesize: %lld, filedata length: %lld, header.packagesize: %lld, client: %s",
         cmd, m_seq, filemd5.c_str(), md5length, offset, filesize, (int64_t)filedata.length(), (int64_t)length, conn->peerAddress().toIpPort().c_str());

    // LOG_DEBUG_BIN((unsigned char*)filedata.c_str(), filedatalength);

    switch (cmd)
    {
        // client upload file
    case msg_type_upload_req:
        return onUploadFileResponse(filemd5, offset, filesize, filedata, conn);

        // client download file
    case msg_type_download_req:
    {
        int32_t clientNetType;
        if (!readStream.ReadInt32(clientNetType))
        {
            LOGE("read clientNetType error, client: %s", conn->peerAddress().toIpPort().c_str());
            return false;
        }

        // if (filedatalength != 0)
        //     return false;
        return onDownloadFileResponse(filemd5, clientNetType, conn);
    }

    default:
        // pBuffer->retrieveAll();
        LOGE("unsupport cmd, cmd: %d, client: %s", cmd, conn->peerAddress().toIpPort().c_str());
        // conn->forceClose();
        return false;
    } // end switch

    ++m_seq;

    return true;
}

/**
 * @brief Handles a file upload response from the client.
 *
 * This function processes an upload request by writing the incoming file data chunk
 * to the appropriate location on the server filesystem. It validates input parameters,
 * manages file state and offset, handles file I/O in binary mode, and sends upload progress
 * or completion response back to the client.
 *
 * @param filemd5   The MD5 hash of the file used as its unique identifier.
 * @param offset    The file write offset indicating where to start writing this chunk.
 * @param filesize  The total expected size of the file.
 * @param filedata  The binary content of the file chunk to be written.
 * @param conn      Shared pointer to the TcpConnection associated with the client.
 * @return true     If the upload chunk is handled successfully.
 * @return false    If any error occurs during the process.
 */
bool FileSession::onUploadFileResponse(const std::string &filemd5, int64_t offset, int64_t filesize, const std::string &filedata, const std::shared_ptr<TcpConnection> &conn)
{
    // Validate: filemd5 must not be empty
    if (filemd5.empty())
    {
        LOGE("Empty filemd5, client: %s", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    // Check if file already exists on server and is not currently being uploaded
    if (Singleton<FileManager>::Instance().isFileExsit(filemd5.c_str()) && !m_bFileUploading)
    {
        // File already complete, respond with completion status
        offset = filesize;
        std::string dummyfiledata;
        send(msg_type_upload_resp, m_seq, file_msg_error_complete, filemd5, offset, filesize, dummyfiledata);

        LOGI("Response to client: cmd=msg_type_upload_resp, errorcode: file_msg_error_complete, filemd5: %s, offset: %lld, filesize: %lld, client: %s",
             filemd5.c_str(), offset, filesize, conn->peerAddress().toIpPort().c_str());
        return true;
    }

    // If offset is 0, this is the beginning of the upload
    if (offset == 0)
    {
        std::string filename = m_strFileBaseDir + filemd5;

        // Open file in binary write mode to prevent newline translation issues on Windows
        m_fp = fopen(filename.c_str(), "wb");
        if (m_fp == nullptr)
        {
            LOGE("fopen file error, filemd5: %s, client: %s", filemd5.c_str(), conn->peerAddress().toIpPort().c_str());
            return false;
        }

        m_bFileUploading = true; // Mark file as in-progress
    }
    else
    {
        // For non-zero offsets, file pointer must already be valid
        if (m_fp == nullptr)
        {
            resetFile();
            LOGE("file pointer should not be null, filemd5: %s, offset: %lld, client: %s",
                 filemd5.c_str(), offset, conn->peerAddress().toIpPort().c_str());
            return false;
        }
    }

    // Move the file pointer to the correct offset
    if (fseek(m_fp, offset, SEEK_SET) == -1)
    {
        LOGE("fseek error, filemd5: %s, errno: %d, errinfo: %s, filedata.length(): %lld, m_fp: 0x%x, client: %s",
             filemd5.c_str(), errno, strerror(errno), filedata.length(), m_fp, conn->peerAddress().toIpPort().c_str());
        resetFile();
        return false;
    }

    // Write binary data chunk to file
    if (fwrite(filedata.c_str(), 1, filedata.length(), m_fp) != filedata.length())
    {
        resetFile();
        LOGE("fwrite error, filemd5: %s, errno: %d, errinfo: %s, filedata.length(): %lld, m_fp: 0x%x, client: %s",
             filemd5.c_str(), errno, strerror(errno), filedata.length(), m_fp, conn->peerAddress().toIpPort().c_str());
        return false;
    }

    // Ensure all written data is flushed to disk
    if (fflush(m_fp) != 0)
    {
        LOGE("fflush error, filemd5: %s, errno: %d, errinfo: %s, filedata.length(): %lld, m_fp: 0x%x, client: %s",
             filemd5.c_str(), errno, strerror(errno), filedata.length(), m_fp, conn->peerAddress().toIpPort().c_str());
        return false;
    }

    // Determine current upload status
    int32_t errorcode = file_msg_error_progress;
    int64_t filedataLength = static_cast<int64_t>(filedata.length());

    // Check for upload completion
    if (offset + filedataLength == filesize)
    {
        offset = filesize;
        errorcode = file_msg_error_complete;
        Singleton<FileManager>::Instance().addFile(filemd5.c_str()); // Mark file as complete
        resetFile();                                                 // Close and reset file handle
    }

    std::string dummyfiledatax;
    send(msg_type_upload_resp, m_seq, errorcode, filemd5, offset, filesize, dummyfiledatax);

    std::string errorcodestr = (errorcode == file_msg_error_complete) ? "file_msg_error_complete" : "file_msg_error_progress";

    LOGI("Response to client: cmd=msg_type_upload_resp, errorcode: %s, filemd5: %s, offset: %lld, filedataLength: %lld, filesize: %lld, upload percent: %d%%, client: %s",
         errorcodestr.c_str(), filemd5.c_str(), offset, filedataLength, filesize, (int32_t)(offset * 100 / filesize), conn->peerAddress().toIpPort().c_str());

    return true;
}

/**
 * @brief Handles a client's request to download a file by sending a chunk of the file.
 *
 * This function first checks the validity of the requested file. If the file exists,
 * it opens the file (if not already opened), calculates the appropriate chunk size
 * based on the client's network type (Wi-Fi or cellular), reads a chunk from disk,
 * and sends it to the client along with current progress or completion status.
 *
 * @param filemd5         The MD5 hash identifying the file to download.
 * @param clientNetType   The type of client's network (e.g., Wi-Fi or cellular).
 * @param conn            Shared pointer to the TCP connection to the client.
 * @return true           If the response was successfully prepared and sent.
 * @return false          If an error occurs (e.g., file not found, I/O error).
 */
bool FileSession::onDownloadFileResponse(const std::string &filemd5, int32_t clientNetType, const std::shared_ptr<TcpConnection> &conn)
{
    // Validate input: filemd5 must not be empty
    if (filemd5.empty())
    {
        LOGE("Empty filemd5, client: %s", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    // If file does not exist on server, notify client with error
    if (!Singleton<FileManager>::Instance().isFileExsit(filemd5.c_str()))
    {
        string dummyfiledata;
        int64_t notExsitFileOffset = 0;
        int64_t notExsitFileSize = 0;
        send(msg_type_download_resp, m_seq, file_msg_error_not_exist, filemd5, notExsitFileOffset, notExsitFileSize, dummyfiledata);

        LOGE("File not found: filemd5: %s, clientNetType: %d, client: %s", filemd5.c_str(), clientNetType, conn->peerAddress().toIpPort().c_str());
        LOGI("Response to client: cmd=msg_type_download_resp, errorcode=file_msg_error_not_exist, filemd5=%s, clientNetType=%d, offset=0, filesize=0, filedataLength=0, client=%s",
             filemd5.c_str(), clientNetType, conn->peerAddress().toIpPort().c_str());
        return true;
    }

    // Open file for reading if not already open
    if (m_fp == NULL)
    {
        string filename = m_strFileBaseDir + filemd5;
        m_fp = fopen(filename.c_str(), "rb+");
        if (m_fp == NULL)
        {
            LOGE("Failed to open file: filemd5: %s, clientNetType: %d, client: %s", filemd5.c_str(), clientNetType, conn->peerAddress().toIpPort().c_str());
            return false;
        }

        // Seek to end to determine file size
        if (fseek(m_fp, 0, SEEK_END) == -1)
        {
            LOGE("fseek to end failed, filemd5: %s, errno: %d, client: %s", filemd5.c_str(), errno, conn->peerAddress().toIpPort().c_str());
            return false;
        }

        m_currentDownloadFileSize = ftell(m_fp);
        if (m_currentDownloadFileSize <= 0)
        {
            LOGE("Invalid file size: %lld, filemd5: %s, client: %s", m_currentDownloadFileSize, filemd5.c_str(), conn->peerAddress().toIpPort().c_str());
            return false;
        }

        // Seek back to beginning to prepare for reading
        if (fseek(m_fp, 0, SEEK_SET) == -1)
        {
            LOGE("fseek to start failed, filemd5: %s, client: %s", filemd5.c_str(), conn->peerAddress().toIpPort().c_str());
            return false;
        }
    }

    string filedata;
    int64_t currentSendSize = 512 * 1024; // Default chunk size for Wi-Fi clients

    // For cellular clients, reduce chunk size to 64KB
    if (clientNetType == client_net_type_cellular)
        currentSendSize = 64 * 1024;

    // Adjust chunk size if reaching file end
    if (m_currentDownloadFileSize <= m_currentDownloadFileOffset + currentSendSize)
        currentSendSize = m_currentDownloadFileSize - m_currentDownloadFileOffset;

    char buffer[512 * 1024] = {0};

    // Read chunk from file
    if (currentSendSize <= 0 || fread(buffer, currentSendSize, 1, m_fp) != 1)
    {
        LOGE("fread error, filemd5: %s, errno: %d, msg: %s, size: %lld, client: %s",
             filemd5.c_str(), errno, strerror(errno), currentSendSize, conn->peerAddress().toIpPort().c_str());
    }

    int64_t sendoffset = m_currentDownloadFileOffset;
    m_currentDownloadFileOffset += currentSendSize;
    filedata.append(buffer, currentSendSize);

    // Determine progress or completion
    int errorcode = file_msg_error_progress;
    if (m_currentDownloadFileOffset == m_currentDownloadFileSize)
        errorcode = file_msg_error_complete;

    // Send response with chunk to client
    send(msg_type_download_resp, m_seq, errorcode, filemd5, sendoffset, m_currentDownloadFileSize, filedata);

    // Log response details
    LOGI("Response to client: cmd=msg_type_download_resp, errorcode=%s, filemd5=%s, clientNetType=%d, offset=%lld, filesize=%lld, dataLen=%zu, percent=%d%%, client=%s",
         (errorcode == file_msg_error_progress ? "file_msg_error_progress" : "file_msg_error_complete"),
         filemd5.c_str(), clientNetType, sendoffset, m_currentDownloadFileSize,
         filedata.length(), (int)(m_currentDownloadFileOffset * 100 / m_currentDownloadFileSize),
         conn->peerAddress().toIpPort().c_str());

    // If download is complete, reset internal file state
    if (errorcode == file_msg_error_complete)
        resetFile();

    return true;
}

void FileSession::resetFile()
{
    if (m_fp != NULL)
    {
        fclose(m_fp);
        m_currentDownloadFileOffset = 0;
        m_currentDownloadFileSize = 0;
        m_fp = NULL;
        m_bFileUploading = false;
    }
}