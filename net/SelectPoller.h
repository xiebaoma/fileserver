/*
 * Author: xiebaoma
 * Date: 2025-06-01
 *
 * Description:
 * SelectPoller is a concrete implementation of the Poller interface using
 * the select() system call. It manages file descriptors and detects which
 * ones are ready for I/O events, dispatching them to the EventLoop.
 * This implementation is mainly for platform compatibility or teaching/demo purposes,
 * as select() has limitations in scalability compared to epoll/kqueue.
 */

#pragma once

#include "Poller.h"
#include <map>
#include "../base/Platform.h"

namespace net
{
    class EventLoop;
    class Channel;

    class SelectPoller : public Poller
    {
    public:
        // Constructor: binds this poller to the given EventLoop
        SelectPoller(EventLoop *loop);

        // Destructor
        virtual ~SelectPoller();

        // Waits for I/O events using select() and fills activeChannels with ready Channels
        virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;

        // Add or update a Channel in the internal FD set
        virtual bool updateChannel(Channel *channel) override;

        // Remove a Channel from the internal FD set
        virtual void removeChannel(Channel *channel) override;

        // Check if the specified Channel is currently being monitored
        virtual bool hasChannel(Channel *channel) const override;

        // Ensures this method is called from the correct thread (owner thread)
        void assertInLoopThread() const;

    private:
        // Initial size of the internal event list
        static const int kInitEventListSize = 16;

        // Fill activeChannels based on the FDs that are ready (from select)
        void fillActiveChannels(int numEvents, ChannelList *activeChannels, fd_set &readfds, fd_set &writefds) const;

        // Internal method for adding/updating/removing a Channel's FD from the internal map
        bool update(int operation, Channel *channel);

    private:
        // Typedef for a list of events (note: unused here as select doesn't use epoll_event)
        typedef std::vector<struct epoll_event> EventList;

        // File descriptor for epoll (not used in select version, might be a placeholder or legacy field)
        int m_epollfd;

        // Placeholder for event storage (not used by select)
        EventList m_events;

        // Mapping from file descriptor to Channel*
        typedef std::map<int, Channel *> ChannelMap;
        ChannelMap m_channels;

        // Pointer to the owner EventLoop, used for thread-safety checks
        EventLoop *m_ownerLoop;
    };
}
