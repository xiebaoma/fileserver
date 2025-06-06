#include "CountDownLatch.h"

/**
 * @brief Constructs the CountDownLatch with a specified initial count.
 *
 * @param count Initial count value.
 */
CountDownLatch::CountDownLatch(int count)
    : count_(count)
{
}

/**
 * @brief Blocks the calling thread until count_ reaches zero.
 *
 * Uses a condition variable to wait efficiently.
 */
void CountDownLatch::wait()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while (count_ > 0)
    {
        condition_.wait(lock);
    }
}

/**
 * @brief Decrements the count.
 *
 * If count reaches zero, notifies all waiting threads.
 */
void CountDownLatch::countDown()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    --count_;
    if (count_ == 0)
    {
        condition_.notify_all();
    }
}

/**
 * @brief Gets the current value of the count.
 *
 * @return The number of remaining countDown() calls required.
 */
int CountDownLatch::getCount() const
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return count_;
}
