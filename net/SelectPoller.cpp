/*
 * Author: xiebaoma
 * Date: 2025-06-01
 *
 * Description:
 * implementation of SelectPoller class, which is a concrete implementation of the Poller interface
 */

#include "SelectPoller.h"

#include <sstream>
#include <string.h>

#include "../base/Platform.h"
#include "../base/AsyncLog.h"
#include "EventLoop.h"
#include "Channel.h"

using namespace net;

namespace
{
    const int kNew = -1;    // The channel is new and not yet added to the poller
    const int kAdded = 1;   // The channel is already added to the poller
    const int kDeleted = 2; // The channel was added but has been removed
}

SelectPoller::SelectPoller(EventLoop *loop) : m_ownerLoop(loop)
{
}

SelectPoller::~SelectPoller()
{
}

bool SelectPoller::hasChannel(Channel *channel) const
{
    assertInLoopThread();
    ChannelMap::const_iterator it = m_channels.find(channel->fd());
    return it != m_channels.end() && it->second == channel;
}

void SelectPoller::assertInLoopThread() const
{
    m_ownerLoop->assertInLoopThread();
}

/**
 * @brief Waits for I/O events on registered file descriptors using the select() system call.
 *        This method checks which channels (file descriptors) are ready for reading or writing
 *        within the given timeout and fills the list of active channels accordingly.
 *
 * @param timeoutMs      Timeout in milliseconds to wait for events.
 * @param activeChannels Output list to store the channels that have active events.
 * @return Timestamp      The current timestamp after select() returns.
 */
Timestamp SelectPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    // Determine the maximum file descriptor for select()
    int maxfd = 0;
    int tmpfd;

    // Prepare read and write fd sets based on registered channels
    for (const auto &iter : m_channels)
    {
        tmpfd = iter.first;
        if (tmpfd > maxfd)
            maxfd = tmpfd;

        // Monitor read events if XPOLLIN is set
        if (iter.second->events() & XPOLLIN)
            FD_SET(tmpfd, &readfds);

        // Monitor write events if XPOLLOUT is set
        if (iter.second->events() & XPOLLOUT)
            FD_SET(tmpfd, &writefds);
    }

    // Convert timeout from milliseconds to timeval struct
    struct timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs - timeout.tv_sec * 1000) * 1000;

    // Call select() to wait for events
    int numEvents = select(maxfd + 1, &readfds, &writefds, NULL, &timeout);

    // Record current timestamp after select() returns
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
        // Events occurred: fill the activeChannels list
        fillActiveChannels(numEvents, activeChannels, readfds, writefds);

        // Optionally resize internal event list (placeholder for epoll compatibility)
        if (static_cast<size_t>(numEvents) == m_events.size())
        {
            m_events.resize(m_events.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        // Timeout occurred: no events
        // LOG_TRACE << " nothing happened";
    }
    else
    {
        // select() error occurred
        int savedErrno;
#ifdef WIN32
        savedErrno = (int)WSAGetLastError(); // Windows-specific error
#else
        savedErrno = errno; // POSIX error
#endif
        LOGSYSE("SelectPoller::poll() error, errno: %d", savedErrno);
    }

    return now;
}

void SelectPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels, fd_set &readfds, fd_set &writefds) const
{
    Channel *channel = NULL;
    bool eventTriggered = false;
    int currentCount = 0;
    for (const auto &iter : m_channels)
    {
        channel = iter.second;

        if (FD_ISSET(iter.first, &readfds))
        {
            channel->add_revents(XPOLLIN);
            eventTriggered = true;
        }

        if (FD_ISSET(iter.first, &writefds))
        {
            channel->add_revents(XPOLLOUT);
            eventTriggered = true;
        }

        if (eventTriggered)
        {
            activeChannels->push_back(channel);
            eventTriggered = false;

            ++currentCount;
            if (currentCount >= numEvents)
                break;
        }
    } // end for-loop
}

/**
 * @brief Update or register a channel with the poller.
 *
 * This function handles both:
 * 1. Adding a new channel (index is kNew or kDeleted).
 * 2. Updating an existing channel (index is kAdded).
 *
 * It maintains the internal m_channels map and calls the `update` helper
 * with the appropriate operation (XEPOLL_CTL_ADD, XEPOLL_CTL_MOD, XEPOLL_CTL_DEL).
 *
 * @param channel The channel to be updated or added.
 * @return true if the operation succeeded, false if any consistency check fails or update fails.
 */
bool SelectPoller::updateChannel(Channel *channel)
{
    assertInLoopThread(); // Ensure the operation happens in the owning EventLoop thread

    const int index = channel->index();

    if (index == kNew || index == kDeleted)
    {
        // Case 1: New or previously deleted channel; try to add

        int fd = channel->fd();

        if (index == kNew)
        {
            // If index is kNew, ensure it is not already in the poller
            if (m_channels.find(fd) != m_channels.end())
            {
                LOGE("fd = %d must not exist in channels_", fd);
                return false;
            }

            // Add the new channel to the internal map
            m_channels[fd] = channel;
        }
        else // index == kDeleted
        {
            // Channel was previously deleted; ensure it exists in the map
            if (m_channels.find(fd) == m_channels.end())
            {
                LOGE("fd = %d must not exist in channels_", fd);
                return false;
            }

            // Sanity check: channel pointer must match
            if (m_channels[fd] != channel)
            {
                LOGE("current channel is not matched current fd, fd = %d", fd);
                return false;
            }
        }

        // Mark channel as added
        channel->set_index(kAdded);

        // Call internal update to add the channel (for select, this may be a no-op)
        return update(XEPOLL_CTL_ADD, channel);
    }
    else
    {
        // Case 2: Existing channel; try to modify or delete

        int fd = channel->fd();

        // Validate the channel exists and is consistent
        if (m_channels.find(fd) == m_channels.end() || m_channels[fd] != channel || index != kAdded)
        {
            LOGE("current channel is not matched current fd, fd = %d, channel = 0x%x", fd, channel);
            return false;
        }

        if (channel->isNoneEvent())
        {
            // If the channel no longer wants any events, remove it from the poller
            if (update(XEPOLL_CTL_DEL, channel))
            {
                channel->set_index(kDeleted);
                return true;
            }
            return false;
        }
        else
        {
            // Modify the events the channel is interested in
            return update(XEPOLL_CTL_MOD, channel);
        }
    }
}

void SelectPoller::removeChannel(Channel *channel)
{
    assertInLoopThread();
    int fd = channel->fd();
    // LOG_TRACE << "fd = " << fd;
    // assert(channels_.find(fd) != channels_.end());
    if (m_channels.find(fd) == m_channels.end())
        return;

    if (m_channels[fd] != channel)
    {
        return;
    }
    // assert(channels_[fd] == channel);
    // assert(channel->isNoneEvent());

    if (!channel->isNoneEvent())
        return;

    int index = channel->index();
    // assert(index == kAdded || index == kDeleted);
    size_t n = m_channels.erase(fd);
    if (n != 1)
        return;

    //(void)n;
    // assert(n == 1);

    if (index == kAdded)
    {
        // update(XEPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

bool SelectPoller::update(int operation, Channel *channel)
{
    int fd = channel->fd();
    if (operation == XEPOLL_CTL_ADD)
    {
        struct epoll_event event;
        memset(&event, 0, sizeof event);
        event.events = channel->events();
        event.data.ptr = channel;

        m_events.push_back(std::move(event));
        return true;
    }

    if (operation == XEPOLL_CTL_DEL)
    {
        for (auto iter = m_events.begin(); iter != m_events.end(); ++iter)
        {
            if (iter->data.ptr == channel)
            {
                m_events.erase(iter);
                return true;
            }
        }
    }
    else if (operation == XEPOLL_CTL_MOD)
    {
        for (auto iter = m_events.begin(); iter != m_events.end(); ++iter)
        {
            if (iter->data.ptr == channel)
            {
                iter->events = channel->events();
                return true;
            }
        }
    }

    std::ostringstream os;
    os << "SelectPoller update fd failed, op = " << operation << ", fd = " << fd;
    os << ", events_ content: \n";
    for (const auto &iter : m_events)
    {
        os << "fd: " << iter.data.fd << ", Channel: 0x%x: " << iter.data.ptr << ", events: " << iter.events << "\n";
    }
    LOGE("%s", os.str().c_str());

    return false;
}
