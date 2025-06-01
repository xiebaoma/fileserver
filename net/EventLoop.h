/*
xiebaoma
2025-06-01
*/

#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "../base/Timestamp.h"
#include "../base/Platform.h"
#include "Callbacks.h"
#include "Sockets.h"
#include "TimerId.h"
#include "TimerQueue.h"

namespace net
{
    class EventLoop;
    class Channel;
    class Poller;
    // class TimerQueue;
    class CTimerHeap;

    ///
    /// Reactor, at most one per thread.
    ///
    /// This is an interface class, so don't expose too much details.
    class EventLoop
    {
    public:
        /// Function type for callbacks
        typedef std::function<void()> Functor;

        /// Constructor
        EventLoop();
        
        /// Destructor - force out-line dtor, for scoped_ptr members.
        ~EventLoop();

        ///
        /// Loops forever.
        ///
        /// Must be called in the same thread as creation of the object.
        ///
        void loop();

        /// Quits loop.
        ///
        /// This is not 100% thread safe, if you call through a raw pointer,
        /// better to call through shared_ptr<EventLoop> for 100% safety.
        void quit();

        ///
        /// Time when poll returns, usually means data arrival.
        ///
        Timestamp pollReturnTime() const { return m_pollReturnTime; }

        /// Returns the current iteration count of the event loop
        int64_t iteration() const { return m_iteration; }

        /// Runs callback immediately in the loop thread.
        /// It wakes up the loop, and run the cb.
        /// If in the same loop thread, cb is run within the function.
        /// Safe to call from other threads.
        void runInLoop(const Functor &cb);
        
        /// Queues callback in the loop thread.
        /// Runs after finish pooling.
        /// Safe to call from other threads.
        void queueInLoop(const Functor &cb);

        // timers，时间单位均是微秒
        ///
        /// Runs callback at 'time'.
        /// Safe to call from other threads.
        ///
        TimerId runAt(const Timestamp &time, const TimerCallback &cb);
        
        ///
        /// Runs callback after @c delay seconds.
        /// Safe to call from other threads.
        ///
        TimerId runAfter(int64_t delay, const TimerCallback &cb);
        
        ///
        /// Runs callback every @c interval seconds.
        /// Safe to call from other threads.
        ///
        TimerId runEvery(int64_t interval, const TimerCallback &cb);
        
        ///
        /// Cancels the timer.
        /// Safe to call from other threads.
        ///
        void cancel(TimerId timerId, bool off);

        /// Removes the timer completely
        void remove(TimerId timerId);

        /// Runs callback at specified time (rvalue reference version)
        TimerId runAt(const Timestamp &time, TimerCallback &&cb);
        
        /// Runs callback after delay (rvalue reference version)
        TimerId runAfter(int64_t delay, TimerCallback &&cb);
        
        /// Runs callback every interval (rvalue reference version)
        TimerId runEvery(int64_t interval, TimerCallback &&cb);

        /// Sets a function to be called on each frame/iteration
        void setFrameFunctor(const Functor &cb);

        // internal usage
        /// Updates the channel in the poller
        bool updateChannel(Channel *channel);
        
        /// Removes the channel from the poller
        void removeChannel(Channel *channel);
        
        /// Checks if the channel exists in the poller
        bool hasChannel(Channel *channel);

        /// Asserts that the current thread is the loop thread
        void assertInLoopThread()
        {
            if (!isInLoopThread())
            {
                abortNotInLoopThread();
            }
        }
        
        /// Checks if the current thread is the loop thread
        bool isInLoopThread() const { return m_threadId == std::this_thread::get_id(); }
        
        /// Checks if event handling is in progress
        bool eventHandling() const { return m_eventHandling; }

        /// Returns the thread ID of this event loop
        const std::thread::id getThreadID() const
        {
            return m_threadId;
        }

    private:
        /// Creates the wakeup file descriptor
        bool createWakeupfd();
        
        /// Wakes up the event loop
        bool wakeup();
        
        /// Aborts when a function is called from a non-loop thread
        void abortNotInLoopThread();
        
        /// Handles read events on the wakeup file descriptor
        bool handleRead();
        
        /// Processes pending functors
        void doOtherTasks();

        /// Prints active channels for debugging
        void printActiveChannels() const;

    private:
        typedef std::vector<Channel *> ChannelList;

        bool m_looping;                          // Indicates if the loop is running
        bool m_quit;                             // Indicates if the loop should quit
        bool m_eventHandling;                    // Indicates if event handling is in progress
        bool m_doingOtherTasks;                  // Indicates if processing pending functors
        const std::thread::id m_threadId;        // Thread ID of the event loop
        Timestamp m_pollReturnTime;              // Time when poll returns
        std::unique_ptr<Poller> m_poller;        // IO multiplexing
        std::unique_ptr<TimerQueue> m_timerQueue; // Timer management
        int64_t m_iteration;                     // Loop iteration count
#ifdef WIN32
        SOCKET m_wakeupFdSend;                   // Socket for sending wakeup signals
        SOCKET m_wakeupFdListen;                 // Socket for listening wakeup signals
        SOCKET m_wakeupFdRecv;                   // Socket for receiving wakeup signals
#else
        SOCKET m_wakeupFd;                       // File descriptor for wakeup
#endif
        std::unique_ptr<Channel> m_wakeupChannel; // Channel for wakeup events

        // scratch variables
        ChannelList m_activeChannels;            // Currently active channels
        Channel *currentActiveChannel_;          // Current channel being processed

        std::mutex m_mutex;                      // Mutex for thread safety
        std::vector<Functor> m_pendingFunctors;  // Functors to be run in the loop thread

        Functor m_frameFunctor;                  // Function called on each loop iteration
    };

}
