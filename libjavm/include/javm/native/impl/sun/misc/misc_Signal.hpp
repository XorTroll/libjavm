
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

namespace javm::native::impl::sun::misc {

    using namespace vm;

    // TODO: properly implement signal support

    class Signal {
        private:
            static inline std::map<String, type::Integer> g_signal_table = { /* ... */ };

            static inline std::map<String, type::Integer> &GetSignalTable() {
                return g_signal_table;
            }

        public:
            static ExecutionResult findSignal(const std::vector<Ptr<Variable>> &param_vars) {
                auto sig_v = param_vars[0];
                const auto sig = jstr::GetValue(sig_v);
                JAVM_LOG("[sun.misc.Signal.findSignal] called - signal: '%s'...", str::ToUtf8(sig).c_str());

                auto &sig_table = GetSignalTable();
                auto it = sig_table.find(sig);
                if(it != sig_table.end()) {
                    return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Integer>(it->second));
                }

                return ExecutionResult::ReturnVariable(TypeUtils::False());
            }

            static ExecutionResult handle0(const std::vector<Ptr<Variable>> &param_vars) {
                JAVM_LOG("[sun.misc.Signal.handle0] called...");
                return ExecutionResult::ReturnVariable(TypeUtils::NewPrimitiveVariable<type::Long>(2));
            }
    };

}