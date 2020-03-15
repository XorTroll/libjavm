
#pragma once
#include <javm/native/native_NativeSync.hpp>
#include <mutex>
#include <condition_variable>

// Sync implementation

namespace nx {

    class LibnxRecursiveMutex : public native::RecursiveMutex {

        private:
            RMutex mtx;

        public:
            LibnxRecursiveMutex() {
                rmutexInit(&this->mtx);
            }

            virtual void Lock() override {
                rmutexLock(&this->mtx);
            }

            virtual bool TryLock() override {
                return rmutexTryLock(&this->mtx);
            }

            virtual void Unlock() override {
                rmutexUnlock(&this->mtx);
            }

            RMutex &GetHandle() {
                return this->mtx;
            }

    };

    class LibnxConditionVariable : public native::ConditionVariable {

        private:
            CondVar condv;

        public:
            LibnxConditionVariable() {
                condvarInit(&this->condv);
            }

            virtual void Wait(Ptr<native::RecursiveMutex> lock) override {
                auto nx_lock = PtrUtils::CastTo<LibnxRecursiveMutex>(lock);
                condvarWait(&this->condv, &nx_lock->GetHandle().lock);
            }

            virtual void WaitFor(Ptr<native::RecursiveMutex> lock, vm::type::Long ms) override {
                auto nx_lock = PtrUtils::CastTo<LibnxRecursiveMutex>(lock);
                condvarWaitTimeout(&this->condv, &nx_lock->GetHandle().lock, ms);
            }

            virtual void Notify() override {
                condvarWakeOne(&this->condv);
            }

            virtual void NotifyAll() override {
                condvarWakeAll(&this->condv);
            }

    };

}

namespace javm::native {

    Ptr<RecursiveMutex> CreateRecursiveMutex() {
        return PtrUtils::New<nx::LibnxRecursiveMutex>();
    }

    Ptr<ConditionVariable> CreateConditionVariable() {
        return PtrUtils::New<nx::LibnxConditionVariable>();
    }

}