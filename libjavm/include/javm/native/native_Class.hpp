
#pragma once
#include <javm/core/core_ClassObject.hpp>
#include <map>

namespace javm::native {

    using NativeMethodFunction = std::function<core::Value(core::Frame &frame, core::ThisValues this_v, std::vector<core::FunctionParameter> parameters)>;
    using NativeStaticFunction = std::function<core::Value(core::Frame &frame, std::vector<core::FunctionParameter> parameters)>;

    #define JAVM_NATIVE_DEFINITION_CTOR(clss) \
    static std::shared_ptr<clss> CreateDefinitionInstance(void *machine_ptr) { \
        auto class_ref = std::make_shared<clss>(); \
        auto super_class_name = class_ref->GetSuperClassName(); \
        if(class_ref->GetName() != "java/lang/Object") { \
            auto super_class_ref = javm::core::FindClassByNameEx(machine_ptr, super_class_name); \
            if(super_class_ref) { \
                auto super_class_instance = super_class_ref->CreateInstanceEx(machine_ptr); \
                class_ref->SetSuperClassInstance(super_class_instance); \
            } \
        } \
        return class_ref; \
    }

    #define JAVM_NATIVE_INSTANCE_CTOR(clss) \
    virtual javm::core::Value CreateInstanceEx(void *machine_ptr) override { \
        auto class_holder = javm::core::CreateNewValue<clss>(); \
        auto class_ref = class_holder->GetReference<clss>(); \
        auto super_class_name = class_ref->GetSuperClassName(); \
        if(class_ref->GetName() != "java/lang/Object") { \
            auto super_class_ref = javm::core::FindClassByNameEx(machine_ptr, super_class_name); \
            if(super_class_ref) { \
                auto super_class_instance = super_class_ref->CreateInstanceEx(machine_ptr); \
                class_ref->SetSuperClassInstance(super_class_instance); \
            } \
        } \
        return class_holder; \
    }

    #define JAVM_NATIVE_EXISTING_INSTANCE_CTOR(clss) \
    virtual javm::core::Value CreateFromExistingInstance() override { \
        return javm::core::CreateExistingValue<clss>(this); \
    }

    #define JAVM_NATIVE_CLASS_CTOR(clss) \
    JAVM_NATIVE_DEFINITION_CTOR(clss) \
    JAVM_NATIVE_INSTANCE_CTOR(clss) \
    JAVM_NATIVE_EXISTING_INSTANCE_CTOR(clss) \
    clss()

    class Class : public core::ClassObject {

        private:
            std::string name;
            std::vector<core::CPInfo> stub_empty_pool;
            bool static_done;

        protected:
            std::string super_class_name;
            std::vector<std::string> interfaces;
            std::map<std::string, NativeMethodFunction> methods;
            std::map<std::string, NativeStaticFunction> static_fns;
            std::map<std::string, core::Value> static_fields;
            std::map<std::string, core::Value> member_fields;

            void SetName(const std::string &class_name) {
                this->name = core::ClassObject::ProcessClassName(class_name);
            }

            void AddInterface(const std::string &intf_name) {
                this->interfaces.push_back(intf_name);
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

            virtual std::vector<std::string> GetInterfaceNames() override {
                return this->interfaces;
            }

            virtual std::vector<core::CPInfo> &GetConstantPool() override {
                // Stub the constant pool, this won't be called for a native class anyway
                return this->stub_empty_pool;
            }

            virtual bool HasField(const std::string &name) override {
                return this->member_fields.find(name) != this->member_fields.end();
            }

            virtual bool HasStaticField(const std::string &name) override {
                return this->static_fields.find(name) != this->static_fields.end();
            }
            
            virtual core::Value GetField(const std::string &name) override {
                if(this->member_fields.find(name) != this->member_fields.end()) {
                    return this->member_fields[name];
                }
                return core::CreateInvalidValue();
            }

            virtual core::Value GetStaticField(const std::string &name) override {
                if(this->static_fields.find(name) != this->static_fields.end()) {
                    return this->static_fields[name];
                }
                return core::CreateInvalidValue();
            }

            virtual void SetField(const std::string &name, core::Value value) override {
                if(this->member_fields.find(name) != this->member_fields.end()) {
                    this->member_fields[name] = value;
                }
                else {
                    this->member_fields.insert(std::make_pair(name, value));
                }
            }
            
            virtual void SetStaticField(const std::string &name, core::Value value) override {
                if(this->static_fields.find(name) != this->static_fields.end()) {
                    this->static_fields[name] = value;
                }
                else {
                    this->static_fields.insert(std::make_pair(name, value));
                }
            }

            virtual bool CanHandleMethod(const std::string &name, const std::string &desc, core::Frame &frame) override {
                if(this->methods.find(name) != this->methods.end()) {
                    return true;
                }
                else if(name == JAVM_CTOR_METHOD_NAME) {
                    // Constructors always exist, just that they are empty by default
                    return true;
                }
                return false;
            }

            virtual bool CanHandleStaticFunction(const std::string &name, const std::string &desc, core::Frame &frame) override {
                if(this->static_fns.find(name) != this->static_fns.end()) {
                    return true;
                }
                return false;
            }

            virtual core::Value HandleMethod(const std::string &name, const std::string &desc, core::Value this_v, std::vector<core::Value> params, core::Frame &frame) override {
                auto it = this->methods.find(name);
                if(it != this->methods.end()) {
                    auto param_descs = ClassObject::ParseFunctionDescriptorParameters(desc);
                    std::vector<core::FunctionParameter> fn_params;
                    // The opposite would make no sense...?
                    if(params.size() == param_descs.size()) {
                        for(u32 i = 0; i < params.size(); i++) {
                            core::FunctionParameter fparam = {};
                            fparam.desc = param_descs[i];
                            fparam.value = params[i];
                            fparam.parsed_type = ClassObject::ParseValueType(fparam.desc);
                            fn_params.push_back(fparam);
                        }
                    }
                    
                    // This is the class definition, so it has the class name of the 'this' object to pass to the method
                    auto method_class_name = this->GetName();

                    core::ThisValues this_vs = {};
                    this_vs.invoker = this_v;

                    this_vs.instance = this_vs.invoker;
                    
                    // Iterate through the 'this' invoker's superclasses until we find the 'this' instance we need.
                    // When inheritance isn't involved (calling the methods of the same type, not inherited ones) both will be the same.
                    while(method_class_name != this_vs.instance->GetReference<core::ClassObject>()->GetName()) {
                        auto super_holder = this_vs.instance->GetReference<core::ClassObject>()->GetSuperClassInstance();
                        if(!super_holder) {
                            this_vs.instance = this_vs.invoker;
                            break;
                        }
                        if(!super_holder->IsClassObject()) {
                            this_vs.instance = this_vs.invoker;
                            break;
                        }
                        this_vs.instance = super_holder;
                    }
                    if(method_class_name == this_vs.instance->GetReference<core::ClassObject>()->GetName()) {
                        if(this_vs.instance->GetReference<core::ClassObject>()->CanHandleMethod(name, desc, frame)) {
                            return it->second(frame, this_vs, fn_params);
                        }
                    }
                }
                auto super_class = this->GetSuperClassInstance();
                if(super_class) {
                    if(super_class->IsClassObject()) {
                        auto super_class_ref = super_class->GetReference<core::ClassObject>();
                        return super_class_ref->HandleMethod(name, desc, this_v, params, frame);
                    }
                }
                if(name == JAVM_CTOR_METHOD_NAME) {
                    // Constructors always exist, just that they are empty by default, so return nothing (void) as if an empty method was called.
                    // Anyway, keep this as a last resource
                    return core::CreateVoidValue();
                }
                return core::CreateInvalidValue();
            }

            virtual core::Value HandleStaticFunction(const std::string &name, const std::string &desc, std::vector<core::Value> params, core::Frame &frame) override {
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
                    auto param_descs = ClassObject::ParseFunctionDescriptorParameters(desc);
                    std::vector<core::FunctionParameter> fn_params;
                    // The opposite would make no sense...?
                    if(params.size() == param_descs.size()) {
                        for(u32 i = 0; i < params.size(); i++) {
                            core::FunctionParameter fparam = {};
                            fparam.desc = param_descs[i];
                            fparam.value = params[i];
                            fparam.parsed_type = ClassObject::ParseValueType(fparam.desc);
                            fn_params.push_back(fparam);
                        }
                    }
                    return it->second(frame, fn_params);
                }
                auto super_class = this->GetSuperClassInstance();
                if(super_class) {
                    if(!super_class->IsNull()) {
                        auto super_class_ref = super_class->GetReference<core::ClassObject>();
                        return super_class_ref->HandleStaticFunction(name, desc, params, frame);
                    }
                }
                frame.ThrowWithType("java.lang.RuntimeException", "Invalid static function call - " + name + " - " + this->GetName());
                return core::CreateInvalidValue();
            }

            template<typename C>
            C *GetThisInvokerInstance(core::ThisValues this_v) {
                return this_v.invoker->template GetReference<C>();
            }

            template<typename C>
            C *GetThisInstance(core::ThisValues this_v) {
                return this_v.instance->template GetReference<C>();
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