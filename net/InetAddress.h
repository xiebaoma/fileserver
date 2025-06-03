/*
 * Author: xiebaoma
 * Date: 2025-06-03
 * Description: Defines the InetAddress class, which is a wrapper around
 * sockaddr_in for convenient manipulation and representation of IPv4 addresses
 * and ports. Provides utilities for address formatting, resolution, and access.
 */

#pragma once

#include <string>
#include "../base/Platform.h"

namespace net
{
    /**
     * @brief Wrapper class for IPv4 socket address (sockaddr_in).
     *
     * Provides easy-to-use constructors and methods to convert between
     * raw socket address structures and human-readable IP/port formats.
     */
    class InetAddress
    {
    public:
        /**
         * @brief Constructs an InetAddress with the given port.
         *
         * @param port         Port number in host byte order.
         * @param loopbackOnly If true, binds to 127.0.0.1. Otherwise, binds to INADDR_ANY.
         */
        explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false);

        /**
         * @brief Constructs an InetAddress from an IP string and port.
         *
         * @param ip    A string representing an IPv4 address (e.g., "192.168.1.1").
         * @param port  Port number in host byte order.
         */
        InetAddress(const std::string &ip, uint16_t port);

        /**
         * @brief Constructs an InetAddress from an existing sockaddr_in.
         *
         * @param addr The raw sockaddr_in structure.
         */
        InetAddress(const struct sockaddr_in &addr)
            : m_addr(addr)
        {
        }

        /**
         * @brief Returns the IP address as a string (e.g., "192.168.1.1").
         */
        std::string toIp() const;

        /**
         * @brief Returns the IP and port as a string (e.g., "192.168.1.1:80").
         */
        std::string toIpPort() const;

        /**
         * @brief Returns the port number in host byte order.
         */
        uint16_t toPort() const;

        /**
         * @brief Returns the internal sockaddr_in structure (const reference).
         */
        const struct sockaddr_in &getSockAddrInet() const { return m_addr; }

        /**
         * @brief Sets the internal sockaddr_in structure.
         *
         * @param addr The new sockaddr_in to assign.
         */
        void setSockAddrInet(const struct sockaddr_in &addr) { m_addr = addr; }

        /**
         * @brief Returns the IP address in network byte order.
         */
        uint32_t ipNetEndian() const { return m_addr.sin_addr.s_addr; }

        /**
         * @brief Returns the port number in network byte order.
         */
        uint16_t portNetEndian() const { return m_addr.sin_port; }

        /**
         * @brief Resolves a hostname (e.g., "example.com") to an IP address.
         *
         * @param hostname The hostname to resolve.
         * @param result   Output parameter to store the resolved InetAddress.
         * @return true if resolution is successful, false otherwise.
         */
        static bool resolve(const std::string &hostname, InetAddress *result);

    private:
        // Underlying socket address structure.
        struct sockaddr_in m_addr;
    };

}
