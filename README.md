# libjavm

libjavm is a simple, header-only, zero-dependency C++17 Java Virtual Machine library.

The only "dependencies" it does require is standard C and C++17 libraries, plus the offial Java standard library (`rt.jar` file).

Note that, for threading and synchronization items (mutexes, condition vars...) you must provide your own implementation. This libraries provide a sample/default implementation with **pthread** for threading and **standard C++** for sync stuff (`std::recursive_mutex`, `std::condition_variable_any`...).

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

> TODO: documentation for the new VM!

## Java tests

The tests used to test the VM (thanks, KiVM!) are located at `java-test`. Currently x out of 28 tests are successfully passed (comparing their output to normal JRE's one):

- `ArgumentTest`: pass!

- `ArithmeticTest`: fail (need to throw an exception internally when dividing by zero!)

- `ArrayTest`: pass!

- `ArrayTest1`: fail (multi-dimension arrays aren't implemented yet!)

- `ArrayTest2`: pass! (the out-of-range exception isn't the same one official Java throws, so that should be corrected...)

- `AssertTest`: pass!

- `ChineseTest`: pass, but the VM currently uses UTF-8 strings, which should be UTF-16!

- `ClassCastTest`: fail (unrelated classes can be casted right now, class casting should be corrected and maybe reimplemented...)

- `ClassNameTest`: semi-pass (primitive types show a different name ('B' instead of 'byte'), and it also failed due to the lack of multi-dimension array support)

- `CovScriptJNITest`: pass (those native functions aren't present, so an exception is thrown)

- `ExceptionTest`: pass!

- `ExceptionTest1`: fail (invalid classes should throw an exception)

- `ExceptionTest2`: fail (makes the VM crash...? needs more debugging)

- `ExceptionTest3`: fail (because the exceptions we throw aren't the ones JRE throws, thus the catch blocks inside the test do nothing...)

- `FileTest`: fail (filesystem-related native methods aren't implemented)

- `GCTest`: pass! (memory is properly free'd, I guess)

- `HashTest`: pass!

- `HelloWorld`: pass!

- `LambdaTest`: fail (dynamic stuff not implemented yet...)

- `Main`: pass (we aren't setting the 'kivm.info' property, thus it's null)

- `PackagePrivateTest`: pass!

- `Polymorphism`: pass!

- `StaticFieldTest`: pass!

- `StringBuilderTest1`: pass!

- `ThreadExceptionTest`: pass!

- `ThreadTest`: pass!

Outcome: 18.5 out of 28 tests passed (for now)!

## TO-DO list

- Implement `invokedynamic`, `wide`, `tableswitch`, `multianewarray` opcodes

- Properly support multi-dimension arrays (`multianewarray`)

- Implement not implemented native methods (only implemented basic ones to get past initialization, for now)

- Use UTF-16 Strings both in Java and C++, currently using UTF-8 / std::string

- Dynamic invoking stuff, related to `invokedynamic` opcode (method handle support, constant pool items related to this...)

- Support or take into account other annotations (currently only `CallerSensitive` is checked)