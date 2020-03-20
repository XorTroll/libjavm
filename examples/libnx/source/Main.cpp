
// Define this to enable debug logs (~300k lines of output, only useful for development!)
// #define JAVM_DEBUG_LOG

#include <javm/javm_VM.hpp>

// We will use JARs
#include <javm/rt/rt_JavaArchiveSource.hpp>
using namespace javm;

// Use threads/condvars/rmutexes for libnx
#include <nx_LibnxThread.hpp>
#include <nx_LibnxSync.hpp>

void DoExit()  {

    // Let's make sure everything we've printed before gets shown
    consoleUpdate(nullptr);

    while(appletMainLoop()) {
        hidScanInput();
        if(hidKeysDown(CONTROLLER_P1_AUTO) & KEY_A) {
            break;
        }
    }

    consoleExit(nullptr);

    exit(0);

}

void CheckHandleException(vm::ExecutionResult ret) {
    if(ret.Is<vm::ExecutionStatus::Thrown>()) {
        auto [thread, throwable_var] = vm::ThreadUtils::GetThrownExceptionInfo();
        auto throwable_obj = throwable_var->GetAs<vm::type::ClassInstance>();
        auto msg_v = throwable_obj->GetField("detailMessage", "Ljava/lang/String;");
        auto msg = "Exception in thread \"" + thread->GetThreadName() + "\" " + vm::TypeUtils::FormatVariableType(throwable_var);
        auto msg_str = vm::StringUtils::GetValue(msg_v);
        if(!msg_str.empty()) {
            msg +=  + ": " + vm::StringUtils::GetValue(msg_v);
        }
        printf("%s\n", msg.c_str());
        for(auto call_info: thread->GetInvertedCallStack()) {
            printf("    at %s.%s%s\n", call_info.caller_type->GetClassName().c_str(), call_info.invokable_name.c_str(), call_info.invokable_desc.c_str());
        }
        DoExit();
    }
    else if(ret.Is<vm::ExecutionStatus::Invalid>()) {
        printf("Invalid return!?\n");
        DoExit();
    }
}

int main(int argc, char **argv) {

    consoleInit(nullptr);

    // 1 - Add class sources (JARs, class files...)
    auto rt_jar = rt::CreateAddClassSource<rt::JavaArchiveSource>("sdmc:/rt.jar"); // Java standard lib JAR (rt.jar)
    auto main_jar = rt::CreateAddClassSource<rt::JavaArchiveSource>("sdmc:/libnx-javm.jar"); // Entrypoint JAR

    // 2 - initial VM preparation (called ONCE)
    rt::InitializeVM();

    // 3 - prepare execution - must be called here (before executing anything else) and after having called ResetExecution()
    auto ret = rt::PrepareExecution();
    CheckHandleException(ret);

    if(main_jar->CanBeExecuted()) {
        auto main_class_type = rt::LocateClassType(main_jar->GetMainClass());
        auto res = main_class_type->CallClassMethod("main", "([Ljava/lang/String;)V", vm::TypeUtils::Null());
        CheckHandleException(res);
    }
    else {
        printf("The JAR file can't be executed or is not an executable JAR.\n");
    }

    DoExit();

    return 0;
}