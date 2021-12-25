
#pragma once
#include <javm/native/native_NativeCode.hpp>
#include <javm/native/impl/impl_Base.hpp>
#include <javm/vm/vm_JavaUtils.hpp>
#include <javm/vm/vm_Thread.hpp>
#include <javm/vm/vm_Properties.hpp>
#include <javm/vm/vm_Reflection.hpp>
#include <unistd.h>
#include <csignal>
#include <sys/time.h>

namespace javm::native::impl::java::io {

    using namespace vm;

    class FileInputStream {
        public:
            static ExecutionResult initIDs(const std::vector<Ptr<Variable>> &param_vars) {
                JAVM_LOG("[java.io.FileInputStream.initIDs] called");
                return ExecutionResult::Void();
            }
    };

}