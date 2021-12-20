# libjavm

> Simple, header-only, zero-dependency C++17 Java Virtual Machine library

The only "dependencies" it does require is standard C and C++17 libraries, plus the offial Java standard library (`rt.jar` file) since virtually all compiled Java uses it.

Note that, for threading and synchronization items (mutexes, condition vars...) you must provide your own implementation. Nevertheless, libjavm provides a default implementation with **pthread** for threading and **standard C++** for sync stuff (`pthread_t`, `std::recursive_mutex`, `std::condition_variable_any`...)

It provides everything necessary to run Java (8 or lower...?) code in any kind of system.

## Credits

- [python-jvm-interpreter](https://github.com/gkbrk/python-jvm-interpreter), as the original base for the project.

- [andyzip](https://github.com/andy-thomason/andyzip) library, since it's used for JAR loading as an easy-to-use and header-only ZIP file reading library.

- [KiVM](https://github.com/imkiva/KiVM), since it's code was checked for a lot of aspects of the VM.

## Usage

Check the [examples](examples) directory for some example programs using this library.

> TODO: proper documentation...

## Comparison with JRE

The tests used to test the VM (slightly modified KiVM tests) are located at [javm-test-suite](javm-test-suite). Currently 19 out of 28 tests are successfully passed (comparing their output with JRE):

- `ArgumentTest`: pass!

- `ArithmeticTest`: pass! (although small differencies between exception msgs)

- `ArrayTest`: pass!

- `ArrayTest1`: fail (JRE throws certain exceptions we currently don't)

- `ArrayTest2`: fail (we need to handle OOB array accesses)

- `AssertTest`: pass!

- `ChineseTest`: pass!

- `ClassCastTest`: fail (unrelated classes can be casted right now, class casting must be corrected)

- `ClassNameTest`: pass!

- `CovScriptJNITest`: fail (works as expected but we should throw a different exception)

- `ExceptionTest`: pass!

- `ExceptionTest1`: fail (invalid classes should throw an exception)

- `ExceptionTest2`: fail (makes the VM crash...? needs more work)

- `ExceptionTest3`: fail (because the exceptions we throw aren't the ones JRE throws, thus the catch blocks inside the test do nothing...)

- `FileTest`: fail (filesystem-related native methods aren't implemented)

- `GCTest`: pass!

- `HashTest`: pass!

- `HelloWorld`: pass!

- `LambdaTest`: fail (dynamic stuff not implemented yet)

- `Main`: pass!

- `PackagePrivateTest`: pass!

- `Polymorphism`: pass!

- `StaticFieldTest`: pass!

- `StaticResolution`: pass!

- `StringBuilderTest1`: pass!

- `ThreadExceptionTest`: pass!

- `ThreadTest`: pass!

## TO-DO list

- Implement `invokedynamic`, `wide`, `tableswitch` opcodes

- Implement not implemented native methods (only implemented basic ones to get past initialization, for now)

- Dynamic invoking stuff, related to `invokedynamic` opcode (method handle support, constant pool items related to this...)

- Support or take into account other annotations (currently only `CallerSensitive` is checked)

- The many `TODO` comments spread in code