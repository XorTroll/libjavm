
// Define this to enable debug logs (~300k lines of output, only useful for development!)
// #define JAVM_DEBUG_LOG

#include <javm/javm_VM.hpp>

// We will use JARs
#include <javm/rt/rt_JavaArchiveSource.hpp>
using namespace javm;

// Use pthread for threading
#include <javm/extras/extras_PthreadThread.hpp>

// Use std C++ for locking
#include <javm/extras/extras_CppSync.hpp>

void PrintClassType(Ptr<vm::ClassType> class_type) {
    printf("\n");
    printf("    Java class { Name: '%s', Access: 0x%X } information:", class_type->GetClassName().c_str(), class_type->GetAccessFlags());
    printf("\n");
    if(class_type->HasSuperClass()) {
        printf("\n");
        printf("    - Super class: '%s'", class_type->GetSuperClassName().c_str());
        printf("\n");
    }
    if(class_type->HasInterfaces()) {
        printf("\n");
        printf("    - Interface(s): ");
        for(auto intf: class_type->GetInterfaceClassNames()) {
            printf("'%s', ", intf.c_str());
        }
        printf("\n");
    }
    printf("\n");
    printf("    - Functions and methods: {");
    for(auto &invokable: class_type->GetInvokables()) {
        printf("\n");
        if(invokable.HasFlag<vm::AccessFlags::Static>()) {
            printf("        Static function ");
        }
        else {
            printf("        Method ");
        }
        printf("{ Name = '%s', Descriptor = '%s', Access = 0x%X }", invokable.GetName().c_str(), invokable.GetDescriptor().c_str(), invokable.GetAccessFlags());
    }
    printf("\n");
    printf("    }");
    printf("\n\n");
    printf("    - Fields: {");
    for(auto &field: class_type->GetFields()) {
        printf("\n");
        if(field.HasFlag<vm::AccessFlags::Static>()) {
            printf("         Static field ");
        }
        else {
            printf("         Field ");
        }
        printf("{ Name = '%s', Descriptor = '%s', Access = 0x%X }", field.GetName().c_str(), field.GetDescriptor().c_str(), field.GetAccessFlags());
    }
    printf("\n");
    printf("    }");
    printf("\n");
}

void CheckHandleException(vm::ExecutionResult ret) {
    if(ret.Is<vm::ExecutionStatus::ThrowableThrown>()) {
        auto throwable_obj = ret.ret_var->GetAs<vm::type::ClassInstance>();
        auto msg_v = throwable_obj->GetField("detailMessage", "Ljava/lang/String;");
        auto cur_thr = vm::ThreadUtils::GetCurrentThread();
        auto msg = "Exception thrown in thread '" + cur_thr->GetThreadName() + "' (" + vm::TypeUtils::FormatVariableType(ret.ret_var) + ") - " + vm::StringUtils::GetValue(msg_v);
        printf("%s\n", msg.c_str());
        for(auto call_info: cur_thr->GetInvertedCallStack()) {
            printf("- At %s - %s%s\n", call_info.caller_type->GetClassName().c_str(), call_info.invokable_name.c_str(), call_info.invokable_desc.c_str());
        }
        exit(0);
    }
    else if(ret.Is<vm::ExecutionStatus::Invalid>()) {
        printf("Invalid return!?\n");
        exit(0);
    }
}

int main(int argc, char **argv) {

    if(argc < 3) {
        printf("Bad arguments - usage: javm-test <rt-jar> <main-jar> [<args-to-be-passed-for-jar-main>]\n");
        return 0;
    }

    // 1 - Add class sources (JARs, class files...)
    auto rt_jar = rt::CreateAddClassSource<rt::JavaArchiveSource>(argv[1]); // Java standard lib JAR (rt.jar)
    auto main_jar = rt::CreateAddClassSource<rt::JavaArchiveSource>(argv[2]); // Entrypoint JAR

    // 2 - initial VM preparation (called ONCE)
    rt::InitializeVM();

    // 3 - prepare execution - must be called here (before executing anything else) and after having called ResetExecution()
    auto ret = rt::PrepareExecution();
    CheckHandleException(ret);

    int args_off = 3;
    int args_len = argc - args_off;

    auto args_arr_v = vm::TypeUtils::NewArray(args_len, rt::LocateClassType("java/lang/String"));
    auto args_arr_obj = args_arr_v->GetAs<vm::type::Array>();
    for(u32 i = 0; i < args_len; i++) {
        args_arr_obj->SetAt(i, vm::StringUtils::CreateNew(argv[args_off + i]));
    }

    if(main_jar->CanBeExecuted()) {
        auto main_class_type = rt::LocateClassType(main_jar->GetMainClass());
        auto res = main_class_type->CallClassMethod("main", "([Ljava/lang/String;)V", args_arr_v);
        CheckHandleException(res);
    }
    else {
        printf("The JAR file can't be executed or is not an executable JAR.");
    }

    return 0;
}