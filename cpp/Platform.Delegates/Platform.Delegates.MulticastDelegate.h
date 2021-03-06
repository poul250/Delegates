#pragma once

#ifndef PLATFORM_DELEGATES_MULTICASTDELEGATE
#define PLATFORM_DELEGATES_MULTICASTDELEGATE

// Based on https://stackoverflow.com/a/23974539/710069 

#include <mutex>
#include "Platform.Delegates.Delegate.h"

namespace Platform::Delegates
{
    template <typename...>
    class MulticastDelegate;

    template <typename ReturnType, typename... Args>
    class MulticastDelegate<ReturnType(Args...)> : public Delegate<ReturnType(Args...)>
    {
        using DelegateRawFunctionType = ReturnType(Args...);
        using DelegateType = Delegate<DelegateRawFunctionType>;
        using DelegateFunctionType = std::function<DelegateRawFunctionType>;

        mutable std::mutex mutex;
        std::vector<DelegateType> callbacks;

        void MoveCallbacksUnsync(MulticastDelegate &other)
        {
            callbacks = std::move(other.callbacks);
        }

        void CopyCallbacksUnsync(const MulticastDelegate &other)
        {
            callbacks = other.callbacks;
        }

        void CopyCallbacks(const MulticastDelegate &other)
        {
            // To prevent the deadlock the order of locking should be the same for all threads.
            // We can use the addresses of MulticastDelegates to sort the mutexes. 
            if (this < &other)
            {
                std::lock_guard lock1(mutex);
                std::lock_guard lock2(other.mutex);
                CopyCallbacksUnsync(other);
            }
            else
            {
                std::lock_guard lock1(other.mutex);
                std::lock_guard lock2(mutex);
                CopyCallbacksUnsync(other);
            }
        }

        void ClearCallbacks()
        {
            std::lock_guard lock(mutex);
            callbacks.clear();
        }

    public:
        MulticastDelegate() {}

        MulticastDelegate(const DelegateType &callback)
        {
            *this += callback;
        }

        MulticastDelegate(DelegateRawFunctionType &callback)
        {
            *this += callback;
        }

        MulticastDelegate(const DelegateFunctionType &callback)
        {
            *this += callback;
        }

        MulticastDelegate(const MulticastDelegate &multicastDelegate)
        {
            CopyCallbacks(multicastDelegate);
        }

        MulticastDelegate(const MulticastDelegate &&multicastDelegate)
        {
            MoveCallbacksUnsync(multicastDelegate);
        }

        MulticastDelegate &operator=(const MulticastDelegate &other) noexcept
        {
            if (this != &other)
            {
                CopyCallbacks(other);
            }
            return *this;
        }

        MulticastDelegate &operator=(MulticastDelegate &&other) noexcept
        {
            if (this != &other)
            {
                MoveCallbacksUnsync(other);
            }
            return *this;
        }

        MulticastDelegate &operator=(const DelegateType &other) noexcept
        {
            ClearCallbacks();
            *this += other;
            return *this;
        }

        MulticastDelegate &operator=(DelegateRawFunctionType &other) noexcept
        {
            ClearCallbacks();
            *this += other;
            return *this;
        }

        MulticastDelegate &operator=(const DelegateFunctionType &other) noexcept
        {
            ClearCallbacks();
            *this += other;
            return *this;
        }

        MulticastDelegate &operator+=(const DelegateType &callback)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            this->callbacks.emplace_back(callback);
            return *this;
        }

        MulticastDelegate &operator+=(DelegateRawFunctionType &callback)
        {
            return *this += DelegateType(callback);
        }

        MulticastDelegate &operator+=(const DelegateFunctionType &callback)
        {
            return *this += DelegateType(callback);
        }

        MulticastDelegate &operator-=(const DelegateType &callback)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            auto searchResult = std::find(this->callbacks.rbegin(), this->callbacks.rend(), callback);
            if (searchResult != this->callbacks.rend()) {
                auto removedCallbackPosition = --(searchResult.base());
                this->callbacks.erase(removedCallbackPosition);
            }
            return *this;
        }

        MulticastDelegate &operator-=(DelegateRawFunctionType &callback)
        {
            return *this -= DelegateType(callback);
        }

        MulticastDelegate &operator-=(const DelegateFunctionType &callback)
        {
            return *this -= DelegateType(callback);
        }

        virtual ReturnType operator()(Args... args) override
        {
            const std::lock_guard<std::mutex> lock(mutex);
            auto callbacksSize = this->callbacks.size();
            if (callbacksSize == 0)
            {
                throw std::bad_function_call();
            }
            auto lastElement = callbacksSize - 1;
            for (size_t i = 0; i < lastElement; i++)
            {
                this->callbacks[i](std::forward<Args>(args)...);
            }
            return this->callbacks[lastElement](std::forward<Args>(args)...);
        }
    };
}

#endif
