
#pragma once
#include <javm/core/core_ClassObject.hpp>
#include <map>

namespace javm::native {

    using NativeMethodFunction = std::function<core::Value(core::Frame &frame, core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters)>;
    using NativeStaticFunction = std::function<core::Value(core::Frame &frame, std::vector<core::FunctionParameter> parameters)>;

    #define JAVM_NATIVE_DEFINITION_CTOR(clss) \
    static std::shared_ptr<clss> CreateDefinitionInstance(void *machine_ptr) { \
        auto class_ref = std::make_shared<clss>(); \
        auto super_class_name = class_ref->GetSuperClassName(); \
        auto super_class_ref = javm::core::FindClassByNameEx(machine_ptr, super_class_name); \
        if(super_class_ref) { \
            auto super_class_instance = super_class_ref->CreateInstanceEx(machine_ptr); \
            class_ref->SetSuperClassInstance(super_class_instance); \
        } \
        return class_ref; \
    }

    #define JAVM_NATIVE_INSTANCE_CTOR(clss) \
    virtual javm::core::Value CreateInstanceEx(void *machine_ptr) override { \
        auto class_holder = javm::core::CreateNewValue<clss>(); \
        auto class_ref = class_holder->GetReference<clss>(); \
        auto super_class_name = class_ref->GetSuperClassName(); \
        auto super_class_ref = javm::core::FindClassByNameEx(machine_ptr, super_class_name); \
        if(super_class_ref) { \
            auto super_class_instance = super_class_ref->CreateInstanceEx(machine_ptr); \
            class_ref->SetSuperClassInstance(super_class_instance); \
        } \
        return class_holder; \
    }

    #define JAVM_NATIVE_CLASS_CTOR(clss) \
    JAVM_NATIVE_DEFINITION_CTOR(clss) \
    JAVM_NATIVE_INSTANCE_CTOR(clss) \
    clss()

    class Class : public core::ClassObject {

        private:
            std::string name;
            std::vector<core::CPInfo> stub_empty_pool;
            bool static_done;

        protected:
            std::string super_class_name;
            std::map<std::string, NativeMethodFunction> methods;
            std::map<std::string, NativeStaticFunction> static_fns;
            std::map<std::string, core::Value> static_fields;
            std::map<std::string, core::Value> member_fields;

            void SetName(std::string class_name) {
                this->name = core::ClassObject::ProcessClassName(class_name);
            }

        public:
            Class() : static_done(false), super_class_name("java/lang/Object") {}

            virtual ~Class() {}

            virtual std::string GetName() override {
                return this->name;
            }

            virtual std::string GetSuperClassName() override {
                return this->super_class_name;
            }

            virtual std::vector<core::CPInfo> &GetConstantPool() override {
                // Stub the constant pool, this won't be called for a native class anyway
                return this->stub_empty_pool;
            }

            virtual bool HasField(std::string name) override {
                return this->member_fields.find(name) != this->member_fields.end();
            }

            virtual bool HasStaticField(std::string name) override {
                return this->static_fields.find(name) != this->static_fields.end();
            }
            
            virtual core::Value GetField(std::string name) override {
                if(this->member_fields.find(name) != this->member_fields.end()) {
                    return this->member_fields[name];
                }
                return core::CreateVoidValue();
            }

            virtual core::Value GetStaticField(std::string name) override {
                if(this->static_fields.find(name) != this->static_fields.end()) {
                    return this->static_fields[name];
                }
                return core::CreateVoidValue();
            }

            virtual void SetField(std::string name, core::Value value) override {
                if(this->member_fields.find(name) != this->member_fields.end()) {
                    this->member_fields[name] = value;
                }
                else {
                    this->member_fields.insert(std::make_pair(name, value));
                }
            }
            
            virtual void SetStaticField(std::string name, core::Value value) override {
                if(this->static_fields.find(name) != this->static_fields.end()) {
                    this->static_fields[name] = value;
                }
                else {
                    this->static_fields.insert(std::make_pair(name, value));
                }
            }

            virtual bool CanHandleMethod(std::string name, std::string desc, core::Frame &frame) override {
                if(this->methods.find(name) != this->methods.end()) {
                    return true;
                }
                else if(name == JAVM_CTOR_METHOD_NAME) {
                    // Constructors always exist, just that they are empty by default
                    return true;
                }
                return false;
            }

            virtual bool CanHandleStaticFunction(std::string name, std::string desc, core::Frame &frame) override {
                if(this->static_fns.find(name) != this->static_fns.end()) {
                    return true;
                }
                return false;
            }

            virtual core::Value HandleMethod(std::string name, std::string desc, core::Frame &frame) override {
                auto it = this->methods.find(name);
                if(it != this->methods.end()) {
                    auto params = ClassObject::ParseFunctionDescriptorParameters(desc);
                    std::vector<core::FunctionParameter> fn_params;
                    for(auto &param: params) {
                        core::FunctionParameter fparam = {};
                        fparam.desc = param;
                        fparam.value = frame.Pop();
                        fparam.parsed_type = ClassObject::ParseValueType(fparam.desc);
                        fn_params.push_back(fparam);
                    }
                    // This is the class definition, so it has the class name of the 'this' object to pass to the method
                    auto method_class_name = core::ClassObject::ProcessClassName(this->GetName());
                    core::Value this_holder = frame.Pop();
                    auto orig_this = this_holder;
                    // Auto-detect whether the function is accessible by the class, and if not, loop until a super class does implement it, to send the correct 'this' object :P
                    // Basically, the name of the class requested by the invoking instruction (this definition's class name) must be the same name/type of the 'this' object we send to the native function.
                    while(method_class_name != core::ClassObject::ProcessClassName(this_holder->GetReference<core::ClassObject>()->GetName())) {
                        auto super_holder = this_holder->GetReference<core::ClassObject>()->GetSuperClassInstance();
                        if(!super_holder) {
                            this_holder = orig_this;
                            break;
                        }
                        if(!super_holder->IsClassObject()) {
                            this_holder = orig_this;
                            break;
                        }
                        this_holder = super_holder;
                    }
                    if(method_class_name == core::ClassObject::ProcessClassName(this_holder->GetReference<core::ClassObject>()->GetName())) {
                        if(this_holder->GetReference<core::ClassObject>()->CanHandleMethod(name, desc, frame)) {
                            core::FunctionParameter this_fparam = {};
                            this_fparam.desc = this->GetName();
                            this_fparam.value = this_holder;
                            this_fparam.parsed_type = ClassObject::ParseValueType(this_fparam.desc);
                            return it->second(frame, this_fparam, fn_params);
                        }
                    }
                }
                else if(name == JAVM_CTOR_METHOD_NAME) {
                    // Constructors always exist, just that they are empty by default, so return nothing (void) as if an empty method was called.
                    return core::CreateVoidValue();
                }
                auto super_class = this->GetSuperClassInstance();
                if(super_class) {
                    if(super_class->IsClassObject()) {
                        auto super_class_ref = super_class->GetReference<core::ClassObject>();
                        return super_class_ref->HandleMethod(name, desc, frame);
                    }
                }
                frame.ThrowExceptionWithType("java.lang.RuntimeException", "Invalid method call");
                return core::CreateVoidValue();
            }

            virtual core::Value HandleStaticFunction(std::string name, std::string desc, core::Frame &frame) override {
                auto it = this->static_fns.find(name);
                if(it != this->static_fns.end()) {
                    if(name == JAVM_STATIC_BLOCK_METHOD_NAME) {
                        if(this->static_done) {
                            // Static block init is already done
                            return core::CreateVoidValue();
                        }
                        else {
                            this->static_done = true;
                        }
                    }
                    auto params = ClassObject::ParseFunctionDescriptorParameters(desc);
                    std::vector<core::FunctionParameter> fn_params;
                    for(auto &param: params) {
                        core::FunctionParameter fparam = {};
                        fparam.desc = param;
                        fparam.value = frame.Pop();
                        fparam.parsed_type = ClassObject::ParseValueType(param);
                        fn_params.push_back(fparam);
                    }
                    return it->second(frame, fn_params);
                }
                auto super_class = this->GetSuperClassInstance();
                if(super_class) {
                    if(!super_class->IsNull()) {
                        auto super_class_ref = super_class->GetReference<core::ClassObject>();
                        return super_class_ref->HandleStaticFunction(name, desc, frame);
                    }
                }
                frame.ThrowExceptionWithType("java.lang.RuntimeException", "Invalid static function call - " + name + " - " + this->GetName());
                return core::CreateVoidValue();
            }

            template<typename C>
            static C *GetThisReference(core::FunctionParameter this_param) {
                return this_param.value->template GetReference<C>();
            }
    };

    #define _JAVM_NATIVE_CLASS_REGISTER_METHOD(name_str, name) { \
        javm::native::NativeMethodFunction fn = std::bind(&JAVM_CLASS_TYPE::name, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3); \
        this->methods.insert(std::make_pair(name_str, fn)); \
    }

    #define _JAVM_NATIVE_CLASS_REGISTER_STATIC_FN(name_str, name) { \
        javm::native::NativeStaticFunction fn = std::bind(&JAVM_CLASS_TYPE::name, this, std::placeholders::_1, std::placeholders::_2); \
        this->static_fns.insert(std::make_pair(name_str, fn)); \
    }

    // ---

    #define JAVM_NATIVE_CLASS_NAME(name) this->SetName(name);
    #define JAVM_NATIVE_CLASS_EXTENDS(clss) this->super_class_name = core::ClassObject::ProcessClassName(clss);

    #define JAVM_NATIVE_CLASS_REGISTER_CTOR(name) _JAVM_NATIVE_CLASS_REGISTER_METHOD(JAVM_CTOR_METHOD_NAME, name)
    #define JAVM_NATIVE_CLASS_REGISTER_STATIC_BLOCK(name) _JAVM_NATIVE_CLASS_REGISTER_STATIC_FN(JAVM_STATIC_BLOCK_METHOD_NAME, name)
    #define JAVM_NATIVE_CLASS_REGISTER_METHOD(name) _JAVM_NATIVE_CLASS_REGISTER_METHOD(#name, name)
    #define JAVM_NATIVE_CLASS_REGISTER_STATIC_FN(name) _JAVM_NATIVE_CLASS_REGISTER_STATIC_FN(#name, name)

    #define JAVM_NATIVE_CLASS_NO_RETURN return javm::core::CreateVoidValue();
}