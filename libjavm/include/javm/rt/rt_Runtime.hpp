
#pragma once
#include <javm/vm/vm_Thread.hpp>
#include <javm/native/native_Standard.hpp>
#include <javm/rt/rt_Context.hpp>

namespace javm::rt {

    vm::ExecutionResult PrepareExecution();

    inline void InitializeVM(const vm::PropertyTable &initial_system_props) {
        native::RegisterNativeStandardImplementation();
        vm::SetInitialSystemProperties(initial_system_props);
    }

    inline void ResetExecution() {
        ResetCachedClassTypes();
    }

}