#pragma once
#ifndef PLATFORM_DELEGATES_MULTICASTDELEGATE
#define PLATFORM_DELEGATES_MULTICASTDELEGATE

// Based on https://stackoverflow.com/a/23974539/710069 and https://stackoverflow.com/a/35920804/710069

#include <algorithm>
#include <iostream>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include "Delegate.h"

namespace Platform::Delegates
{
    template <typename...>
    class MulticastDelegate;

    template <typename ReturnType, typename... Args>
    class MulticastDelegate<ReturnType(Args...)> : public Delegate<ReturnType(Args...)>
    {
        using DelegateRawFunctionType = ReturnType(Args...);
        using DelegateType = Delegate<ReturnType(Args...)>;
        using DelegateFunctionType = std::function<ReturnType(Args...)>;

        std::vector<DelegateType> callbacks;
        std::mutex mutex;

    public:
        MulticastDelegate() {}

        MulticastDelegate(const DelegateType& callback)
        {
            this += callback;
        }

        MulticastDelegate(const MulticastDelegate&) = delete;

        void operator=(const MulticastDelegate&) = delete;

        MulticastDelegate<ReturnType(Args...)>& operator+= (const DelegateType&& callback)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            this->callbacks.emplace_back(callback);
            return *this;
        }

        MulticastDelegate<ReturnType(Args...)>& operator+= (DelegateRawFunctionType&& callback)
        {
            return *this += (DelegateType)callback;
        }

        MulticastDelegate<ReturnType(Args...)>& operator+= (DelegateFunctionType&& callback)
        {
            //return *this += (DelegateType)callback;
            return *this;
        }

        MulticastDelegate<ReturnType(Args...)>& operator-= (DelegateType&& callback)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            auto searchResult = std::find_if(this->callbacks.rbegin(), this->callbacks.rend(),
                [&](DelegateType& other)
                {
                    return callback == other;
                });
            if (searchResult != this->callbacks.rend()) {
                auto removedCallbackPosition = --(searchResult.base());
                this->callbacks.erase(removedCallbackPosition);
            }
            return *this;
        }

        MulticastDelegate<ReturnType(Args...)>& operator-= (DelegateRawFunctionType&& callback)
        {
            return *this -= (DelegateType)callback;
        }

        MulticastDelegate<ReturnType(Args...)>& operator-= (DelegateFunctionType&& callback)
        {
            //return *this -= DelegateType(callback);
            return *this;
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
                this->callbacks[i](args...);
            }
            return this->callbacks[lastElement](args...);
        }
    };
}

#endif