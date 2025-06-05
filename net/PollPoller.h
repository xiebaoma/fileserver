/*
 * Author: xiebaoma
 * Date: 2025-06-05
 *
 * Description:
 * This header defines the PollPoller class, a concrete implementation of the Poller interface
 * using the poll(2) system call for I/O multiplexing. This implementation is designed for
 * non-Windows systems only (i.e., UNIX-like environments).
 */

#pragma once

#ifndef WIN32

#include "Poller.h"

#include <vector>
#include <map>

struct pollfd;

namespace net
{
    class Channel;
    class EventLoop;

    // PollPoller implements the Poller interface using the poll system call.
    class PollPoller : public Poller
    {
    public:
        // Constructor: takes a pointer to the owning EventLoop.
        PollPoller(EventLoop *loop);

        // Destructor: cleans up resources if needed.
        virtual ~PollPoller();

        // Waits for I/O events with a timeout and fills the activeChannels list.
        virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels);

        // Updates or adds a Channel to the polling set.
        virtual bool updateChannel(Channel *channel);

        // Removes a Channel from the polling set.
        virtual void removeChannel(Channel *channel);

        // Asserts that the current thread is the loop thread.
        void assertInLoopThread() const;

    private:
        // Populates the activeChannels list based on the events returned by poll.
        void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

    private:
        // Type alias for the list of pollfd structures used in the poll system call.
        typedef std::vector<struct pollfd> PollFdList;

        // Maps file descriptors to their corresponding Channel objects.
        typedef std::map<int, Channel *> ChannelMap;

        ChannelMap m_channels;  // Map of active channels.
        PollFdList m_pollfds;   // List of pollfd structures for polling.
        EventLoop *m_ownerLoop; // The EventLoop that owns this Poller.
    };

} // namespace net

#endif // WIN32
