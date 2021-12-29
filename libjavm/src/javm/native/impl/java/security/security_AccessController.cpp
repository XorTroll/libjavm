#include <javm/javm_VM.hpp>
#include <javm/native/impl/java/security/security_AccessController.hpp>

namespace javm::native::impl::java::security {

    using namespace vm;

    ExecutionResult AccessController::doPrivileged(const std::vector<Ptr<Variable>> &param_vars) {
        auto action_v = param_vars[0];
        JAVM_LOG("[java.security.AccessController.doPrivileged] called - action type: '%s'", str::ToUtf8(FormatVariableType(action_v)).c_str());
        auto action_obj = action_v->GetAs<type::ClassInstance>();
        const auto res = action_obj->CallInstanceMethod(u"run", u"()Ljava/lang/Object;", action_v);
        JAVM_LOG("Return type of privileged action: %d", static_cast<u32>(res.status));
        return res;
    }

    ExecutionResult AccessController::getStackAccessControlContext(const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.security.AccessController.getStackAccessControlContext] called");
        return ExecutionResult::ReturnVariable(MakeNull());
    }

}