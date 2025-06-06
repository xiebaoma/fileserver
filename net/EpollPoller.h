/**
 * @file EpollPoller.h
 * @brief A concrete implementation of Poller using epoll(7) for I/O multiplexing on Linux.
 *
 * This class is part of a reactor-based event loop system.
 * It uses epoll to efficiently monitor multiple file descriptors to see if I/O is possible.
 *
 * @author xiebaoma
 * @date 2025-06-06
 */

#pragma once

#ifndef WIN32 // Only compile this file on non-Windows systems (i.e., Linux)

#include <vector>
#include <map>

#include "../base/Timestamp.h"
#include "Poller.h"

struct epoll_event;

namespace net
{
    class EventLoop;

    /**
     * @class EPollPoller
     * @brief epoll-based implementation of the Poller interface.
     *
     * Handles registration, modification, removal, and polling of file descriptors
     * through epoll. Each Poller instance is bound to a specific EventLoop.
     */
    class EPollPoller : public Poller
    {
    public:
        /**
         * @brief Constructor that initializes epoll instance.
         * @param loop Pointer to the EventLoop that owns this Poller.
         */
        EPollPoller(EventLoop *loop);

        /**
         * @brief Destructor. Cleans up the epoll instance.
         */
        virtual ~EPollPoller();

        /**
         * @brief Waits for I/O events with a timeout, and populates active channels.
         * @param timeoutMs Maximum time to wait in milliseconds.
         * @param activeChannels Output list of channels that have active events.
         * @return Timestamp indicating when the poll returned.
         */
        virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels);

        /**
         * @brief Adds or updates the given channel in the epoll interest list.
         * @param channel The channel to add or update.
         * @return True if successful, false otherwise.
         */
        virtual bool updateChannel(Channel *channel);

        /**
         * @brief Removes the given channel from epoll.
         * @param channel The channel to remove.
         */
        virtual void removeChannel(Channel *channel);

        /**
         * @brief Checks if the given channel exists in the internal map.
         * @param channel The channel to check.
         * @return True if found, false otherwise.
         */
        virtual bool hasChannel(Channel *channel) const;

        /**
         * @brief Ensures this method is called from the associated EventLoop thread.
         */
        void assertInLoopThread() const;

    private:
        static const int kInitEventListSize = 16; ///< Initial size for the event buffer

        /**
         * @brief Populates the activeChannels list based on epoll events.
         * @param numEvents Number of events returned by epoll_wait().
         * @param activeChannels Output list to fill.
         */
        void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

        /**
         * @brief Adds, modifies, or deletes a channel's fd in epoll.
         * @param operation EPOLL_CTL_ADD / MOD / DEL.
         * @param channel The channel whose fd is to be operated on.
         * @return True if operation succeeds, false otherwise.
         */
        bool update(int operation, Channel *channel);

    private:
        typedef std::vector<struct epoll_event> EventList; ///< Container for epoll events

        int m_epollfd;      ///< File descriptor for the epoll instance
        EventList m_events; ///< Buffer used to store returned events

        typedef std::map<int, Channel *> ChannelMap;

        ChannelMap m_channels;  ///< Map from fd to corresponding Channel*
        EventLoop *m_ownerLoop; ///< The EventLoop that owns this Poller
    };
}

#endif // WIN32
