
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

namespace javm::native::impl::java::security {

    using namespace vm;

    class AccessController {
        public:
            static ExecutionResult doPrivileged(const std::vector<Ptr<Variable>> &param_vars) {
                auto action_v = param_vars[0];
                JAVM_LOG("[java.security.AccessController.doPrivileged] called - action type: '%s'", str::ToUtf8(TypeUtils::FormatVariableType(action_v)).c_str());
                auto action_obj = action_v->GetAs<type::ClassInstance>();
                auto ret = action_obj->CallInstanceMethod(u"run", u"()Ljava/lang/Object;", action_v);
                JAVM_LOG("Return type of privileged action: %d", static_cast<u32>(ret.status));
                return ret;
            }

            static ExecutionResult getStackAccessControlContext(const std::vector<Ptr<Variable>> &param_vars) {
                JAVM_LOG("[java.security.AccessController.getStackAccessControlContext] called");
                return ExecutionResult::ReturnVariable(TypeUtils::Null());
            }
    };

}