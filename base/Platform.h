/*
 * Author: xiebaoma
 * Date: 2025-06-03
 * Description: This header provides a platform abstraction layer for network-related
 * definitions and includes. It ensures compatibility across Windows and Unix-like systems
 * (e.g., Linux), particularly in socket programming and epoll/poll event handling.
 */

#pragma once

#include <stdint.h>

// Suppress deprecated warnings depending on the compiler
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(disable : 4996)
#endif

#ifdef WIN32

// Link necessary Windows libraries
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Shlwapi.lib")

// Define missing socket-related types for Windows compatibility
typedef int socklen_t;
typedef unsigned int in_addr_t;

// Define poll/epoll event flags manually for cross-platform compatibility
#define XPOLLIN 1
#define XPOLLPRI 2
#define XPOLLOUT 4
#define XPOLLERR 8
#define XPOLLHUP 16
#define XPOLLNVAL 32
#define XPOLLRDHUP 8192

// Define epoll control operation constants
#define XEPOLL_CTL_ADD 1
#define XEPOLL_CTL_DEL 2
#define XEPOLL_CTL_MOD 3

// Define epoll_data_t and epoll_event to emulate Linux-style epoll on Windows
#pragma pack(push, 1)
typedef union epoll_data
{
    void *ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;

struct epoll_event
{
    uint32_t events;   // Epoll event bitmask
    epoll_data_t data; // User-defined data associated with event
};
#pragma pack(pop)

// Include essential Windows networking headers
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <Ws2ipdef.h>
#include <io.h>      // for _pipe
#include <fcntl.h>   // for O_BINARY
#include <shlwapi.h> // for Path APIs

// Automatically initializes and cleans up Winsock on object lifetime
class NetworkInitializer
{
public:
    NetworkInitializer();
    ~NetworkInitializer();
};

#else // Non-Windows platforms

// Define socket constants and macros for Linux/Unix
typedef int SOCKET;

#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define closesocket(s) close(s)

// Include essential Unix socket and system headers
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <stdint.h>
#include <endian.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <inttypes.h>
#include <errno.h>
#include <dirent.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/epoll.h>
#include <sys/syscall.h>

// Ensure compatibility on Ubuntu for readv() usage
#ifdef __UBUNTU
#include <sys/uio.h>
#endif

// Map Linux poll/epoll constants to generic macros
#define XPOLLIN POLLIN
#define XPOLLPRI POLLPRI
#define XPOLLOUT POLLOUT
#define XPOLLERR POLLERR
#define XPOLLHUP POLLHUP
#define XPOLLNVAL POLLNVAL
#define XPOLLRDHUP POLLRDHUP

#define XEPOLL_CTL_ADD EPOLL_CTL_ADD
#define XEPOLL_CTL_DEL EPOLL_CTL_DEL
#define XEPOLL_CTL_MOD EPOLL_CTL_MOD

// Define byte order conversion macros for 64-bit values on Linux
#define ntohll(x) be64toh(x)
#define htonll(x) htobe64(x)

#endif // WIN32
