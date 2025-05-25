/**
 *  File Management Class, FileManager.h
 *  Author: xiebaoma
 *  Date: 2025-05-25
 **/
#pragma once
#include <string>
#include <list>
#include <mutex>

/**
 * @class FileManager
 * @brief Manages file operations and maintains a list of uploaded files
 *
 * This class handles file existence checking and registration of new files.
 * It is designed as a final class that cannot be inherited from.
 */
class FileManager final
{
public:
    /**
     * @brief Default constructor
     */
    FileManager();

    /**
     * @brief Destructor
     */
    ~FileManager();

    /**
     * @brief Copy constructor (deleted)
     * @param rhs The source object
     */
    FileManager(const FileManager &rhs) = delete;

    /**
     * @brief Assignment operator (deleted)
     * @param rhs The source object
     * @return Reference to this object
     */
    FileManager &operator=(const FileManager &rhs) = delete;

    /**
     * @brief Initialize the file manager with a base path
     * @param basepath The base directory path for file operations
     * @return true if initialization succeeds, false otherwise
     */
    bool init(const char *basepath);

    /**
     * @brief Check if a file exists
     * @param filename The name of the file to check
     * @return true if the file exists, false otherwise
     */
    bool isFileExsit(const char *filename);

    /**
     * @brief Add a file to the managed file list
     * @param filename The name of the file to add
     */
    void addFile(const char *filename);

private:
    // All uploaded files are named by their MD5 hash values
    std::list<std::string> m_listFiles; /**< List of managed file names */
    std::mutex m_mtFile;                /**< Mutex for thread-safe file list operations */
    std::string m_basepath;             /**< Base directory path for file operations */
};