/**
 *  @file FileServer.h
 *  @brief Main file server service class
 *  @author xiebaoma
 *  @date 2025-05-25
 **/
#pragma once
#include <memory>
#include <list>
#include <map>
#include <mutex>
#include "../net/TcpServer.h"
#include "../net/EventLoop.h"
#include "FileSession.h"

using namespace net;

/**
 * @struct StoredUserInfo
 * @brief Structure to store user information
 */
struct StoredUserInfo
{
    int32_t userid;       /**< User ID */
    std::string username; /**< Username */
    std::string password; /**< Password */
    std::string nickname; /**< User nickname */
};

/**
 * @class FileServer
 * @brief Main file server class that manages connections and file sessions
 * 
 * This class is responsible for initializing the TCP server,
 * handling connections, and managing file sessions.
 */
class FileServer final
{
public:
    /**
     * @brief Default constructor
     */
    FileServer() = default;
    
    /**
     * @brief Default destructor
     */
    ~FileServer() = default;

    /**
     * @brief Copy constructor (deleted)
     * @param rhs Source object
     */
    FileServer(const FileServer &rhs) = delete;
    
    /**
     * @brief Assignment operator (deleted)
     * @param rhs Source object
     * @return Reference to this object
     */
    FileServer &operator=(const FileServer &rhs) = delete;

    /**
     * @brief Initialize the file server
     * @param ip IP address to bind
     * @param port Port to listen on
     * @param loop Event loop for the server
     * @param fileBaseDir Base directory for file storage (default: "filecache/")
     * @return true if initialization succeeds, false otherwise
     */
    bool init(const char *ip, short port, EventLoop *loop, const char *fileBaseDir = "filecache/");
    
    /**
     * @brief Uninitialize the file server
     */
    void uninit();

private:
    /**
     * @brief Callback for new connections or disconnections
     * 
     * Called when a new connection is established or a connection is closed.
     * Use conn->connected() to determine the connection state.
     * Generally only called in the main event loop.
     * 
     * @param conn Shared pointer to the TCP connection
     */
    void onConnected(std::shared_ptr<TcpConnection> conn);
    
    /**
     * @brief Callback for disconnected connections
     * @param conn Shared pointer to the TCP connection
     */
    void onDisconnected(const std::shared_ptr<TcpConnection> &conn);

private:
    std::unique_ptr<TcpServer> m_server;                /**< TCP server instance */
    std::list<std::shared_ptr<FileSession>> m_sessions; /**< List of active file sessions */
    std::mutex m_sessionMutex;                          /**< Mutex to protect m_sessions in multi-threaded context */
    std::string m_strFileBaseDir;                       /**< Base directory for file storage */
};
