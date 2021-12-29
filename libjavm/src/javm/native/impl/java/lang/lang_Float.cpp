#include <javm/javm_VM.hpp>
#include <javm/native/impl/java/lang/lang_Float.hpp> 

namespace javm::native::impl::java::lang {

    using namespace vm;

    ExecutionResult Float::floatToRawIntBits(const std::vector<Ptr<Variable>> &param_vars) {
        auto f_v = param_vars[0];
        auto f_flt = f_v->GetValue<type::Float>();
        JAVM_LOG("[java.lang.Float.floatToRawIntBits] called - float: %f", f_flt);

        union {
            int i;
            float flt;
        } float_conv{};
        float_conv.flt = f_flt;

        const auto res_i = float_conv.i;
        return ExecutionResult::ReturnVariable(NewPrimitiveVariable<type::Integer>(res_i));
    }

}