/*
 * Author: xiebaoma
 * Date: 2025-06-04
 *
 * Description:
 * This header defines the Acceptor class in the net namespace.
 * It encapsulates the logic for accepting new incoming TCP connections
 * and dispatching them via a callback to the upper layer.
 */

#pragma once

#include <functional>

#include "Channel.h"
#include "Sockets.h"

namespace net
{
    class EventLoop;
    class InetAddress;

    class Acceptor
    {
    public:
        // Callback type for handling new incoming connections.
        typedef std::function<void(int sockfd, const InetAddress &)> NewConnectionCallback;

        // Constructor: initializes the acceptor with the given EventLoop,
        // listening address, and reuse port option.
        Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);

        // Destructor.
        ~Acceptor();

        // Sets the callback to be called when a new connection is accepted.
        void setNewConnectionCallback(const NewConnectionCallback &cb)
        {
            m_newConnectionCallback = cb;
        }

        // Returns true if the acceptor is currently listening for connections.
        bool listenning() const { return m_listenning; }

        // Starts listening for incoming connections.
        void listen();

    private:
        // Internal handler for read events on the listening socket.
        void handleRead();

    private:
        EventLoop *m_loop;                             // Event loop that handles events for this acceptor.
        Socket m_acceptSocket;                         // Socket used to accept incoming connections.
        Channel m_acceptChannel;                       // Channel associated with the accept socket.
        NewConnectionCallback m_newConnectionCallback; // Callback for new connections.
        bool m_listenning;                             // Indicates whether the acceptor is currently listening.

#ifndef WIN32
        int m_idleFd; // Used to reserve a file descriptor to avoid EMFILE errors on Unix.
#endif
    };
}
