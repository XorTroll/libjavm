
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

namespace javm::native::impl::sun::reflect {

    using namespace vm;

    class Reflection {
        public:
            static ExecutionResult getCallerClass(const std::vector<Ptr<Variable>> &param_vars) {
                auto thr = ThreadUtils::GetCurrentThread();
                JAVM_LOG("[sun.reflect.Reflection.getCallerClass] called...");

                for(const auto &call_info: thr->GetInvertedCallStack()) {
                    // Find the currently invoked method, check if it is '@CallerSensitive'
                    bool found = false;
                    for(const auto &raw_inv: call_info.caller_type->GetRawInvokables()) {
                        if((raw_inv.GetName() == call_info.invokable_name) && (raw_inv.GetDescriptor() == call_info.invokable_desc)) {
                            found = !raw_inv.HasAnnotation(u"Lsun/reflect/CallerSensitive;");
                        }
                    }
                    if(found) {
                        // This one is a valid one
                        JAVM_LOG("[sun.reflect.Reflection.getCallerClass] called - caller class type: '%s'...", str::ToUtf8(call_info.caller_type->GetClassName()).c_str());

                        auto dummy_ref_type = ReflectionUtils::FindTypeByName(call_info.caller_type->GetClassName());
                        return ExecutionResult::ReturnVariable(TypeUtils::NewClassTypeVariable(dummy_ref_type));
                    }
                }

                return ExecutionResult::ReturnVariable(TypeUtils::Null());
            }

            static ExecutionResult getClassAccessFlags(const std::vector<Ptr<Variable>> &param_vars) {
                JAVM_LOG("[sun.reflect.Reflection.getClassAccessFlags] called");
                return GetClassModifiers(param_vars[0]);
            }
    };

}