/**
 *  @brief File Management Class Implementation
 *  @author: xiebaoam
 *  @date: 2025-05-25
 **/
#include "FileManager.h"

#include <string.h>

#include "../base/AsyncLog.h"
#include "../base/Platform.h"

/**
 * @brief Default constructor
 */
FileManager::FileManager()
{
}

/**
 * @brief Destructor
 */
FileManager::~FileManager()
{
}

/**
 * @brief Initialize the file manager with a base path
 *
 * Creates the base directory if it doesn't exist and loads existing files
 * into the file list on non-Windows platforms.
 *
 * @param basepath The base directory path for file operations
 * @return true if initialization succeeds, false otherwise
 */
bool FileManager::init(const char *basepath)
{
    m_basepath = basepath;

#ifdef WIN32
    // Create directory on Windows if it doesn't exist
    if (!PathFileExistsA(basepath))
    {
        LOGE("basepath %s doesnot exist.", basepath);

        if (!CreateDirectoryA(basepath, NULL))
        {
            LOGE("create base dir error, %s", basepath);
            return false;
        }
    }

    return true;
#else
    DIR *dp = opendir(basepath);
    if (dp == NULL)
    {
        LOGE("open base dir error, errno: %d, %s", errno, strerror(errno));

        if (mkdir(basepath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
            return true;

        LOGE("create base dir error, %s , errno: %d, %s", basepath, errno, strerror(errno));

        return false;
    }

    struct dirent *dirp;
    // struct stat filestat;
    while ((dirp = readdir(dp)) != NULL)
    {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
            continue;

        // if (stat(dirp->d_name, &filestat) != 0)
        //{
        //     LOGW << "stat filename: [" << dirp->d_name << "] error, errno: " << errno << ", " << strerror(errno);
        //     continue;
        // }

        m_listFiles.emplace_back(dirp->d_name);
        LOGI("filename: %s", dirp->d_name);
    }

    closedir(dp);
#endif

    return true;
}

/**
 * @brief Check if a file exists
 *
 * First checks the cached file list, then checks the file system if not found in cache.
 * If found in the file system but not in cache, adds the file to the cache.
 *
 * @param filename The name of the file to check
 * @return true if the file exists, false otherwise
 */
bool FileManager::isFileExsit(const char *filename)
{
    std::lock_guard<std::mutex> guard(m_mtFile);
    // First check the cache
    for (const auto &iter : m_listFiles)
    {
        if (iter == filename)
            return true;
    }

    // Then check the file system
    std::string filepath = m_basepath;
    filepath += filename;
    FILE *fp = fopen(filepath.c_str(), "r");
    if (fp != NULL)
    {
        fclose(fp);
        m_listFiles.emplace_back(filename);
        return true;
    }

    return false;
}

/**
 * @brief Add a file to the managed file list
 *
 * Thread-safe method to add a filename to the internal cache.
 *
 * @param filename The name of the file to add
 */
void FileManager::addFile(const char *filename)
{
    std::lock_guard<std::mutex> guard(m_mtFile);
    m_listFiles.emplace_back(filename);
}
