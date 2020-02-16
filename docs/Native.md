# Native API

## Classes

The basic type for any kind of Java class is `javm::core::ClassObject`. This big type includes the basic elements a class must have (interfaces, super class, class name, method/static function handling, fields...), plus various helpers regarding class-related stuff (processing class names, function descriptions...)

libjavm class objects inherit from two main class types:

### ClassFile

Bytecode/external classes (*.class files, both independent files or classes inside JAR archives) are represented by `javm::core::ClassFile` type.

### Native classes

Native classes, which take care of the bridge between C++ and Java code, are represented by `javm::native::Class` type.

## Native coding

Creating a custom C++ native Java class to interact between C++ and Java may look complex initially, but it is easier than it looks.

Let's take a look at this example. We want to create a class holding a two-dimensional x/y point (`com.test.Point2D`):

```cpp
#include <java/lang/String.class.hpp>

// Let's mimic Java packages with C++ namespaces, makes it look better
namespace com::test {

    using namespace javm;

    // Important: native classes must inherit from native::Class, not java::lang::Object!
    // Java inheritance is handled differently, not via C++ inheritance in this case!
    class Point2D : public native::Class {

        private:
            // Let's store actual variables here, like a normal C++ class

            int x;
            int y;

        public:
            // Let's make some getters/setters to be called from Java functions
            // We will use these to get/set our private values

            void SetX(int x) {
                this->x = x;
            }

            void SetY(int y) {
                this->y = y;
            }

            int GetX() {
                return this->x;
            }

            int GetY() {
                return this->y;
            }

        public:
            // Always use this macro for the constructor, since it also takes care of essential elements regarding native classes!
            // It's the equivalent of typing Point2D() + several definitions prior to that

            JAVM_NATIVE_CLASS_CTOR(Point2D) {

                // Register the class name - this must always be done!
                JAVM_NATIVE_CLASS_NAME("com.test.Point2D")

                // Register the constructor and methods defined below
                JAVM_NATIVE_CLASS_REGISTER_CTOR(constructor)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(getX)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(getY)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(setX)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(setY)
                JAVM_NATIVE_CLASS_REGISTER_METHOD(toString)

            }

            // Constructor/methods are always like this: core::Value <fn>(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters)

            // Static block/static functions are always like this: core::Value <fn>(core::Frame &frame, std::vector<core::FunctionParameter> parameters)

            // The name for the constructor call isn't relevant

            core::Value constructor(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                    // Let's get the Point2D class pointer from the 'this' object we were given
                    auto this_ref = this->GetThisInvokerInstance<Point2D>(this_v);

                    // We assume we are given two ints
                    // First we get the values, then we get the actual variables from the values
                    auto x_val = parameters[0].value;
                    auto x = x_val->Get<int>();
                    auto y_val = parameters[1].value;
                    auto y = y_val->Get<int>();

                    // Set the variables we were given
                    this_ref->SetX(x);
                    this_ref->SetY(y);

                    // Constructors don't return anything
                    // Use this macro when specifying a function is void, constructor or static block!
                    
                    JAVM_NATIVE_CLASS_NO_RETURN
                }

            core::Value getX(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                    // Get the Point2D class pointer from the 'this' object we were given
                    auto this_ref = this->GetThisInvokerInstance<Point2D>(this_v);

                    // Get the object's x int
                    auto x = this_ref->GetX();
                    
                    // Return a value holding the item
                    return core::CreateNewValue<int>(x);
                }

            core::Value getY(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                    // Same as getX, but with Y
                    auto this_ref = this->GetThisInvokerInstance<Point2D>(this_v);

                    auto y = this_ref->GetY();
                    
                    return core::CreateNewValue<int>(y);
                }

            core::Value setX(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                    // Get the Point2D class pointer from the 'this' object we were given
                    auto this_ref = this->GetThisInvokerInstance<Point2D>(this_v);

                    // First we get the value, then we get the actual variable from the value
                    auto x_val = parameters[0].value;
                    auto x = x_val->Get<int>();

                    // Note: SetX = C++ function to set the actual object, setX = Java function
                    this_ref->SetX(x);
                    
                    // It's a void function ;)
                    JAVM_NATIVE_CLASS_NO_RETURN
                }

            core::Value setY(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                    // Same as setX with Y
                    auto this_ref = this->GetThisInvokerInstance<Point2D>(this_v);

                    // First we get the value, then we get the actual variable from the value
                    auto y_val = parameters[0].value;
                    auto y = y_val->Get<int>();

                    this_ref->SetY(y);
                    
                    // It's a void function ;)
                    JAVM_NATIVE_CLASS_NO_RETURN
                }

            core::Value toString(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters) {
                    // Same as setX with Y
                    auto this_ref = this->GetThisInvokerInstance<Point2D>(this_v);

                    // This time, we will create the C++ string with the point coordinates like "(x, y)"
                    std::string str = "(";
                    auto x = this_ref->GetX();
                    str += std::to_string(x);
                    str += ", ";
                    auto y = this_ref->GetY();
                    str += std::to_string(y);
                    str += ")";

                    // Now, we want to create a Java String with the C++ string.
                    // We create the object with core::CreateNewClassWith<true> (true = call the internal Java constructor, although it's empty)

                    auto str_value = core::CreateNewClassWith<true>(frame, "java.lang.String", [&](auto *ref) {
                        // We cast the class pointer to a java::lang::String, and assing it the C++ string with SetNativeString
                        reinterpret_cast<java::lang::String*>(ref)->SetNativeString(str);
                    });

                    // As an extra example, if we wanted to call a method of the Java object (for instance, String.length()), we do it this way:
                    // First, we get the String value as an actual String
                    auto str_obj = str_value->GetReference<java::lang::String>();
                    
                    // We specify the function name and the return type, what we can easily do using core::TypeDefinitions's static functions.
                    auto len_value = str_obj->CallMethod(frame, "length", core::TypeDefinitions::GetPrimitiveTypeDefinition<int>());
                    auto len = len_value->Get<int>();

                    // Now, len == str.length()
                    if(len != str.length()) {
                        // This would make no sense, unless 'str' or the String object were modified before doing this check.
                    }
                    
                    // We return the Java String value
                    return str_value;
                }

    };

}
```

### Java code in C++ and javm API

A bit of this is shown in the class example above:

- Creating and throwing an Exception/Throwable object:

```cpp
// Assuming we have a 'frame' variable, which is part of the elements sent to a native function!
// The <true> indicates whether to execute the Java constructor, which should always be true, except on special cases
auto str = javm::core::CreateNewClassWith<true>(frame, "java.lang.String", [&](auto *ref) {
    reinterpret_cast<java::lang::String*>(ref)->SetNativeString("This is an error :(");
});

auto exception = javm::core::CreateNewClass<true>(frame, "java.lang.RuntimeException", str);

frame.ThrowWithInstance(exception);
```

```java
// In Java we don't do ' new String("...") ', it is directly assigned
String str = "This is an error :(";

RuntimeException exception = new RuntimeException(str);

throw exception;
```

- Iterating a String's characters:

```cpp
auto str = javm::core::CreateNewClassWith<true>(frame, "java.lang.String", [&](auto *ref) {
    reinterpret_cast<java::lang::String*>(ref)->SetNativeString("Demo text");
});

auto str_obj = str->GetReference<java::lang::String>();

auto length = str_obj->CallMethod(frame, "length", javm::core::TypeDefinitions::GetPrimitiveTypeDefinition<int>());

auto length_val = length->Get<int>();

for(int i = 0; i < length_val; i++) {
    auto i_v = javm::core::CreateNewValue<int>(i);
    auto c = str_obj->CallMethod(frame, "charAt", javm::core::TypeDefinitions::GetPrimitiveTypeDefinition<char>(), i_v);
    auto c_val = c->Get<char>();
    // Do something with the char...
}
```

```java
String str = "Demo text";

int length = str.length();

for(int i = 0; i < length; i++) {
    char c = str.charAt(i);
    // Do something with the char...
}

```

> TODO: more examples and usage of the API