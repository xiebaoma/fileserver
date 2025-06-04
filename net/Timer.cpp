/*
 * Author: xiebaoma
 * Date: 2025-06-04
 *
 * Description:implementation of the Timer class, which represents a single timed task
 */

#include "Timer.h"

using namespace net;

std::atomic<int64_t> Timer::s_numCreated;

Timer::Timer(const TimerCallback &cb, Timestamp when, int64_t interval, int64_t repeatCount /* = -1*/)
    : m_callback(cb),
      m_expiration(when),
      m_interval(interval),
      m_repeatCount(repeatCount),
      m_sequence(++s_numCreated),
      m_canceled(false)
{
}

Timer::Timer(TimerCallback &&cb, Timestamp when, int64_t interval)
    : m_callback(std::move(cb)),
      m_expiration(when),
      m_interval(interval),
      m_repeatCount(-1),
      m_sequence(++s_numCreated),
      m_canceled(false)
{
}

void Timer::run()
{
    if (m_canceled)
        return;

    m_callback();

    if (m_repeatCount != -1)
    {
        --m_repeatCount;
        if (m_repeatCount == 0)
        {
            // repeatCount_ = 0;
            return;
        }
    }

    m_expiration += m_interval;
}