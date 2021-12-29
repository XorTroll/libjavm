#include <javm/javm_VM.hpp>
#include <javm/native/impl/sun/reflect/reflect_Reflection.hpp>
#include <javm/native/impl/impl_Base.hpp>

namespace javm::native::impl::sun::reflect {

    using namespace vm;

    ExecutionResult Reflection::getCallerClass(const std::vector<Ptr<Variable>> &param_vars) {
        auto accessor = GetCurrentThread();
        JAVM_LOG("[sun.reflect.Reflection.getCallerClass] called...");

        for(const auto &call_info: accessor->GetInvertedCallStack()) {
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

                auto dummy_ref_type = ref::FindReflectionTypeByName(call_info.caller_type->GetClassName());
                return ExecutionResult::ReturnVariable(NewClassTypeVariable(dummy_ref_type));
            }
        }

        return ExecutionResult::ReturnVariable(MakeNull());
    }

    ExecutionResult Reflection::getClassAccessFlags(const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[sun.reflect.Reflection.getClassAccessFlags] called");
        return GetClassModifiers(param_vars[0]);
    }

}