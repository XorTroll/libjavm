
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

namespace javm::native::impl::sun::misc {

    using namespace vm;

    class VM {
        public:
            static ExecutionResult initialize(const std::vector<Ptr<Variable>> &param_vars) {
                JAVM_LOG("[sun.misc.VM.initialize] called");
                return ExecutionResult::Void();
            }
    };

}