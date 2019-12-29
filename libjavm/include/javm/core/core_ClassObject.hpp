
#pragma once
#include <javm/core/core_Frame.hpp>
#include <javm/core/core_ConstantPool.hpp>
#include <map>
#include <memory>

namespace javm::core {

    #define JAVM_CTOR_METHOD_NAME "<init>"
    #define JAVM_STATIC_BLOCK_METHOD_NAME "<clinit>"

    struct FunctionParameter {
        std::string desc;
        ValueType parsed_type;
        ValuePointerHolder value;
    };

    /*
    
    ClassObject is the base type for a class in this VM. Two types of classes inherit from this:

    - core::ClassFile -> Bytecode class loaded from a .class file
    - native::Class -> Native class wrapping C/C++ functions as if it was a Java class, base of Java <-> C/C++ communication

    */

    class ClassObject {

        private:
            ValuePointerHolder super_class_instance;

            template<typename T>
            void HandlePushArgument(Frame &frame, T &t) {
                static_assert(std::is_pointer_v<T>, "PPP");
                auto holder = ValuePointerHolder::CreateFromExisting(t);
                frame.Push(holder);
            }

        public:

            // Some static helpers for Java classes and functions

            static std::string ProcessClassName(std::string input_name) {
                std::string copy = input_name;

                // Anything else to check?

                // Names like "java.lang.String" -> "java/lang/String"
                std::replace(copy.begin(), copy.end(), '.', '/');

                return copy;
            }

            static std::string GetPresentableClassName(std::string input_name) {
                std::string copy = input_name;

                // Opposite to the function above :P
                std::replace(copy.begin(), copy.end(), '/', '.');

                return copy;
            }

            static u32 GetFunctionParameterCount(std::string desc) {
                auto tmp = desc.substr(desc.find_first_of('(') + 1);
                tmp = tmp.substr(0, tmp.find_last_of(')'));
                bool parsing_class = false;
                u32 count = 0;
                for(auto &ch: tmp) {
                    if(parsing_class) {
                        if(ch == ';') {
                            parsing_class = false;
                        }
                        continue;
                    }
                    else {
                        if(ch == 'L') {
                            parsing_class = true;
                        }
                    }
                    count++;
                }
                return count;
            }

            static std::vector<std::string> ParseFunctionDescriptorParameters(std::string desc) {
                std::vector<std::string> params;
                auto tmp = desc.substr(desc.find_first_of('(') + 1);
                tmp = tmp.substr(0, tmp.find_last_of(')'));
                bool parsing_class = false;
                std::string tmp_class;
                for(auto &ch: tmp) {
                    if(parsing_class) {
                        if(ch == ';') {
                            params.push_back("L" + tmp_class + ";");
                            tmp_class = "";
                            parsing_class = false;
                        }
                        else {
                            tmp_class += ch;
                        }
                        continue;
                    }
                    else {
                        if(ch == 'L') {
                            parsing_class = true;
                            continue;
                        }
                    }
                    std::string str;
                    str += ch;
                    params.push_back(str);
                }
                return params;
            }

            static ValueType ParseValueType(std::string param) {
                if(param == "B") {
                    return ValueType::Byte;
                }
                if(param == "Z") {
                    return ValueType::Boolean;
                }
                if(param == "S") {
                    return ValueType::Short;
                }
                if(param == "C") {
                    return ValueType::Character;
                }
                if(param == "I") {
                    return ValueType::Integer;
                }
                if(param == "J") {
                    return ValueType::Long;
                }
                if(param == "F") {
                    return ValueType::Float;
                }
                if(param == "D") {
                    return ValueType::Double;
                }
                if(param.front() == '[') {
                    return ValueType::Array;
                }
                if(param.front() == 'L') {
                    return ValueType::ClassObject;
                }
                return ValueType::Void;
            }

            static std::string GetValueTypeName(ValueType type) {
                switch(type) {
                    case ValueType::Byte:
                        return "byte";
                    case ValueType::Boolean:
                        return "boolean";
                    case ValueType::Short:
                        return "short";
                    case ValueType::Character:
                        return "char";
                    case ValueType::Integer:
                        return "int";
                    case ValueType::Long:
                        return "long";
                    case ValueType::Float:
                        return "float";
                    case ValueType::Double:
                        return "byte";
                    case ValueType::Array:
                        return "array";
                    case ValueType::ClassObject:
                        return "class";
                    default:
                        return "void";
                }
                return "<unknown>";
            }

            static std::string GetParameterTypeName(FunctionParameter param) {
                switch(param.parsed_type) {
                    case ValueType::Byte:
                        return "byte";
                    case ValueType::Boolean:
                        return "boolean";
                    case ValueType::Short:
                        return "short";
                    case ValueType::Character:
                        return "char";
                    case ValueType::Integer:
                        return "int";
                    case ValueType::Long:
                        return "long";
                    case ValueType::Float:
                        return "float";
                    case ValueType::Double:
                        return "byte";
                    case ValueType::Array: {
                        FunctionParameter dummy = {};
                        auto arrtype = param.desc.substr(1);
                        dummy.desc = arrtype;
                        dummy.parsed_type = ParseValueType(arrtype);
                        return GetParameterTypeName(dummy) + "[]";
                    }
                    case ValueType::ClassObject: {
                        auto tmp = param.desc.substr(1);
                        return tmp.substr(0, tmp.length() - 1);
                    }
                    default:
                        return "void";
                }
                return "<unknown>";
            }

            template<typename T>
            static std::string MakeDescriptorParameter(T &t) {
                static_assert(!std::is_pointer_v<T>, "PPP");
                if constexpr(std::is_same_v<T, u8>) {
                    return "B";
                }
                if constexpr(std::is_same_v<T, bool>) {
                    return "Z";
                }
                if constexpr(std::is_same_v<T, short>) {
                    return "S";
                }
                if constexpr(std::is_same_v<T, char>) {
                    return "C";
                }
                if constexpr(std::is_same_v<T, int>) {
                    return "I";
                }
                if constexpr(std::is_same_v<T, long>) {
                    return "J";
                }
                if constexpr(std::is_same_v<T, float>) {
                    return "F";
                }
                if constexpr(std::is_same_v<T, double>) {
                    return "D";
                }
                if constexpr(std::is_same_v<T, std::vector<ValuePointerHolder>>) {
                    return "[I";
                }
                if constexpr(std::is_same_v<T, ClassObject> || std::is_base_of_v<ClassObject, T>) {
                    return "L" + t.GetName() + ";";
                }
                return "";
            }

            template<typename ...Args>
            static std::string BuildFunctionDescriptor(Args &&...args) {
                std::string desc;
                ((desc += MakeDescriptorParameter(*args)), ...);
                return desc;
            }

            static bool ExpectsReturn(std::string desc) {
                return desc.back() != 'V';
            }

            virtual std::string GetName() = 0;
            virtual std::string GetSuperClassName() = 0;
            virtual std::vector<CPInfo> &GetConstantPool() = 0;
            virtual ValuePointerHolder CreateInstanceEx(void *machine_ptr) = 0;
            virtual ValuePointerHolder GetField(std::string name) = 0;
            virtual ValuePointerHolder GetStaticField(std::string name) = 0;
            virtual void SetField(std::string name, ValuePointerHolder value) = 0;
            virtual void SetStaticField(std::string name, ValuePointerHolder value) = 0;
            virtual bool CanHandleMethod(std::string name, std::string desc, Frame &frame) = 0;
            virtual ValuePointerHolder HandleMethod(std::string name, std::string desc, Frame &frame) = 0;
            virtual ValuePointerHolder HandleStaticFunction(std::string name, std::string desc, Frame &frame) = 0;

            ValuePointerHolder CreateInstance(Frame &frame) {
                return this->CreateInstanceEx(frame.GetMachinePointer());
            }

            void SetSuperClassInstance(ValuePointerHolder super_class) {
                this->super_class_instance = super_class;
            }

            ValuePointerHolder GetSuperClassInstance() {
                return this->super_class_instance;
            }

            template<typename C>
            C *GetSuperClassReference() {
                return this->super_class_instance.GetReference<C>();
            }

            template<typename ...Args>
            ValuePointerHolder CallMethod(Frame &frame, std::string name, Args &&...args) {
                frame.PushReference(this);
                (this->HandlePushArgument(frame, args), ...);
                auto desc = BuildFunctionDescriptor(args...);
                if(this->CanHandleMethod(name, desc, frame)) {
                    return this->HandleMethod(name, desc, frame);
                }
                return ValuePointerHolder::CreateVoid();
            }
    };

    // Defined later in machine code
    std::shared_ptr<ClassObject> &FindClassByName(Frame &frame, std::string name);

    std::shared_ptr<ClassObject> &FindClassByNameEx(void *machine, std::string name);
}