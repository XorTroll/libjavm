
#pragma once
#include <javm/native/native_NativeThread.hpp>
#include <pthread.h>

// Threading implementation - pthread

namespace javm::extras {

    using PthreadEntry = void*(*)(void*);

    inline constexpr vm::type::Long CastFromPthread(pthread_t pthread) {
        return static_cast<vm::type::Long>(pthread);
    }

    inline constexpr pthread_t CastToPthread(vm::type::Long handle) {
        return static_cast<pthread_t>(handle);
    }

    class PthreadThread : public native::Thread {
        private:
            bool existing;
            pthread_t pthread;

        private:
            inline constexpr vm::type::Long GetHandleImpl() {
                return CastFromPthread(pthread);
            }

        public:
            PthreadThread() : existing(false) {}

            PthreadThread(native::ThreadHandle handle) : existing(true), pthread(CastToPthread(handle)) {}

            virtual void Start(native::ThreadEntrypoint entry_fn) override {
                if(existing) {
                    return;
                }
                pthread_create(&this->pthread, nullptr, reinterpret_cast<PthreadEntry>(entry_fn), reinterpret_cast<void*>(this));
            }

            virtual native::ThreadHandle GetHandle() override {
                return this->GetHandleImpl();
            }

            virtual bool IsAlive() override {
                const auto ret = pthread_kill(this->pthread, 0);
                return (ret == 0);
            }
    };

}

namespace javm::native {

    ThreadHandle GetCurrentThreadHandle() {
        auto self_thr = pthread_self();
        return extras::CastFromPthread(self_thr);
    }

    Ptr<Thread> CreateThread() {
        return ptr::New<extras::PthreadThread>();
    }

    Ptr<Thread> CreateExistingThread(const native::ThreadHandle handle) {
        return ptr::New<extras::PthreadThread>(handle);
    }

    Priority GetThreadPriority(const ThreadHandle handle) {
        // Stub
        return Thread::DefaultPriority;
    }

    void SetThreadPriority(const ThreadHandle handle, const Priority prio) {
        // Stub
    }

}