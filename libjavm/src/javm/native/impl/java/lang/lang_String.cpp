#include <javm/javm_VM.hpp>
#include <javm/native/impl/java/lang/lang_String.hpp>

namespace javm::native::impl::java::lang {

    using namespace vm;

    ExecutionResult String::intern(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[java.lang.String.intern] called - string: '%s'", str::ToUtf8(jutil::GetStringValue(this_var)).c_str());
        jutil::InternVariable(this_var);
        return ExecutionResult::ReturnVariable(this_var);
    }

}