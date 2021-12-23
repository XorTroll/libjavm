
#pragma once
#include <javm/native/native_NativeSync.hpp>
#include <mutex>
#include <condition_variable>

// Sync implementation - standard C++

namespace javm::extras {

    class CppRecursiveMutex : public native::RecursiveMutex {
        private:
            std::recursive_mutex cpp_mutex;

        public:
            virtual void Lock() override {
                this->cpp_mutex.lock();
            }

            virtual bool TryLock() override {
                return this->cpp_mutex.try_lock();
            }

            virtual void Unlock() override {
                this->cpp_mutex.unlock();
            }

            inline std::recursive_mutex &GetCppHandle() {
                return this->cpp_mutex;
            }
    };

    class CppConditionVariable : public native::ConditionVariable {
        private:
            std::condition_variable_any cpp_cv;

        public:
            virtual void Wait(Ptr<native::RecursiveMutex> lock) override {
                auto cpp_lock = ptr::CastTo<CppRecursiveMutex>(lock);
                this->cpp_cv.wait(cpp_lock->GetCppHandle());
            }

            virtual void WaitFor(Ptr<native::RecursiveMutex> lock, const vm::type::Long ms) override {
                auto cpp_lock = ptr::CastTo<CppRecursiveMutex>(lock);
                this->cpp_cv.wait_for(cpp_lock->GetCppHandle(), std::chrono::milliseconds(ms));
            }

            virtual void Notify() override {
                this->cpp_cv.notify_one();
            }

            virtual void NotifyAll() override {
                this->cpp_cv.notify_all();
            }
    };

}

namespace javm::native {

    Ptr<RecursiveMutex> CreateRecursiveMutex() {
        return ptr::New<extras::CppRecursiveMutex>();
    }

    Ptr<ConditionVariable> CreateConditionVariable() {
        return ptr::New<extras::CppConditionVariable>();
    }

}