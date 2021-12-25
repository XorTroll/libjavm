
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

    class Double {
        public:
            static ExecutionResult doubleToRawLongBits(const std::vector<Ptr<Variable>> &param_vars) {
                auto d_v = param_vars[0];
                const auto d_dbl = d_v->GetValue<type::Double>();
                JAVM_LOG("[java.lang.Double.doubleToRawLongBits] called - double: %f", d_dbl);
                
                
                union {
                    long l;
                    double dbl;
                } double_conv{};
                double_conv.dbl = d_dbl;

                const auto res_l = double_conv.l;
                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Long>(res_l));
            }

            static ExecutionResult longBitsToDouble(const std::vector<Ptr<Variable>> &param_vars) {
                auto l_v = param_vars[0];
                const auto l_long = l_v->GetValue<type::Long>();
                JAVM_LOG("[java.lang.Double.longBitsToDouble] called - long: %ld", l_long);

                union {
                    long l;
                    double dbl;
                } double_conv{};
                double_conv.l = l_long;

                const auto res_d = double_conv.dbl;
                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Double>(res_d));
            }
    };

}