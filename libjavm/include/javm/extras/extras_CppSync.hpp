
#pragma once
#include <javm/native/native_NativeSync.hpp>
#include <mutex>
#include <condition_variable>

// Sync implementation - standard C++

namespace javm::extras {

    class CppRecursiveMutex : public native::RecursiveMutex {

        private:
            std::recursive_mutex cpp_mtx;

        public:
            virtual void Lock() override {
                this->cpp_mtx.lock();
            }

            virtual bool TryLock() override {
                return this->cpp_mtx.try_lock();
            }

            virtual void Unlock() override {
                this->cpp_mtx.unlock();
            }

            std::recursive_mutex &GetCppHandle() {
                return this->cpp_mtx;
            }

    };

    class CppConditionVariable : public native::ConditionVariable {

        private:
            std::condition_variable_any cpp_condv;

        public:
            // In the Wait calls, we do know that the recursive mutex type is the C++ one, so we directly cast to it

            virtual void Wait(Ptr<native::RecursiveMutex> lock) override {
                auto cpp_lock = PtrUtils::CastTo<CppRecursiveMutex>(lock);
                this->cpp_condv.wait(cpp_lock->GetCppHandle());
            }

            virtual void WaitFor(Ptr<native::RecursiveMutex> lock, vm::type::Long ms) override {
                auto cpp_lock = PtrUtils::CastTo<CppRecursiveMutex>(lock);
                this->cpp_condv.wait_for(cpp_lock->GetCppHandle(), std::chrono::milliseconds(ms));
            }

            virtual void Notify() override {
                this->cpp_condv.notify_one();
            }

            virtual void NotifyAll() override {
                this->cpp_condv.notify_all();
            }

    };

}

namespace javm::native {

    Ptr<RecursiveMutex> CreateRecursiveMutex() {
        return PtrUtils::New<extras::CppRecursiveMutex>();
    }

    Ptr<ConditionVariable> CreateConditionVariable() {
        return PtrUtils::New<extras::CppConditionVariable>();
    }

}