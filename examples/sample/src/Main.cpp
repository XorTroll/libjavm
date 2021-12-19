// Define this to enable debug logs (tons of output, only useful for development!)
// #define JAVM_DEBUG_LOG

#include <javm/javm_VM.hpp>

// We will use/load JAR archives
#include <javm/rt/rt_JavaArchiveSource.hpp>
using namespace javm;

// Threading and sync implementations must be included:

// Use default threading implementation (pthread)
#include <javm/extras/extras_PthreadThread.hpp>

// Use default sync implementation (std C++)
#include <javm/extras/extras_CppSync.hpp>

void CheckHandleException(const vm::ExecutionResult ret) {
    if(ret.Is<vm::ExecutionStatus::Thrown>()) {
        auto [thread, throwable_var] = vm::ThreadUtils::GetThrownExceptionInfo();
        auto throwable_obj = throwable_var->GetAs<vm::type::ClassInstance>();
        auto msg_v = throwable_obj->GetField(u"detailMessage", u"Ljava/lang/String;");
        auto msg = u"Exception in thread \"" + thread->GetThreadName() + u"\" " + vm::TypeUtils::FormatVariableType(throwable_var);
        const auto msg_str = vm::StringUtils::GetValue(msg_v);
        if(!msg_str.empty()) {
            msg += u": " + msg_str;
        }
        printf("%s\n", StrUtils::ToUtf8(msg).c_str());
        for(auto call_info: thread->GetInvertedCallStack()) {
            printf("    at %s.%s%s\n", StrUtils::ToUtf8(call_info.caller_type->GetClassName()).c_str(), StrUtils::ToUtf8(call_info.invokable_name).c_str(), StrUtils::ToUtf8(call_info.invokable_desc).c_str());
        }
        exit(0);
    }
    else if(ret.Is<vm::ExecutionStatus::Invalid>()) {
        printf("Invalid return!?\n");
        exit(0);
    }
}

// In this example, the program is called with Java's standard lib JAR (rt.jar) and another executable JAR to run it, plus optional arguments to be forwarded to Java code

int main(int argc, char **argv) {
    constexpr auto ExpectedArgCount = 3;
    if(argc < ExpectedArgCount) {
        printf("Expected usage: sample <rt-jar-path> <main-jar-path> [<java-main-args>]\n");
        return 0;
    }

    // 1) add class sources (JARs, class files...)
    auto rt_jar = rt::CreateAddClassSource<rt::JavaArchiveSource>(argv[1]); // Java standard library JAR (rt.jar)
    auto main_jar = rt::CreateAddClassSource<rt::JavaArchiveSource>(argv[2]); // Entrypoint JAR

    // 2) initial VM preparation (must be called ONCE)
    rt::InitializeVM();

    // 3) prepare execution, which must be done here (before any executions) and/or after having called ResetExecution()
    const auto ret = rt::PrepareExecution();
    CheckHandleException(ret);

    const auto args_len = argc - ExpectedArgCount;

    // Create a Java string array (String[]) and populate it
    auto args_arr_v = vm::TypeUtils::NewArray(args_len, rt::LocateClassType(u"java/lang/String"));
    auto args_arr_obj = args_arr_v->GetAs<vm::type::Array>();
    for(auto i = 0; i < args_len; i++) {
        args_arr_obj->SetAt(i, vm::StringUtils::CreateNew(StrUtils::FromUtf8(argv[ExpectedArgCount + i])));
    }

    if(main_jar->CanBeExecuted()) {
        // Call the JAR's main(String[]) method
        auto main_class_type = rt::LocateClassType(main_jar->GetMainClass());
        const auto ret = main_class_type->CallClassMethod(u"main", u"([Ljava/lang/String;)V", args_arr_v);
        CheckHandleException(ret);
    }
    else {
        // The JAR failed to load or it doesn't specify a main class (is a JAR library)
        printf("The JAR file can't be executed or is not an executable JAR...\n");
    }

    return 0;
}