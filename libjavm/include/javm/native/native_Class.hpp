
#pragma once
#include <javm/core/core_ClassObject.hpp>
#include <map>

namespace javm::native {

    using NativeMethodFunction = std::function<core::ValuePointerHolder(core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters)>;
    using NativeStaticFunction = std::function<core::ValuePointerHolder(std::vector<core::FunctionParameter> parameters)>;

    #define JAVM_NATIVE_DEFINITION_CTOR(clss) \
    public: \
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
    public: \
    virtual javm::core::ValuePointerHolder CreateInstanceEx(void *machine_ptr) override { \
        auto class_holder = javm::core::ValuePointerHolder::Create<clss>(); \
        auto class_ref = class_holder.GetReference<clss>(); \
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
    JAVM_NATIVE_INSTANCE_CTOR(clss)

    class Class : public core::ClassObject {

        private:
            std::string name;
            std::vector<core::CPInfo> stub_empty_pool;
            bool static_done;

        protected:
            std::string super_class_name;
            std::map<std::string, NativeMethodFunction> methods;
            std::map<std::string, NativeStaticFunction> static_fns;
            std::map<std::string, core::ValuePointerHolder> static_fields;
            std::map<std::string, core::ValuePointerHolder> member_fields;

        public:
            Class(std::string class_name) : name(core::ClassObject::ProcessClassName(class_name)), static_done(false) {}

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

            virtual core::ValuePointerHolder GetField(std::string name) override {
                if(this->member_fields.find(name) != this->member_fields.end()) {
                    return this->member_fields[name];
                }
                return core::ValuePointerHolder::CreateVoid();
            }

            virtual core::ValuePointerHolder GetStaticField(std::string name) override {
                if(this->static_fields.find(name) != this->static_fields.end()) {
                    return this->static_fields[name];
                }
                return core::ValuePointerHolder::CreateVoid();
            }

            virtual void SetField(std::string name, core::ValuePointerHolder value) override {
                if(this->member_fields.find(name) != this->member_fields.end()) {
                    this->member_fields[name] = value;
                }
                else {
                    this->member_fields.insert(std::make_pair(name, value));
                }
            }
            
            virtual void SetStaticField(std::string name, core::ValuePointerHolder value) override {
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
                if(this->static_fns.find(name) != this->static_fns.end()) {
                    return true;
                }
                auto super_class = this->GetSuperClassInstance();
                if(super_class.IsClassObject()) {
                    auto super_class_ref = super_class.GetReference<core::ClassObject>();
                    return super_class_ref->CanHandleMethod(name, desc, frame);
                }
                return false;
            }

            virtual core::ValuePointerHolder HandleMethod(std::string name, std::string desc, core::Frame &frame) override {
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
                    core::FunctionParameter this_fparam = {};
                    this_fparam.desc = this->GetName();
                    this_fparam.value = frame.Pop();
                    this_fparam.parsed_type = ClassObject::ParseValueType(this_fparam.desc);
                    return it->second(this_fparam, fn_params);
                }
                auto super_class = this->GetSuperClassInstance();
                if(!super_class.IsNull()) {
                    auto super_class_ref = super_class.GetReference<core::ClassObject>();
                    return super_class_ref->HandleMethod(name, desc, frame);
                }
                return core::ValuePointerHolder::CreateVoid();
            }

            virtual core::ValuePointerHolder HandleStaticFunction(std::string name, std::string desc, core::Frame &frame) override {
                auto it = this->static_fns.find(name);
                if(it != this->static_fns.end()) {
                    if(name == JAVM_STATIC_BLOCK_METHOD_NAME) {
                        if(this->static_done) {
                            // Static block init is already done
                            return core::ValuePointerHolder::CreateVoid();
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
                    return it->second(fn_params);
                }
                auto super_class = this->GetSuperClassInstance();
                if(!super_class.IsNull()) {
                    auto super_class_ref = super_class.GetReference<core::ClassObject>();
                    return super_class_ref->HandleStaticFunction(name, desc, frame);
                }
                return core::ValuePointerHolder::CreateVoid();
            }

            // Gets the proper 'this' - since super classes are stored within the derived class, iterate until we find the correct class
            template<typename C>
            static core::ValuePointerHolder GetThis(core::ValuePointerHolder this_param) {
                if(!this_param.IsValidCast<C>()) {
                    auto class_ref = this_param.GetReference<core::ClassObject>();
                    auto super_class = class_ref->GetSuperClassInstance();
                    if(!super_class.IsNull()) {
                        return GetThis<C>(super_class);
                    }
                }
                return this_param;
            }

            template<typename C>
            static C *GetThisReference(core::ValuePointerHolder this_param) {
                auto holder = GetThis<C>(this_param);
                return holder.template GetReference<C>();
            }
    };

    #define _JAVM_NATIVE_CLASS_REGISTER_METHOD(name_str, name) { \
        javm::native::NativeMethodFunction fn = std::bind(&JAVM_CLASS_TYPE::name, this, std::placeholders::_1, std::placeholders::_2); \
        this->methods.insert(std::make_pair(name_str, fn)); \
    }

    #define _JAVM_NATIVE_CLASS_REGISTER_STATIC_FN(name_str, name) { \
        javm::native::NativeStaticFunction fn = std::bind(&JAVM_CLASS_TYPE::name, this, std::placeholders::_1); \
        this->static_fns.insert(std::make_pair(name_str, fn)); \
    }

    #define JAVM_NATIVE_CLASS_REGISTER_CTOR(name) _JAVM_NATIVE_CLASS_REGISTER_METHOD(JAVM_CTOR_METHOD_NAME, name)
    #define JAVM_NATIVE_CLASS_REGISTER_STATIC_BLOCK(name) _JAVM_NATIVE_CLASS_REGISTER_METHOD(JAVM_STATIC_BLOCK_METHOD_NAME, name)
    #define JAVM_NATIVE_CLASS_REGISTER_METHOD(name) _JAVM_NATIVE_CLASS_REGISTER_METHOD(#name, name)
    #define JAVM_NATIVE_CLASS_REGISTER_STATIC_FN(name) _JAVM_NATIVE_CLASS_REGISTER_STATIC_FN(#name, name)

    #define JAVM_NATIVE_CLASS_NO_RETURN return javm::core::ValuePointerHolder::CreateVoid();
}