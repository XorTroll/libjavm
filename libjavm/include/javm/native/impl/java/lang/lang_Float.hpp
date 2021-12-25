
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

    class Float {
        public:
            static ExecutionResult floatToRawIntBits(const std::vector<Ptr<Variable>> &param_vars) {
                auto f_v = param_vars[0];
                auto f_flt = f_v->GetValue<type::Float>();
                JAVM_LOG("[java.lang.Float.floatToRawIntBits] called - float: %f", f_flt);

                union {
                    int i;
                    float flt;
                } float_conv{};
                float_conv.flt = f_flt;

                const auto res_i = float_conv.i;
                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Integer>(res_i));
            }
    };

}