#pragma once

#include <mutex>
#include <condition_variable>

/**
 * @brief A thread synchronization aid that allows one or more threads to wait until
 *        a set of operations being performed in other threads completes.
 *
 * Usage:
 * - Initialize with a given count.
 * - Threads call wait() to block until the count reaches zero.
 * - Other threads call countDown() to decrement the count.
 * - When the count reaches zero, all waiting threads are notified.
 *
 * Thread-safe.
 */
class CountDownLatch
{
public:
    /**
     * @brief Construct a new CountDownLatch with an initial count.
     *
     * @param count The number of times countDown() must be called before wait() returns.
     */
    explicit CountDownLatch(int count);

    /**
     * @brief Blocks the calling thread until the count reaches zero.
     */
    void wait();

    /**
     * @brief Decrements the count. If the count reaches zero, unblocks all waiting threads.
     */
    void countDown();

    /**
     * @brief Returns the current count value (primarily for inspection/debugging).
     *
     * @return Current count value.
     */
    int getCount() const;

private:
    mutable std::mutex m_mutex;         ///< Mutex to protect access to shared state
    std::condition_variable condition_; ///< Condition variable for blocking/waiting
    int count_;                         ///< Number of remaining countDown() calls needed
};
