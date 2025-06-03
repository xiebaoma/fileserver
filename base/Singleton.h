/*
 * Author: xiebaoma
 * Date: 2025-06-03
 * Description: A thread-unsafe generic Singleton implementation template.
 * Provides global access to a single instance of a class type T.
 */

#pragma once

/**
 * @brief A generic Singleton template class.
 *
 * This class ensures that only one instance of type T is created and provides
 * global access to that instance. It is implemented using lazy initialization.
 *
 * Note: This version is not thread-safe. For thread safety, consider enabling
 * pthread_once (already commented for reference).
 */
template <typename T>
class Singleton
{
public:
    /**
     * @brief Returns a reference to the singleton instance.
     *
     * If the instance does not exist, it will be created on first access.
     *
     * @return Reference to the singleton instance of type T.
     */
    static T &Instance()
    {
        // Thread-safe initialization (not enabled in this version):
        // pthread_once(&ponce_, &Singleton::init);

        if (value_ == nullptr)
        {
            value_ = new T();
        }
        return *value_;
    }

private:
    // Constructor is private to prevent direct instantiation.
    Singleton();

    // Destructor is defaulted and private to restrict access.
    ~Singleton() = default;

    // Deleted copy constructor and assignment operator to enforce singleton.
    Singleton(const Singleton &) = delete;
    Singleton &operator=(const Singleton &) = delete;

    /**
     * @brief Initializes the singleton instance.
     *
     * This function is intended to be used with pthread_once for thread-safe
     * lazy initialization.
     */
    static void init()
    {
        value_ = new T();
        // Optional cleanup at program exit:
        // ::atexit(destroy);
    }

    /**
     * @brief Destroys the singleton instance.
     *
     * Can be registered with atexit() to ensure cleanup on program termination.
     */
    static void destroy()
    {
        delete value_;
    }

private:
    // Static pointer to the singleton instance.
    static T *value_;

    // Optional pthread_once variable for thread-safe initialization.
    // static pthread_once_t ponce_;
};

// Static member initialization
// template<typename T>
// pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

template <typename T>
T *Singleton<T>::value_ = nullptr;
