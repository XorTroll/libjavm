
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

namespace javm::native::impl::java::lang {

    using namespace vm;

    class String {
        public:
            static ExecutionResult intern(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
                JAVM_LOG("[java.lang.String.intern] called - string: '%s'", str::ToUtf8(jstr::GetValue(this_var)).c_str());
                return ExecutionResult::ReturnVariable(jstr::CheckIntern(this_var));
            }
    };

}