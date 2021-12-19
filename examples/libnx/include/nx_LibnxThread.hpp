
#pragma once
#include <javm/native/native_NativeThread.hpp>
#include <switch.h>

// Threading implementation

#define R_TRY(rc) { \
    const auto _tmp_rc = (rc); \
    if(R_FAILED(_tmp_rc)) { \
        diagAbortWithResult(_tmp_rc); \
    } \
}

namespace nx {

    class LibnxThread : public native::Thread {
        private:
            bool existing;
            ::Thread thread;

            inline constexpr vm::type::Long GetHandleImpl() {
                return static_cast<vm::type::Long>(thread.handle);
            }

        public:
            static constexpr size_t StackSize = 0x8000;
            static constexpr int Priority = 0x2B;
            static constexpr int CpuId = -2;

            LibnxThread() : existing(false) {}

            LibnxThread(native::ThreadHandle handle) : existing(true) {
                this->thread = {};
                this->thread.handle = (Handle)handle;
            }

            virtual void Start(native::ThreadEntrypoint entry_fn) override {
                if(existing) {
                    return;
                }
                R_TRY(threadCreate(&this->thread, entry_fn, reinterpret_cast<void*>(this), nullptr, StackSize, Priority, CpuId));
                R_TRY(threadStart(&this->thread));
            }

            virtual native::ThreadHandle GetHandle() override {
                return this->GetHandleImpl();
            }

            virtual bool IsAlive() override {
                // If the thread has ended, its handle should no longer be valid, making this SVC fail
                s32 tmp_prio = 0;
                const auto rc = svcGetThreadPriority(&tmp_prio, this->thread.handle);
                return R_SUCCEEDED(rc);
            }
    };

}

namespace javm::native {

    ThreadHandle GetCurrentThreadHandle() {
        return static_cast<ThreadHandle>(threadGetCurHandle());
    }

    Ptr<Thread> CreateThread() {
        return ptr::New<nx::LibnxThread>();
    }

    Ptr<Thread> CreateExistingThread(const ThreadHandle handle) {
        return ptr::New<nx::LibnxThread>(handle);
    }

    Priority GetThreadPriority(const ThreadHandle handle) {
        // Stub
        return Thread::DefaultPriority;
    }

    void SetThreadPriority(const ThreadHandle handle, const Priority prio) {
        // Stub
    }

}