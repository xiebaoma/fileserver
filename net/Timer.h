/*
 * Author: xiebaoma
 * Date: 2025-06-04
 *
 * Description:
 * This header defines the Timer class in the net namespace.
 * A Timer represents a single timed task with optional repetition.
 * It holds the callback function, expiration time, interval, and repeat count.
 * Used internally by TimerQueue to manage timing events.
 */

#pragma once

#include <atomic>
#include <stdint.h>
#include "../base/Timestamp.h"
#include "../net/Callbacks.h"

namespace net
{
    /**
     * @brief Represents a timer with optional repeat behavior.
     *
     * A Timer encapsulates a user-defined callback, an expiration timestamp,
     * a repeat interval (in microseconds), and a repeat count.
     * It can be canceled and tracked using a unique sequence number.
     */
    class Timer
    {
    public:
        /**
         * @brief Constructs a Timer with a callback, expiration time, interval, and repeat count.
         * @param cb The callback to be executed when the timer expires.
         * @param when The expiration timestamp.
         * @param interval Repeat interval in microseconds (0 means one-shot).
         * @param repeatCount Number of times to repeat (-1 means infinite).
         */
        Timer(const TimerCallback &cb, Timestamp when, int64_t interval, int64_t repeatCount = -1);

        /**
         * @brief Move-constructor version of Timer.
         */
        Timer(TimerCallback &&cb, Timestamp when, int64_t interval);

        /**
         * @brief Executes the stored callback function.
         */
        void run();

        /**
         * @brief Returns whether the timer has been canceled.
         */
        bool isCanceled() const { return m_canceled; }

        /**
         * @brief Cancels the timer.
         * @param off If true, sets the timer to canceled state.
         */
        void cancel(bool off) { m_canceled = off; }

        /**
         * @brief Returns the expiration timestamp.
         */
        Timestamp expiration() const { return m_expiration; }

        /**
         * @brief Returns the remaining repeat count.
         */
        int64_t getRepeatCount() const { return m_repeatCount; }

        /**
         * @brief Returns the unique sequence number of the timer.
         */
        int64_t sequence() const { return m_sequence; }

        /**
         * @brief Returns the total number of Timer instances created.
         */
        static int64_t numCreated() { return s_numCreated; }

    private:
        // Disable copy construction and assignment.
        Timer(const Timer &rhs) = delete;
        Timer &operator=(const Timer &rhs) = delete;

    private:
        const TimerCallback m_callback; // User-defined callback function.
        Timestamp m_expiration;         // Absolute time at which timer should trigger.
        const int64_t m_interval;       // Interval for repeating timer in microseconds.
        int64_t m_repeatCount;          // Remaining times to repeat (-1 means infinite).
        const int64_t m_sequence;       // Unique sequence number for identification.
        bool m_canceled;                // Whether the timer has been canceled.

        static std::atomic<int64_t> s_numCreated; // Total number of Timer objects created.
    };
}
