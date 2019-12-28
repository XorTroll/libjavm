
#pragma once
#include <javm/core/core_ClassObject.hpp>
#include <map>

namespace javm::native {

    using NativeMethodFunction = std::function<core::ValuePointerHolder(core::FunctionParameter this_param, std::vector<core::FunctionParameter> parameters)>;
    using NativeStaticFunction = std::function<core::ValuePointerHolder(std::vector<core::FunctionParameter> parameters)>;

    #define JAVM_NATIVE_CLASS_CTOR(clss) \
    public: \
    virtual javm::core::ValuePointerHolder CreateInstance() override { \
        return javm::core::ValuePointerHolder::Create<clss>(); \
    }

    class Class : public core::ClassObject {

        private:
            std::string name;
            std::vector<core::CPInfo> stub_empty_pool;
            bool static_done;

        protected:
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

            virtual bool CanHandleMethod(std::string name, std::string desc) override {
                if(this->methods.find(name) != this->methods.end()) {
                    return true;
                }
                if(this->static_fns.find(name) != this->static_fns.end()) {
                    return true;
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
                return core::ValuePointerHolder::CreateVoid();
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