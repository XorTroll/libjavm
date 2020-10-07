
#pragma once
#include <javm/vm/vm_TypeBase.hpp>
#include <javm/vm/vm_Sync.hpp>
#include <javm/vm/vm_Attributes.hpp>
#include <javm/native/native_NativeCode.hpp>
#include <vector>

namespace javm::vm {

    class ClassBaseField : public AccessFlagsItem, public AttributesItem {

        private:
            NameAndTypeData nat_data;

        public:
            ClassBaseField(NameAndTypeData nat, u16 flags, std::vector<AttributeInfo> attrs, ConstantPool &pool) : nat_data(nat) {
                this->SetAccessFlags(flags);
                this->SetAttributes(attrs, pool);
            }

            NameAndTypeData GetNameAndType() {
                return this->nat_data;
            }

            String GetName() {
                return this->nat_data.processed_name;
            }

            String GetDescriptor() {
                return this->nat_data.processed_desc;
            }

            bool MethodIsInvokable() {
                // Is invokable: is native or has Code attribute
                if(this->HasFlag<AccessFlags::Native>()) {
                    return true;
                }
                for(auto &attr: this->GetAttributes()) {
                    if(attr.GetName() == AttributeType::Code) {
                        return true;
                    }
                }
                return false;
            }

    };

    class ClassField : public ClassBaseField {

        private:
            Ptr<Variable> var;

        public:
            using ClassBaseField::ClassBaseField;

            void SetVariable(Ptr<Variable> new_var) {
                this->var = new_var;
            }

            Ptr<Variable> GetVariable() {
                return this->var;
            }

            inline bool HasVariable() {
                return PtrUtils::IsValid(this->var);
            }

    };

    class ClassInvokable : public ClassBaseField {

        public:
            using ClassBaseField::ClassBaseField;

    };

    namespace inner_impl {

        void ThreadNotifyExecutionStartImpl(Ptr<ClassType> type, const String &name, const String &descriptor);
        void ThreadNotifyExecutionEndImpl();
        void ThreadNotifyExceptionThrown();

    }

    class ExecutionScopeGuard {

        public:
            ExecutionScopeGuard(Ptr<ClassType> type, const String &name, const String &descriptor) {
                inner_impl::ThreadNotifyExecutionStartImpl(type, name, descriptor);
            }

            void NotifyThrown() {
                inner_impl::ThreadNotifyExceptionThrown();
            }

            ~ExecutionScopeGuard() {
                inner_impl::ThreadNotifyExecutionEndImpl();
            }

    };

    class MonitoredItem {

        protected:
            Ptr<Monitor> monitor;

        public:
            MonitoredItem() : monitor(PtrUtils::New<Monitor>()) {}

            Ptr<Monitor> GetMonitor() {
                return this->monitor;
            }

    };

    class ClassType : public AccessFlagsItem, public MonitoredItem {

        private:
            String class_name;
            String super_class_name;
            std::vector<String> interface_class_names;
            std::vector<ClassBaseField> fields;
            std::vector<ClassBaseField> invokables;
            std::vector<ClassField> static_fields;
            bool static_block_called;
            bool static_block_enabled;
            ConstantPool pool;

        public:
            ClassType(const String &name, const String &super_name, std::vector<String> interface_names, std::vector<ClassBaseField> fields, std::vector<ClassBaseField> invokables, u16 flags, ConstantPool pool) : MonitoredItem(), class_name(name), super_class_name(super_name), interface_class_names(interface_names), fields(fields), invokables(invokables), static_block_called(false), static_block_enabled(true), pool(pool) {
                this->SetAccessFlags(flags);
                for(auto &field: this->fields) {
                    if(field.HasFlag<AccessFlags::Static>()) {
                        // Push a copy, this kind of field can be got/set
                        // Since static fields are used from the class and not the instance, the type needs to hold 'editable' static fields
                        this->static_fields.emplace_back(field.GetNameAndType(), field.GetAccessFlags(), field.GetAttributes(), this->pool);
                    }
                }
            }

            String GetClassName() {
                return this->class_name;
            }

            String GetSuperClassName() {
                return this->super_class_name;
            }

            bool HasSuperClass() {
                return !this->super_class_name.empty();
            }

            Ptr<ClassType> FindSelf() {
                return inner_impl::LocateClassTypeImpl(this->class_name);
            }

            Ptr<ClassType> GetSuperClassType() {
                if(this->HasSuperClass()) {
                    return inner_impl::LocateClassTypeImpl(this->super_class_name);
                }
                return nullptr;
            }

            std::vector<String> &GetInterfaceClassNames() {
                return this->interface_class_names;
            }

            bool HasInterfaces() {
                return !this->interface_class_names.empty();
            }

            std::vector<ClassBaseField> &GetRawFields() {
                return this->fields;
            }

            std::vector<ClassBaseField> &GetRawInvokables() {
                return this->invokables;
            }

            type::Integer GetRawFieldUnsafeOffset(const String &name, const String &descriptor) {
                u32 static_count = 0;
                for(u32 i = 0; i < this->fields.size(); i++) {
                    auto &field = this->fields[i];
                    if(field.HasFlag<AccessFlags::Static>()) {
                        if(field.GetName() == name) {
                            if(field.GetDescriptor() == descriptor) {
                                return static_count;
                            }
                        }
                        static_count++;
                    }
                    else {
                        if(field.GetName() == name) {
                            if(field.GetDescriptor() == descriptor) {
                                return i - static_count;
                            }
                        }
                    }
                }
                return -1;
            }

            bool IsRawFieldStatic(const String &name, const String &descriptor) {
                for(auto &field: this->fields) {
                    if(field.GetName() == name) {
                        if(field.GetDescriptor() == descriptor) {
                            return field.HasFlag<AccessFlags::Static>();
                        }
                    }
                }
                return false;
            }

            void EnableStaticInitializer() {
                this->static_block_enabled = true;
            }
            
            void DisableStaticInitializer() {
                this->static_block_enabled = false;
            }

            ExecutionResult EnsureStaticInitializerCalled() {
                // First, call it on our super class (if we have it)
                if(this->HasSuperClass()) {
                    auto super_class = this->GetSuperClassType();
                    if(super_class) {
                        auto ret = super_class->EnsureStaticInitializerCalled();
                        if(ret.IsInvalidOrThrown()) {
                            return ret;
                        }
                    }
                }
                // Check if we can call the static initializer
                if(this->static_block_enabled) {
                    // Just in case, call the static initializer if it hasn't been called yet
                    if(!this->static_block_called) {
                        this->static_block_called = true;
                        if(!this->HasClassMethod(u"<clinit>", u"()V")) {
                            return ExecutionResult::Void();
                        }
                        JAVM_LOG("[clinit] Calling static init of '%s'...", StrUtils::ToUtf8(this->class_name).c_str());
                        auto ret = this->CallClassMethod(u"<clinit>", u"()V");
                        JAVM_LOG("[clinit] Done '%s'...", StrUtils::ToUtf8(this->class_name).c_str());
                        return ret;
                    }
                }
                return ExecutionResult::Void();
            }

            std::vector<ClassBaseField> &GetFields() {
                return this->fields;
            }

            std::vector<ClassBaseField> &GetInvokables() {
                return this->invokables;
            }

            ConstantPool &GetConstantPool() {
                return this->pool;
            }

            ExecutionResult CallClassMethod(const String &name, const String &descriptor, std::vector<Ptr<Variable>> param_vars) {
                // Ensure static initializer is or has been called
                auto ret = this->EnsureStaticInitializerCalled();
                if(ret.IsInvalidOrThrown()) {
                    return ret;
                }
                for(auto &fn: this->invokables) {
                    if(fn.HasFlag<AccessFlags::Static>()) {
                        if(fn.GetName() == name) {
                            if(fn.GetDescriptor() == descriptor) {
                                if(native::HasNativeClassMethod(this->class_name, name, descriptor)) {
                                    auto native_fn = native::FindNativeClassMethod(this->class_name, name, descriptor);
                                    return native_fn(param_vars);
                                }
                                else if(fn.HasFlag<AccessFlags::Native>()) {
                                    return inner_impl::ThrowWithTypeAndMessageImpl(u"java/lang/RuntimeException", u"Native class method not implemented: " + this->class_name + u" - " + name + descriptor);
                                }
                                for(auto attr: fn.GetAttributes()) {
                                    if(attr.GetName() == AttributeType::Code) {
                                        MemoryReader reader(attr.GetInfo(), attr.GetInfoLength());
                                        CodeAttributeData code(reader);
                                        auto self_type = this->FindSelf();
                                        const bool is_sync = fn.HasFlag<AccessFlags::Synchronized>();
                                        ExecutionScopeGuard guard(self_type, name, descriptor);
                                        if(is_sync) {
                                            this->monitor->Enter();
                                        }
                                        auto ret = inner_impl::ExecuteStaticCode(code.GetCode(), code.GetMaxLocals(), this->pool, param_vars);
                                        if(is_sync) {
                                            this->monitor->Leave();
                                        }
                                        if(ret.Is<ExecutionStatus::Thrown>()) {
                                            guard.NotifyThrown();
                                        }
                                        return ret;
                                    }
                                }
                            }
                        }
                    }
                }
                if(this->HasSuperClass()) {
                    auto super_class = this->GetSuperClassType();
                    if(super_class) {
                        return super_class->CallClassMethod(name, descriptor, param_vars);
                    }
                }
                return ExecutionResult::InvalidState();
            }

            bool HasClassMethod(const String &name, const String &descriptor) {
                for(auto &fn: this->invokables) {
                    if(fn.HasFlag<AccessFlags::Static>()) {
                        if(fn.GetName() == name) {
                            if(fn.GetDescriptor() == descriptor) {
                                return true;
                            }
                        }
                    }
                }
                return false;
            }

            template<typename ...JArgs>
            ExecutionResult CallClassMethod(const String &name, const String &descriptor, JArgs &&...java_args) {
                std::vector<Ptr<Variable>> param_vars = { std::forward<JArgs>(java_args)... };
                return this->CallClassMethod(name, descriptor, param_vars);
            }

            Ptr<Variable> GetStaticField(const String &name, const String &descriptor) {
                // Just in case, call the static initializer if it hasn't been called yet
                auto ret = this->EnsureStaticInitializerCalled();
                if(ret.IsInvalidOrThrown()) {
                    return nullptr;
                }
                for(auto &field: this->static_fields) {
                    if(field.GetName() == name) {
                        if(field.GetDescriptor() == descriptor) {
                            if(field.HasVariable()) {
                                return field.GetVariable();
                            }
                            else {
                                auto f_field_type = TypeTraits::GetFieldDescriptorFullType(descriptor);
                                auto default_var = inner_impl::NewDefaultVariableImpl(f_field_type.type);
                                field.SetVariable(default_var);
                                return default_var;
                            }
                        }
                    }
                }
                if(this->HasSuperClass()) {
                    auto super_class_type = this->GetSuperClassType();
                    if(super_class_type) {
                        return super_class_type->GetStaticField(name, descriptor);
                    }
                }
                return nullptr;
            }

            void SetStaticField(const String &name, const String &descriptor, Ptr<Variable> var) {
                // Just in case, call the static initializer if it hasn't been called yet
                auto ret = this->EnsureStaticInitializerCalled();
                if(ret.IsInvalidOrThrown()) {
                    return;
                }
                for(auto &field: this->static_fields) {
                    if(field.GetName() == name) {
                        if(field.GetDescriptor() == descriptor) {
                            field.SetVariable(var);
                        }
                    }
                }
            }

            bool HasStaticField(const String &name, const String &descriptor) {
                for(auto &field: this->static_fields) {
                    if(field.GetName() == name) {
                        if(field.GetDescriptor() == descriptor) {
                            return true;
                        }
                    }
                }
                if(this->HasSuperClass()) {
                    auto super_class_type = this->GetSuperClassType();
                    if(super_class_type) {
                        return super_class_type->HasStaticField(name, descriptor);
                    }
                }
                return false;
            }

            Ptr<Variable> GetStaticFieldByUnsafeOffset(type::Integer offset) {
                for(u32 i = 0; i < this->static_fields.size(); i++) {
                    auto &field = this->static_fields[i];
                    if(i == offset) {
                        return this->GetStaticField(field.GetName(), field.GetDescriptor());
                    }
                }
                return nullptr;
            }

            void SetStaticFieldByUnsafeOffset(type::Integer offset, Ptr<Variable> var) {
                for(u32 i = 0; i < this->static_fields.size(); i++) {
                    auto &field = this->static_fields[i];
                    if(i == offset) {
                        this->SetStaticField(field.GetName(), field.GetDescriptor(), var);
                    }
                }
            }

            bool CanCastTo(const String &class_name) {
                if(ClassUtils::EqualClassNames(class_name, this->class_name)) {
                    return true;
                }
                for(auto &intf: this->interface_class_names) {
                    if(ClassUtils::EqualClassNames(class_name, intf)) {
                        return true;
                    }
                }
                if(this->HasSuperClass()) {
                    return this->GetSuperClassType()->CanCastTo(class_name);
                }
                return false;
            }

    };

    class ClassInstance : public MonitoredItem {

        private:
            Ptr<ClassType> class_type;
            Ptr<ClassInstance> super_class_instance;
            std::vector<Ptr<ClassInstance>> interface_instances;
            std::vector<ClassField> member_fields;
            std::vector<ClassInvokable> methods;

        public:
            ClassInstance(Ptr<ClassType> type) : class_type(type) {
                if(type->HasSuperClass()) {
                    auto super_class_type = type->GetSuperClassType();
                    if(super_class_type) {
                        this->super_class_instance = PtrUtils::New<ClassInstance>(super_class_type);
                    }
                }
                for(auto &intf_name: type->GetInterfaceClassNames()) {
                    auto intf_type = inner_impl::LocateClassTypeImpl(intf_name);
                    if(intf_type) {
                        auto intf_instance = PtrUtils::New<ClassInstance>(intf_type);
                        this->interface_instances.push_back(intf_instance);
                    }
                }
                for(auto &field: type->GetFields()) {
                    if(!field.HasFlag<AccessFlags::Static>()) {
                        // Create non-static get/set fields
                        this->member_fields.emplace_back(field.GetNameAndType(), field.GetAccessFlags(), field.GetAttributes(), type->GetConstantPool());
                    }
                }
                for(auto &fn: type->GetInvokables()) {
                    if(!fn.HasFlag<AccessFlags::Static>()) {
                        this->methods.emplace_back(fn.GetNameAndType(), fn.GetAccessFlags(), fn.GetAttributes(), type->GetConstantPool());
                    }
                }
            }

            Ptr<ClassType> GetClassType() {
                return this->class_type;
            }

            bool HasSuperClass() {
                return PtrUtils::IsValid(this->super_class_instance);
            }

            std::vector<ClassInvokable> GetMethods() {
                return this->methods;
            }

            Ptr<ClassInstance> GetSuperClassInstance() {
                return this->super_class_instance;
            }

            Ptr<ClassInstance> GetInstanceByClassType(Ptr<ClassInstance> this_as_obj, const String &class_name) {
                if(ClassUtils::EqualClassNames(class_name, this->class_type->GetClassName())) {
                    return this_as_obj;
                }
                // Check interfaces - if the class name refers to an interface, then we should be implementing the method
                for(auto &intf: this->class_type->GetInterfaceClassNames()) {
                    if(ClassUtils::EqualClassNames(class_name, intf)) {
                        return this_as_obj;
                    }
                }
                if(this->HasSuperClass()) {
                    return this->super_class_instance->GetInstanceByClassType(this->super_class_instance, class_name);
                }
                return nullptr;
            }

            Ptr<ClassInstance> GetInstanceByClassTypeAndMethodSpecial(Ptr<ClassInstance> this_as_obj, const String &class_name, const String &fn_name, const String &fn_descriptor) {
                // This means that the method belongs to the instance or to an implemented version of it
                if(ClassUtils::EqualClassNames(class_name, this->class_type->GetClassName())) {
                    // First: check self
                    for(auto &method: this->methods) {
                        if(method.GetName() == fn_name) {
                            if(method.GetDescriptor() == fn_descriptor) {
                                if(method.MethodIsInvokable()) {
                                    return this_as_obj;
                                }
                            }
                        }
                    }
                    // Next: check interfaces
                    for(auto &intf: this->interface_instances) {
                        for(auto &intf_method: intf->GetMethods()) {
                            if(intf_method.GetName() == fn_name) {
                                if(intf_method.GetDescriptor() == fn_descriptor) {
                                    if(intf_method.MethodIsInvokable()) {
                                        return intf;
                                    }
                                }
                            }
                        }
                    }
                }
                if(this->HasSuperClass()) {
                    JAVM_LOG("Super class: '%s'", StrUtils::ToUtf8(this->super_class_instance->GetClassType()->GetClassName()).c_str());
                    return this->super_class_instance->GetInstanceByClassTypeAndMethodSpecial(this->super_class_instance, class_name, fn_name, fn_descriptor);
                }
                return nullptr;
            }

            Ptr<ClassInstance> GetInstanceByClassTypeAndMethodVirtualInterface(Ptr<ClassInstance> this_as_obj, const String &class_name, const String &fn_name, const String &fn_descriptor) {
                // First: check self
                for(auto &method: this->methods) {
                    if(method.GetName() == fn_name) {
                        if(method.GetDescriptor() == fn_descriptor) {
                            if(method.MethodIsInvokable()) {
                                return this_as_obj;
                            }
                        }
                    }
                }
                // Next: check interfaces
                for(auto &intf: this->interface_instances) {
                    for(auto &intf_method: intf->GetMethods()) {
                        
                        if(intf_method.GetName() == fn_name) {
                            if(intf_method.GetDescriptor() == fn_descriptor) {
                                if(intf_method.MethodIsInvokable()) {
                                    return intf;
                                }
                            }
                        }
                    }
                }
                if(this->HasSuperClass()) {
                    JAVM_LOG("Super class: '%s'", StrUtils::ToUtf8(this->super_class_instance->GetClassType()->GetClassName()).c_str());
                    return this->super_class_instance->GetInstanceByClassTypeAndMethodVirtualInterface(this->super_class_instance, class_name, fn_name, fn_descriptor);
                }
                return nullptr;
            }

            Ptr<Variable> GetField(const String &name, const String &descriptor) {
                for(auto &field: this->member_fields) {
                    if(field.GetName() == name) {
                        if(field.GetDescriptor() == descriptor) {
                            if(field.HasVariable()) {
                                return field.GetVariable();
                            }
                            else {
                                auto f_field_type = TypeTraits::GetFieldDescriptorFullType(descriptor);
                                auto default_var = inner_impl::NewDefaultVariableImpl(f_field_type.type);
                                field.SetVariable(default_var);
                                return default_var;
                            }
                        }
                    }
                }
                if(this->HasSuperClass()) {
                    return this->super_class_instance->GetField(name, descriptor);
                }
                return nullptr;
            }

            void SetField(const String &name, const String &descriptor, Ptr<Variable> var) {
                for(auto &field: this->member_fields) {
                    if(field.GetName() == name) {
                        if(field.GetDescriptor() == descriptor) {
                            field.SetVariable(var);
                            return;
                        }
                    }
                }
                if(this->HasSuperClass()) {
                    return this->super_class_instance->SetField(name, descriptor, var);
                }
            }

            bool HasField(const String &name, const String &descriptor) {
                for(auto &field: this->member_fields) {
                    if(field.GetName() == name) {
                        if(field.GetDescriptor() == descriptor) {
                            return true;
                        }
                    }
                }
                if(this->HasSuperClass()) {
                    return this->super_class_instance->HasField(name, descriptor);
                }
                return false;
            }

            Ptr<Variable> GetFieldByUnsafeOffset(type::Integer offset) {
                for(u32 i = 0; i < this->member_fields.size(); i++) {
                    auto &field = this->member_fields[i];
                    if(i == offset) {
                        return this->GetField(field.GetName(), field.GetDescriptor());
                    }
                }
                return nullptr;
            }

            void SetFieldByUnsafeOffset(type::Integer offset, Ptr<Variable> var) {
                for(u32 i = 0; i < this->member_fields.size(); i++) {
                    auto &field = this->member_fields[i];
                    if(i == offset) {
                        this->SetField(field.GetName(), field.GetDescriptor(), var);
                    }
                }
            }

            ExecutionResult CallInstanceMethod(const String &name, const String &descriptor, Ptr<Variable> this_as_var, std::vector<Ptr<Variable>> param_vars) {
                for(auto &fn: this->methods) {
                    if(fn.GetName() == name) {
                        if(fn.GetDescriptor() == descriptor) {
                            auto class_name = this->class_type->GetClassName();
                            if(native::HasNativeInstanceMethod(class_name, name, descriptor)) {
                                auto native_fn = native::FindNativeInstanceMethod(class_name, name, descriptor);
                                return native_fn(this_as_var, param_vars);
                            }
                            else if(fn.HasFlag<AccessFlags::Native>()) {
                                return inner_impl::ThrowWithTypeAndMessageImpl(u"java/lang/RuntimeException", u"Native instance method not implemented: " + this->class_type->GetClassName() + u" - " + name + descriptor);
                            }
                            for(auto attr: fn.GetAttributes()) {
                                if(attr.GetName() == AttributeType::Code) {
                                    MemoryReader reader(attr.GetInfo(), attr.GetInfoLength());
                                    CodeAttributeData code(reader);
                                    const bool is_sync = fn.HasFlag<AccessFlags::Synchronized>();
                                    ExecutionScopeGuard guard(this->class_type, name, descriptor);
                                    if(is_sync) {
                                        this->monitor->Enter();
                                    }
                                    auto ret = inner_impl::ExecuteCode(code.GetCode(), code.GetMaxLocals(), this_as_var, this->class_type->GetConstantPool(), param_vars);
                                    if(is_sync) {
                                        this->monitor->Leave();
                                    }
                                    if(ret.Is<ExecutionStatus::Thrown>()) {
                                        guard.NotifyThrown();
                                    }
                                    return ret;
                                }
                            }
                        }
                    }
                }
                if(this->HasSuperClass()) {
                    return this->super_class_instance->CallInstanceMethod(name, descriptor, this_as_var, param_vars);
                }
                return ExecutionResult::InvalidState();
            }

            template<typename ...JArgs>
            ExecutionResult CallInstanceMethod(const String &name, const String &descriptor, Ptr<Variable> this_as_var, JArgs &&...java_args) {
                std::vector<Ptr<Variable>> param_vars = { std::forward<JArgs>(java_args)... };
                return this->CallInstanceMethod(name, descriptor, this_as_var, param_vars);
            }

            template<typename ...JArgs>
            inline ExecutionResult CallConstructor(Ptr<Variable> this_as_var, const String &descriptor, JArgs &&...java_args) {
                return this->CallInstanceMethod(u"<init>", descriptor, this_as_var, java_args...);
            }

    };

}