/*
 * Author: xiebaoma
 * Date: 2025-06-01
 *
 * Description:
 * Channel is a core abstraction in the Reactor pattern. It represents a file descriptor
 * (e.g., socket, eventfd, timerfd) and its associated events (read/write/error/close).
 * Channel is used by EventLoop to dispatch events to the corresponding callbacks.
 */

#pragma once

#include <memory>
#include <functional>
#include "../base/Timestamp.h"

namespace net
{
    class EventLoop;

    class Channel
    {
    public:
        // Type alias for callback functions
        typedef std::function<void()> EventCallback;
        typedef std::function<void(Timestamp)> ReadEventCallback;

        // Constructor: binds the channel to an EventLoop and a file descriptor
        Channel(EventLoop *loop, int fd);

        // Destructor
        ~Channel();

        // Dispatch the appropriate callback based on active events
        void handleEvent(Timestamp receiveTime);

        // Set the callback for read events
        void setReadCallback(const ReadEventCallback &cb)
        {
            m_readCallback = cb;
        }

        // Set the callback for write events
        void setWriteCallback(const EventCallback &cb)
        {
            m_writeCallback = cb;
        }

        // Set the callback for close events
        void setCloseCallback(const EventCallback &cb)
        {
            m_closeCallback = cb;
        }

        // Set the callback for error events
        void setErrorCallback(const EventCallback &cb)
        {
            m_errorCallback = cb;
        }

        // Return the associated file descriptor
        int fd() const { return m_fd; }

        // Return the interested events (bitmask)
        int events() const { return m_events; }

        // Set the actual events that occurred (set by poller)
        void set_revents(int revt) { m_revents = revt; }

        // Add new occurred events to existing revents
        void add_revents(int revt) { m_revents |= revt; }

        // Check if no events are currently registered
        bool isNoneEvent() const { return m_events == kNoneEvent; }

        // Enable reading (EPOLLIN)
        bool enableReading();

        // Disable reading
        bool disableReading();

        // Enable writing (EPOLLOUT)
        bool enableWriting();

        // Disable writing
        bool disableWriting();

        // Disable all events (both read and write)
        bool disableAll();

        // Check if writing is currently enabled
        bool isWriting() const { return m_events & kWriteEvent; }

        // Get the index used by the poller (e.g., epoll)
        int index() { return m_index; }

        // Set the index used by the poller
        void set_index(int idx) { m_index = idx; }

        // Convert revents to a string for logging/debugging
        string reventsToString() const;

        // Return the owner event loop
        EventLoop *ownerLoop() { return m_loop; }

        // Remove the channel from the poller
        void remove();

    private:
        // Ask the poller to update the events for this channel
        bool update();

        // Event flags used to track interest in different types of events
        static const int kNoneEvent;
        static const int kReadEvent;
        static const int kWriteEvent;

        // Associated EventLoop
        EventLoop *m_loop;

        // File descriptor associated with this channel
        const int m_fd;

        // Interested events (registered with the poller)
        int m_events;

        // Events that were returned by poll (actual events occurred)
        int m_revents;

        // Index used by poller (e.g., in epoll's fd list)
        int m_index;

        // Callback functions for various events
        ReadEventCallback m_readCallback;
        EventCallback m_writeCallback;
        EventCallback m_closeCallback;
        EventCallback m_errorCallback;
    };
}
