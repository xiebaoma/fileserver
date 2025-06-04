/*
 * Author: xiebaoma
 * Date: 2025-06-04
 *
 * Description:
 * This header defines the TimerQueue class within the net namespace.
 * TimerQueue manages multiple timers in a thread-safe way, allowing
 * users to schedule timed callbacks (single-shot or repeated).
 * It is typically used in asynchronous network programming.
 */

#pragma once

#include <set>
#include <vector>

#include "../base/Timestamp.h"
#include "../net/Callbacks.h"
#include "../net/Channel.h"

namespace net
{
    class EventLoop;
    class Timer;
    class TimerId;

    /**
     * @brief A queue to manage and dispatch timers based on expiration timestamps.
     *
     * Timers can be one-shot or periodic. This class integrates with EventLoop,
     * and the doTimer() function should be triggered when the timerfd fires.
     */
    class TimerQueue
    {
    public:
        /**
         * @brief Constructs the TimerQueue associated with the given EventLoop.
         * @param loop Pointer to the EventLoop this TimerQueue belongs to.
         */
        TimerQueue(EventLoop *loop);

        /**
         * @brief Destructor: cleans up all active timers.
         */
        ~TimerQueue();

        /**
         * @brief Adds a new timer.
         * @param cb Callback to invoke when the timer expires.
         * @param when The absolute time when the timer should fire.
         * @param interval Interval in microseconds for repeating (0 for one-shot).
         * @param repeatCount How many times the timer should repeat (-1 for infinite).
         * @return A TimerId used to manage (e.g. cancel) the timer.
         */
        TimerId addTimer(const TimerCallback &cb, Timestamp when, int64_t interval, int64_t repeatCount);

        /**
         * @brief Overload that accepts an rvalue reference to the callback.
         */
        TimerId addTimer(TimerCallback &&cb, Timestamp when, int64_t interval, int64_t repeatCount);

        /**
         * @brief Removes a timer from the queue.
         * @param timerId The ID of the timer to remove.
         */
        void removeTimer(TimerId timerId);

        /**
         * @brief Cancels a timer. If off is true, disables it immediately.
         * @param timerId The ID of the timer to cancel.
         * @param off Whether to immediately disable the timer (true) or wait for it to expire.
         */
        void cancel(TimerId timerId, bool off);

        /**
         * @brief Called when the underlying timerfd triggers.
         * This function executes expired timer callbacks and resets repeating timers.
         */
        void doTimer();

    private:
        // Disable copy and assignment
        TimerQueue(const TimerQueue &rhs) = delete;
        TimerQueue &operator=(const TimerQueue &rhs) = delete;

        // Internal types for managing timers
        typedef std::pair<Timestamp, Timer *> Entry;
        typedef std::set<Entry> TimerList;
        typedef std::pair<Timer *, int64_t> ActiveTimer;
        typedef std::set<ActiveTimer> ActiveTimerSet;

        // Adds a timer inside the event loop thread context.
        void addTimerInLoop(Timer *timer);

        // Removes a timer inside the event loop thread context.
        void removeTimerInLoop(TimerId timerId);

        // Cancels a timer inside the event loop thread context.
        void cancelTimerInLoop(TimerId timerId, bool off);

        // Inserts a timer into the sorted timer list.
        void insert(Timer *timer);

    private:
        EventLoop *m_loop;  // The event loop that owns this timer queue.
        TimerList m_timers; // Ordered set of all active timers.
    };

}
