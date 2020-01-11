# libjavm

libjavm is a small, header-only, zero-dependency C++17 Java Virtual Machine library.

It provides everything necessary to run Java (8) code in any kind of system.

The only thing necessary is a proper standard library, whose native interface is being worked on asan optional part of the library.

## Credits

- [python-jvm-interpreter](https://github.com/gkbrk/python-jvm-interpreter), since this project is basically a C++ port of that VM project, with the aim of extending and improving it.

- [andyzip](https://github.com/andy-thomason/andyzip) library, since it's used for JAR loading as an easy-to-use and header-only ZIP file reading library

- [KiVM](https://github.com/imkiva/KiVM), since it's code was checked for certain aspects (mainly the implementation of certain instructions)

## Usage

This is a quick demo of how libjavm works:

```cpp
#include <javm/core/core_Machine.hpp>
using namespace javm;

int main() {

    // First of all, create a machine.
    core::Machine machine;

    // You can load .class files...
    machine.LoadClassFile("<path-to-class-file>");

    // ...or even JAR archive files!
    machine.LoadJavaArchive("<path-to-jar-file>");

    // Then, this loads the basic Java classes and types: Object, String... (necessary for most stuff)
    machine.LoadBuiltinNativeClasses();

    // Create input arguments for function call - for main(...) we create a string array:
    java::lang::String arg1;
    arg1.SetNativeString("hello");
    java::lang::String arg2;
    arg2.SetNativeString("java");
    // Javm arrays are vectors of value holders, so this simple function simplifies their creation
    auto args = core::CreateArray<java::lang::String>({ arg1, arg2 });

    // Call the function. Input parameters must be passed as pointers, so it's as simple as creating them here and passing by '&'.
    auto ret_value = machine.CallFunction("<loaded-class-name>", "<static-function-name>", &args);
    
    // Before checking the return value, check if any exceptions ocurred.
    if(machine.WasExceptionThrown()) {
        auto exception_info = machine.GetExceptionInfo();

        // Log Java-style exception info :P
        printf("Exception in thread 'main' %s: %s\n", exception_info.class_type.c_str(), exception_info.message.c_str());
    }
    else {
        // Everything went fine, let's check the returned value.
        auto value_type = ret_value->GetValueType();

        auto value_name = core::ClassObject::GetValueTypeName(value_type);
        printf("Returned value type: %s\n", value_name.c_str());

        // Switch and handle the value, depending on the type
        switch(value_type) {
            // ...
        }
    }

    return 0;
}
```

### For a more deep explanation and/or documentation, go [here](docs/Start.md)!

## TO-DO list

- [ ] Implement all opcodes/instructions (very, very few missing)

- [ ] Fix problems with polymorphism/inheritance/interfaces (specially broken with non-native code)

- [ ] Implement a basic standard library (barely started, this will take a long time)

## Standard library

### Implemented types

- `java.lang.Object` (methods like `clone` not implemented yet)

- `java.lang.String` (constructor or methods not implemented yet)

- `java.lang.StringBuilder` (barely implemented constructor, `append` and `toString`)

- `java.io.PrintStream` (implemented String-based constructor, `print` and `println`)

- `java.lang.System` (implemented `out` and `err` static streams to write to console)

- `java.lang.Throwable` (implemented String-based constructor and `getMessage`)

- `java.lang.Exception` (empty class extending from `Throwable`)