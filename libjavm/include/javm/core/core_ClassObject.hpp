
#pragma once
#include <javm/core/core_Frame.hpp>
#include <javm/core/core_ConstantPool.hpp>
#include <map>
#include <memory>
#include <algorithm>

namespace javm::core {

    #define JAVM_CTOR_METHOD_NAME "<init>"
    #define JAVM_STATIC_BLOCK_METHOD_NAME "<clinit>"
    #define JAVM_EMPTY_METHOD_DESCRIPTOR "()V"

    struct FunctionParameter {
        std::string desc;
        ValueType parsed_type;
        Value value;
    };

    /*
    
    'this' values for invoking native methods:

    Invoker is the object calling the function, the reference pushed to the stack in the code.
    Instance is the actual instance containing the method, always a native class.

    In a normal method both are the same, but when this involves inheritance, both are different.
    For instance, in this case:

    > String str = "hello";
    > str.getClass();

    Inside getClass() native function, the 'invoker' will be a String variable, and the 'instance' will be a Object variable, since the function itself is Object.getClass().
    Normally accessing the 'instance' this would be enough, but for certain cases (getClass is one of them) the 'invoker' this is necessary. Hence, both are accessible.

    */
    struct ThisValues {
        Value invoker;
        Value instance;
    };

    /*
    
    ClassObject is the base type for a class in this VM. Two types of classes inherit from this:

    - core::ClassFile -> Bytecode class loaded from a .class file
    - native::Class -> Native class wrapping C/C++ functions as if it was a Java class, base of Java <-> C/C++ communication

    */

    class ClassObject {

        private:
            Value super_class_instance;

        public:

            template<typename T>
            static void DoPushArgument(std::vector<Value> tmp_holder, T t) {
                static_assert(std::is_same_v<T, Value>, "Arguments must be core::Value!");
                tmp_holder.push_back(t);
            }

            template<typename ...Args>
            static void PushParameters(Frame &frame, Args &&...args) {
                std::vector<Value> tmp_params_holder;
                (DoPushArgument(tmp_params_holder, args), ...);
                std::reverse(tmp_params_holder.begin(), tmp_params_holder.end());
                for(auto param: tmp_params_holder) {
                    frame.Push(param);
                }
            }

            // Some static helpers for Java classes and functions

            static std::string ProcessClassName(const std::string &input_name) {
                std::string copy = input_name;

                // Anything else to check?

                // Names like "java.lang.String" -> "java/lang/String"
                std::replace(copy.begin(), copy.end(), '.', '/');

                return copy;
            }

            static std::string GetPresentableClassName(const std::string &input_name) {
                std::string copy = input_name;

                // Opposite to the function above :P
                std::replace(copy.begin(), copy.end(), '/', '.');

                return copy;
            }

            static u32 GetFunctionParameterCount(const std::string &desc) {
                auto tmp = desc.substr(desc.find_first_of('(') + 1);
                tmp = tmp.substr(0, tmp.find_last_of(')'));
                bool parsing_class = false;
                u32 count = 0;
                for(auto &ch: tmp) {
                    if(ch == '[') {
                        // Array, so not a new param
                        continue;
                    }
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

            static std::vector<std::string> ParseFunctionDescriptorParameters(const std::string &desc) {
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

            static ValueType ParseValueType(const std::string &param) {
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
                return "<invalid>";
            }

            static std::string GetValueName(Value val) {
                if(val) {
                    if(val->IsClassObject()) {
                        auto class_ref = val->GetReference<ClassObject>();
                        return GetPresentableClassName(class_ref->GetName());
                    }
                    else if(val->IsArray()) {
                        auto array_ref = val->GetReference<Array>();
                        auto array_type = array_ref->GetValueType();
                        return GetValueTypeName(array_type) + "[" + std::to_string(array_ref->GetLength()) + "]";
                    }
                    else {
                        return GetValueTypeName(val->GetValueType());
                    }
                }
                return "<invalid>";
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

            static std::string MakeDescriptorParameter(Value val) {
                switch(val->GetValueType()) {
                    case ValueType::Byte:  {
                        return "B";
                    }
                    case ValueType::Boolean:  {
                        return "Z";
                    }
                    case ValueType::Short:  {
                        return "S";
                    }
                    case ValueType::Character:  {
                        return "C";
                    }
                    case ValueType::Integer:  {
                        return "I";
                    }
                    case ValueType::Long:  {
                        return "J";
                    }
                    case ValueType::Float:  {
                        return "F";
                    }
                    case ValueType::Double:  {
                        return "D";
                    }
                    case ValueType::Array:  {
                        auto arr = val->GetReference<Array>();
                        return "[" + MakeDescriptorParameter(arr->GetAt(0));
                    }
                    case ValueType::ClassObject:  {
                        return "L" + val->GetReference<ClassObject>()->GetName() + ";";
                    }
                    default:
                        return "V";
                }
            }

            template<typename ...Args>
            static std::string BuildFunctionDescriptor(Value return_type, Args &&...args) {
                std::string desc = "(";
                ((desc += MakeDescriptorParameter(args)), ...);
                desc += ")" + MakeDescriptorParameter(return_type);
                return desc;
            }

            static bool ExpectsReturn(const std::string &desc) {
                return desc.back() != 'V';
            }

            static std::vector<Value> LoadStaticFunctionParameters(Frame &frame, const std::string &fn_desc) {
                std::vector<Value> params;
                const u32 param_count = GetFunctionParameterCount(fn_desc);
                for(u32 i = 0; i < param_count; i++) {
                    params.push_back(frame.Pop());
                }
                // Params are read in inverse order!
                std::reverse(params.begin(), params.end());
                return params;
            }

            static std::pair<Value, std::vector<Value>> LoadMethodParameters(Frame &frame, const std::string &fn_desc) {
                std::vector<Value> params;
                const u32 param_count = GetFunctionParameterCount(fn_desc);
                for(u32 i = 0; i < param_count; i++) {
                    params.push_back(frame.Pop());
                }
                auto this_v = frame.Pop();
                // Params are read in inverse order!
                std::reverse(params.begin(), params.end());
                return std::make_pair(this_v, params);
            }

            virtual std::string GetName() = 0;
            virtual std::string GetSuperClassName() = 0;
            virtual std::vector<std::string> GetInterfaceNames() = 0;
            virtual std::vector<CPInfo> &GetConstantPool() = 0;
            virtual Value CreateInstanceEx(void *machine_ptr) = 0;
            virtual Value CreateFromExistingInstance() = 0;
            virtual bool HasField(const std::string &name) = 0;
            virtual bool HasStaticField(const std::string &name) = 0;
            virtual Value GetField(const std::string &name) = 0;
            virtual Value GetStaticField(const std::string &name) = 0;
            virtual void SetField(const std::string &name, Value value) = 0;
            virtual void SetStaticField(const std::string &name, Value value) = 0;
            virtual bool CanHandleMethod(const std::string &name, const std::string &desc, Frame &frame) = 0;
            virtual bool CanHandleStaticFunction(const std::string &name, const std::string &desc, Frame &frame) = 0;
            virtual Value HandleMethod(const std::string &name, const std::string &desc, Value this_v, std::vector<Value> params, Frame &frame) = 0;
            virtual Value HandleStaticFunction(const std::string &name, const std::string &desc, std::vector<Value> params, Frame &frame) = 0;
            
            bool CanSuperClassHandleMethod(const std::string &name, const std::string &desc, Frame &frame) {
                if(this->super_class_instance) {
                    if(this->GetSuperClassReference<ClassObject>()->CanHandleMethod(name, desc, frame)) {
                        return true;
                    }
                    return this->GetSuperClassReference<ClassObject>()->CanSuperClassHandleMethod(name, desc, frame);
                }
                return false;
            }

            bool CanSuperClassHandleStaticFunction(const std::string &name, const std::string &desc, Frame &frame) {
                if(this->super_class_instance) {
                    if(this->GetSuperClassReference<ClassObject>()->CanHandleStaticFunction(name, desc, frame)) {
                        return true;
                    }
                    return this->GetSuperClassReference<ClassObject>()->CanSuperClassHandleStaticFunction(name, desc, frame);
                }
                return false;
            }

            bool CanAllHandleMethod(const std::string &name, const std::string &desc, Frame &frame) {
                if(this->CanHandleMethod(name, desc, frame)) {
                    return true;
                }
                if(this->CanSuperClassHandleMethod(name, desc, frame)) {
                    return true;
                }
                return false;
            }

            bool CanAllHandleStaticFunction(const std::string &name, const std::string &desc, Frame &frame) {
                if(this->CanHandleStaticFunction(name, desc, frame)) {
                    return true;
                }
                if(this->CanSuperClassHandleStaticFunction(name, desc, frame)) {
                    return true;
                }
                return false;
            }

            bool CanCastTo(const std::string &class_name) {
                // Iterate through every superclass to see if the name matches
                if(this->GetName() == class_name) {
                    return true;
                }
                auto super_class_instance = this->GetSuperClassInstance();
                if(super_class_instance) {
                    return super_class_instance->GetReference<ClassObject>()->CanCastTo(class_name);
                }
                return false;
            }

            Value CreateInstance(Frame &frame) {
                return this->CreateInstanceEx(frame.GetMachinePointer());
            }

            void SetSuperClassInstance(Value super_class) {
                this->super_class_instance = super_class;
            }

            Value GetSuperClassInstance() {
                return this->super_class_instance;
            }

            template<typename C>
            C *GetSuperClassReference() {
                if(!this->super_class_instance) {
                    return nullptr;
                }
                return this->super_class_instance->GetReference<C>();
            }

            template<typename T>
            void PrepareParameters(std::vector<Value> &params, T t) {
                static_assert(std::is_same_v<T, Value>, "Derp");
                params.push_back(t);
            }

            // Meant to be used from native code
            template<typename ...Args>
            Value CallMethod(Frame &frame, const std::string &name, Value return_type, Args &&...args) {
                auto this_v = this->CreateFromExistingInstance();
                std::vector<Value> params;
                (PrepareParameters(params, args), ...);
                std::reverse(params.begin(), params.end());
                auto desc = BuildFunctionDescriptor(return_type, args...);
                if(this->CanAllHandleMethod(name, desc, frame)) {
                    return this->HandleMethod(name, desc, this_v, params, frame);
                }
                return CreateInvalidValue();
            }

            // Meant to be used from native code
            template<typename ...Args>
            Value CallStaticFunction(Frame &frame, const std::string &name, Value return_type, Args &&...args) {
                std::vector<Value> params;
                (PrepareParameters(params, args), ...);
                std::reverse(params.begin(), params.end());
                auto desc = BuildFunctionDescriptor(return_type, args...);
                if(this->CanAllHandleStaticFunction(name, desc, frame)) {
                    return this->HandleStaticFunction(name, desc, params, frame);
                }
                return CreateInvalidValue();
            }
    };

    // Defined later in machine code
    std::shared_ptr<ClassObject> FindClassByName(Frame &frame, const std::string &name);

    std::shared_ptr<ClassObject> FindClassByNameEx(void *machine, const std::string &name);

    template<bool CallCtor, typename ...Args>
    Value MachineCreateNewClass(void *machine, const std::string &name, Args &&...args);

    template<bool CallCtor, typename ...Args>
    Value CreateNewClass(void *machine, const std::string &name, Args &&...args) {
        return MachineCreateNewClass<CallCtor>(machine, name, args...);
    }

    template<bool CallCtor, typename ...Args>
    Value CreateNewClass(Frame &frame, const std::string &name, Args &&...args) {
        return MachineCreateNewClass<CallCtor>(frame.GetMachinePointer(), name, args...);
    }

    template<bool CallCtor, typename ...Args>
    Value CreateNewClassWith(void *machine, const std::string &name, std::function<void(ClassObject*)> ref_fn, Args &&...args) {
        auto class_val = MachineCreateNewClass<CallCtor>(machine, name, args...);
        auto class_ref = class_val->template GetReference<ClassObject>();
        ref_fn(class_ref);
        return class_val;
    }

    template<bool CallCtor, typename ...Args>
    Value CreateNewClassWith(Frame &frame, const std::string &name, std::function<void(ClassObject*)> ref_fn, Args &&...args) {
        auto class_val = CreateNewClass<CallCtor>(frame, name, args...);
        auto class_ref = class_val->template GetReference<ClassObject>();
        ref_fn(class_ref);
        return class_val;
    }
}