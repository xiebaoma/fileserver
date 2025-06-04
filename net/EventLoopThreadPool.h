/*
 * Author: xiebaoma
 * Date: 2025-06-04
 *
 * Description:
 * This header defines the EventLoopThreadPool class in the net namespace.
 * It manages a pool of EventLoop threads, allowing for distribution of tasks
 * across multiple threads in a scalable and efficient way.
 * Commonly used in multithreaded networking servers.
 */

#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <string>

namespace net
{
    class EventLoop;
    class EventLoopThread;

    class EventLoopThreadPool
    {
    public:
        // Callback type invoked when a thread is initialized.
        typedef std::function<void(EventLoop *)> ThreadInitCallback;

        // Constructor: creates an uninitialized thread pool.
        EventLoopThreadPool();

        // Destructor: ensures proper cleanup of threads and loops.
        ~EventLoopThreadPool();

        // Initializes the thread pool with the given base loop and number of threads.
        void init(EventLoop *baseLoop, int numThreads);

        // Starts all threads and their associated event loops.
        // Optional initialization callback can be provided.
        void start(const ThreadInitCallback &cb = ThreadInitCallback());

        // Stops all event loops and threads.
        void stop();

        // Returns the next EventLoop using round-robin scheduling.
        EventLoop *getNextLoop();

        // Returns an EventLoop based on a hash code.
        // Ensures that the same hash always maps to the same loop.
        EventLoop *getLoopForHash(size_t hashCode);

        // Returns all EventLoops managed by this thread pool.
        std::vector<EventLoop *> getAllLoops();

        // Returns true if the thread pool has been started.
        bool started() const
        {
            return m_started;
        }

        // Returns the name of the thread pool.
        const std::string &name() const
        {
            return m_name;
        }

        // Returns a string containing runtime information about the thread pool.
        const std::string info() const;

    private:
        EventLoop *m_baseLoop;                                   // The main thread's EventLoop (not owned).
        std::string m_name;                                      // Name identifier for the thread pool.
        bool m_started;                                          // Flag indicating whether the pool has been started.
        int m_numThreads;                                        // Number of worker threads.
        int m_next;                                              // Index for round-robin scheduling.
        std::vector<std::unique_ptr<EventLoopThread>> m_threads; // Owns the EventLoopThread objects.
        std::vector<EventLoop *> m_loops;                        // Raw pointers to each thread's EventLoop.
    };
}
