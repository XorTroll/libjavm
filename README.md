# libjavm

libjavm is a simple, header-only, zero-dependency C++17 Java Virtual Machine library.

The only "dependency" it does require is standard C and C++17 libraries, plus the offial Java standard library (`rt.jar` file).

It provides everything necessary to run Java (8 or lower...?) code in any kind of system.

## Credits

- [python-jvm-interpreter](https://github.com/gkbrk/python-jvm-interpreter), as the original base for the project.

- [andyzip](https://github.com/andy-thomason/andyzip) library, since it's used for JAR loading as an easy-to-use and header-only ZIP file reading library.

- [KiVM](https://github.com/imkiva/KiVM), since it's code was checked for a lot of aspects of the VM.

## Usage

This is a quick demo of how libjavm works:

```cpp
#include <javm/javm_VM.hpp>

// We will use JAR archives
#include <javm/rt/rt_JavaArchiveSource.hpp>
using namespace javm;

// An implementation for threads and another one for synchronization need to be included
// By default, libjavm provides implementations for pthread-threading and standard C++'s sync stuff

// Use pthread for threading
#include <javm/extras/extras_PthreadThread.hpp>

// Use std C++ for locking
#include <javm/extras/extras_CppSync.hpp>

void CheckHandleException(vm::ExecutionResult ret) {
    // If an exception is thrown, let's handle it and throw it
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

// In this example, the program is called with Java's standard lib JAR (rt.jar) and another executable JAR to run it, plus optional arguments to be forwarded to the JAR

int main(int argc, char **argv) {

    if(argc < 3) {
        printf("Bad arguments - usage: test <rt-jar> <main-jar> [<args-to-be-passed-for-jar-main>]\n");
        return 0;
    }

    // 1 - Add class sources (JARs, class files...)
    auto rt_jar = rt::CreateAddClassSource<rt::JavaArchiveSource>(argv[1]); // Java standard lib JAR (rt.jar)
    auto main_jar = rt::CreateAddClassSource<rt::JavaArchiveSource>(argv[2]); // Entrypoint JAR

    // 2 - Initial VM preparation (called only ONCE)
    rt::InitializeVM();

    // 3 - Prepare execution - must be called here (before executing anything else) and after having called ResetExecution()
    auto ret = rt::PrepareExecution();
    CheckHandleException(ret);

    int args_off = 3;
    int args_len = argc - args_off;

    // Create a String array (String[]) and populate it
    auto args_arr_v = vm::TypeUtils::NewArray(args_len, rt::LocateClassType("java/lang/String"));
    auto args_arr_obj = args_arr_v->GetAs<vm::type::Array>();
    for(u32 i = 0; i < args_len; i++) {
        args_arr_obj->SetAt(i, vm::StringUtils::CreateNew(argv[args_off + i]));
    }

    if(main_jar->CanBeExecuted()) {
        // Call the JAR's main(String[]) method
        auto main_class_type = rt::LocateClassType(main_jar->GetMainClass());
        auto res = main_class_type->CallClassMethod("main", "([Ljava/lang/String;)V", args_arr_v);
        CheckHandleException(res);
    }
    else {
        // The JAR failed to load or it doesn't specify a main class (is a JAR library)
        printf("The JAR file can't be executed or is not an executable JAR.");
    }

    return 0;
}
```

### For a more deep explanation and/or documentation, go [here](docs/Start.md)!

## TO-DO list

- Implement `invokedynamic`, `wide`, `tableswitch` opcodes

- Implement not implemented native methods (only implemented basic ones to get past initialization, for now)

- Use UTF-16 Strings both in Java and C++, currently using UTF-8 / std::string

- Dynamic invoking stuff, related to `invokedynamic` opcode (method handle support, constant pool items related to this...)

- Support or take into account annotations (currently only `CallerSensitive` is checked)