#include <javm/javm_VM.hpp>
#include <javm/native/impl/sun/misc/misc_Signal.hpp>

namespace javm::native::impl::sun::misc {

    using namespace vm;

    // TODO: properly implement signal support

    ExecutionResult Signal::findSignal(const std::vector<Ptr<Variable>> &param_vars) {
        auto sig_v = param_vars[0];
        const auto sig = jutil::GetStringValue(sig_v);
        JAVM_LOG("[sun.misc.Signal.findSignal] called - signal: '%s'...", str::ToUtf8(sig).c_str());
        return ExecutionResult::ReturnVariable(MakeFalse());
    }

    ExecutionResult Signal::handle0(const std::vector<Ptr<Variable>> &param_vars) {
        JAVM_LOG("[sun.misc.Signal.handle0] called...");
        return ExecutionResult::ReturnVariable(NewPrimitiveVariable<type::Long>(2));
    }

}