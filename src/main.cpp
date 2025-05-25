/**
 *  @file main.cpp
 *  @brief Entry point for the file server application
 *  @author xiebaoma
 *  @date 2025-05-25
 **/
#include <iostream>
#include <stdlib.h>

#include "../base/Platform.h"
#include "../base/Singleton.h"
#include "../base/ConfigFileReader.h"
#include "../base/AsyncLog.h"
#include "../net/EventLoop.h"
#include "FileManager.h"

#ifndef WIN32
#include <string.h>
#include "../utils/DaemonRun.h"
#endif

#include "FileServer.h"

using namespace net;

#ifdef WIN32
// Initialize Windows socket library
NetworkInitializer windowsNetworkInitializer;
#endif

/**
 * @brief Main event loop for the application
 */
EventLoop g_mainLoop;

#ifndef WIN32
/**
 * @brief Signal handler for graceful program exit
 * @param signo Signal number received
 */
void prog_exit(int signo)
{
    std::cout << "program recv signal [" << signo << "] to exit." << std::endl;

    Singleton<FileServer>::Instance().uninit();
    g_mainLoop.quit();
}
#endif

/**
 * @brief Entry point for the FileServer program.
 *
 * Initializes configuration, logging, file manager, and the file server itself.
 * Supports daemon mode on Unix-based systems.
 */
int main(int argc, char *argv[])
{
#ifndef WIN32
    // Set signal handlers for Unix-based systems
    signal(SIGCHLD, SIG_DFL);   // Use default handler for child termination
    signal(SIGPIPE, SIG_IGN);   // Ignore broken pipe signal
    signal(SIGINT, prog_exit);  // Gracefully exit on Ctrl+C
    signal(SIGTERM, prog_exit); // Gracefully exit on kill

    // Parse command-line arguments
    int ch;
    bool bdaemon = false;
    while ((ch = getopt(argc, argv, "d")) != -1)
    {
        switch (ch)
        {
        case 'd':
            bdaemon = true; // Run as a daemon if -d is provided
            break;
        }
    }

    // Launch as daemon process if requested
    if (bdaemon)
        daemon_run();
#endif

    // Load configuration file path (platform dependent)
#ifdef WIN32
    CConfigFileReader config("../etc/fileserver.conf");
#else
    CConfigFileReader config("etc/fileserver.conf");
#endif

    std::string logFileFullPath;

#ifndef WIN32
    // Retrieve the log directory from config
    const char *logfilepath = config.getConfigName("logfiledir");
    if (logfilepath == NULL)
    {
        LOGF("logdir is not set in config file");
        return 1;
    }

    // Create log directory if it does not exist
    DIR *dp = opendir(logfilepath);
    if (dp == NULL)
    {
        if (mkdir(logfilepath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
        {
            LOGF("Failed to create log directory: %s, errno: %d, %s",
                 logfilepath, errno, strerror(errno));
            return 1;
        }
    }
    else
    {
        closedir(dp);
    }

    logFileFullPath = logfilepath;
#endif

    // Append log file name from config
    const char *logfilename = config.getConfigName("logfilename");
    logFileFullPath += logfilename;

    // Initialize the asynchronous logger
    CAsyncLog::init(logFileFullPath.c_str());

    // Initialize the file manager with the file cache directory
    const char *filecachedir = config.getConfigName("filecachedir");
    Singleton<FileManager>::Instance().init(filecachedir);

    // Retrieve listening IP and port from config and initialize the file server
    const char *listenip = config.getConfigName("listenip");
    short listenport = (short)atol(config.getConfigName("listenport"));
    Singleton<FileServer>::Instance().init(listenip, listenport, &g_mainLoop, filecachedir);

    LOGI("FileServer initialization completed. Ready to accept client connections.");

    // Enter the main event loop
    g_mainLoop.loop();

    LOGI("FileServer exited.");

    return 0;
}
