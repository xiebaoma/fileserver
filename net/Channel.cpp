/*
 * Author: xiebaoma
 * Date: 2025-06-01
 *
 * Description:
 * implementation of Channel class, which is part of the Reactor pattern.
 */

#include "Channel.h"
#include <sstream>

#include "../base/Platform.h"
#include "../base/AsyncLog.h"
#include "Poller.h"
#include "EventLoop.h"

using namespace net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = XPOLLIN | XPOLLPRI;
const int Channel::kWriteEvent = XPOLLOUT;

Channel::Channel(EventLoop *loop, int fd__) : m_loop(loop),
                                              m_fd(fd__),
                                              m_events(0),
                                              m_revents(0),
                                              m_index(-1)
{
}

Channel::~Channel()
{
}

bool Channel::enableReading()
{
    m_events |= kReadEvent;
    return update();
}

bool Channel::disableReading()
{
    m_events &= ~kReadEvent;

    return update();
}

bool Channel::enableWriting()
{
    m_events |= kWriteEvent;

    return update();
}

bool Channel::disableWriting()
{
    m_events &= ~kWriteEvent;
    return update();
}

bool Channel::disableAll()
{
    m_events = kNoneEvent;
    return update();
}

bool Channel::update()
{
    // addedToLoop_ = true;
    return m_loop->updateChannel(this);
}

void Channel::remove()
{
    if (!isNoneEvent())
        return;

    m_loop->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{

    LOGD(reventsToString().c_str());
    if ((m_revents & XPOLLHUP) && !(m_revents & XPOLLIN))
    {
        if (m_closeCallback)
            m_closeCallback();
    }

    if (m_revents & XPOLLNVAL)
    {
        LOGW("Channel::handle_event() XPOLLNVAL");
    }

    if (m_revents & (XPOLLERR | XPOLLNVAL))
    {
        if (m_errorCallback)
            m_errorCallback();
    }

    if (m_revents & (XPOLLIN | XPOLLPRI | XPOLLRDHUP))
    {
        if (m_readCallback)
            m_readCallback(receiveTime);
    }

    if (m_revents & XPOLLOUT)
    {
        if (m_writeCallback)
            m_writeCallback();
    }
}

string Channel::reventsToString() const
{
    std::ostringstream oss;
    oss << m_fd << ": ";
    if (m_revents & XPOLLIN)
        oss << "IN ";
    if (m_revents & XPOLLPRI)
        oss << "PRI ";
    if (m_revents & XPOLLOUT)
        oss << "OUT ";
    if (m_revents & XPOLLHUP)
        oss << "HUP ";
    if (m_revents & XPOLLRDHUP)
        oss << "RDHUP ";
    if (m_revents & XPOLLERR)
        oss << "ERR ";
    if (m_revents & XPOLLNVAL)
        oss << "NVAL ";

    return oss.str().c_str();
}
