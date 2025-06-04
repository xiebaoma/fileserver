/*
xiebaoma
2025-06-04
*/

#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <functional>

namespace net
{
    class EventLoop;

    /**
     * @brief A class that manages a single EventLoop running in its own dedicated thread.
     *
     * This class encapsulates thread creation, synchronization, and EventLoop lifecycle management.
     * It is typically used within a thread pool or standalone when a background event loop is needed.
     */
    class EventLoopThread
    {
    public:
        // Callback type invoked when the thread starts and EventLoop is ready.
        typedef std::function<void(EventLoop *)> ThreadInitCallback;

        /**
         * @brief Constructs the EventLoopThread with an optional initialization callback and name.
         * @param cb Initialization callback that will be called with the EventLoop pointer after setup.
         * @param name The name of the thread (optional, for logging/debugging).
         */
        EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), const std::string &name = "");

        /**
         * @brief Destructor: stops the thread and cleans up resources.
         */
        ~EventLoopThread();

        /**
         * @brief Starts the thread and the associated EventLoop.
         * Blocks until the EventLoop is initialized and ready.
         * @return Pointer to the initialized EventLoop running in the new thread.
         */
        EventLoop *startLoop();

        /**
         * @brief Signals the thread to exit and stops the EventLoop.
         */
        void stopLoop();

    private:
        // The thread entry function. Initializes the EventLoop and runs its loop().
        void threadFunc();

        EventLoop *m_loop;                     // Pointer to the EventLoop managed by this thread.
        bool m_exiting;                        // Flag indicating if the thread is exiting.
        std::unique_ptr<std::thread> m_thread; // Thread object running the EventLoop.
        std::mutex m_mutex;                    // Mutex for synchronizing loop startup.
        std::condition_variable m_cond;        // Condition variable to wait for loop initialization.
        ThreadInitCallback m_callback;         // Optional callback run during thread initialization.
    };

}
