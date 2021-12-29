
#pragma once
#include <javm/vm/vm_Variable.hpp>

namespace javm::native::impl::sun::misc {

    using namespace vm;

    class Unsafe {
        public:
            static ExecutionResult registerNatives(const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult arrayBaseOffset(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult arrayIndexScale(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult addressSize(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult objectFieldOffset(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult getIntVolatile(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult compareAndSwapInt(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult allocateMemory(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult putLong(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult getByte(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
            static ExecutionResult freeMemory(Ptr<Variable> this_var, const std::vector<Ptr<Variable>> &param_vars);
    };

}