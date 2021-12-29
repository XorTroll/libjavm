#include <javm/javm_VM.hpp>

namespace javm::vm {

    ClassType::ClassType(const String &name, const String &super_name, const String &source_file, const std::vector<String> &interface_names, const std::vector<ClassBaseField> &fields, const std::vector<ClassBaseField> &invokables, const u16 flags, ConstantPool pool) : MonitoredItem(), class_name(name), super_class_name(super_name), source_file(source_file), interface_class_names(interface_names), fields(fields), invokables(invokables), static_block_called(false), static_block_enabled(true), pool(pool) {
        this->SetAccessFlags(flags);
        for(const auto &field: this->fields) {
            if(field.HasFlag<AccessFlags::Static>()) {
                // Push a copy, this kind of field can be got/set
                // Since static fields are used from the class and not the instance, the type needs to hold 'editable' static fields
                this->static_fields.emplace_back(field.GetNameAndType(), field.GetAccessFlags(), field.GetAttributes(), this->pool);
            }
        }
    }

    Ptr<ClassType> ClassType::FindSelf() {
        return rt::LocateClassType(this->class_name);
    }

    Ptr<ClassType> ClassType::GetSuperClassType() {
        if(this->HasSuperClass()) {
            return rt::LocateClassType(this->super_class_name);
        }
        return nullptr;
    }

    type::Integer ClassType::GetRawFieldUnsafeOffset(const String &name, const String &descriptor) {
        u32 static_count = 0;
        for(u32 i = 0; i < this->fields.size(); i++) {
            const auto &field = this->fields.at(i);
            if(field.HasFlag<AccessFlags::Static>()) {
                if((field.GetName() == name) && (field.GetDescriptor() == descriptor)) {
                    return static_count;
                }
                static_count++;
            }
            else {
                if((field.GetName() == name) && (field.GetDescriptor() == descriptor)) {
                    return i - static_count;
                }
            }
        }
        return -1;
    }

    bool ClassType::IsRawFieldStatic(const String &name, const String &descriptor) {
        for(const auto &field: this->fields) {
            if((field.GetName() == name) && (field.GetDescriptor() == descriptor)) {
                return field.HasFlag<AccessFlags::Static>();
            }
        }
        return false;
    }

    ExecutionResult ClassType::EnsureStaticInitializerCalled() {
        // First, call it on our super class (if we have it)
        if(this->HasSuperClass()) {
            auto super_class = this->GetSuperClassType();
            if(super_class) {
                const auto ret = super_class->EnsureStaticInitializerCalled();
                if(ret.IsInvalidOrThrown()) {
                    return ret;
                }
            }
        }

        // Check if we can call the static initializer
        if(this->static_block_enabled) {
            // Call the static initializer if it hasn't been called yet
            if(!this->static_block_called) {
                this->static_block_called = true;

                constexpr auto StaticInitializerMethodName = u"<clinit>";
                constexpr auto StaticInitializerMethodDescriptor = u"()V";
                if(!this->HasClassMethod(StaticInitializerMethodName, StaticInitializerMethodDescriptor)) {
                    return ExecutionResult::Void();
                }

                JAVM_LOG("[clinit] Calling static init of '%s'...", str::ToUtf8(this->class_name).c_str());
                const auto ret = this->CallClassMethod(StaticInitializerMethodName, StaticInitializerMethodDescriptor);
                JAVM_LOG("[clinit] Done '%s'...", str::ToUtf8(this->class_name).c_str());
                return ret;
            }
        }
        return ExecutionResult::Void();
    }

    ExecutionResult ClassType::CallClassMethod(const String &name, const String &descriptor, const std::vector<Ptr<Variable>> &param_vars) {
        // Ensure static initializer is or has been called
        const auto ret = this->EnsureStaticInitializerCalled();
        if(ret.IsInvalidOrThrown()) {
            return ret;
        }
        for(const auto &fn: this->invokables) {
            if(fn.HasFlag<AccessFlags::Static>() && (fn.GetName() == name) && (fn.GetDescriptor() == descriptor)) {
                if(native::HasNativeClassMethod(this->class_name, name, descriptor)) {
                    auto native_fn = native::FindNativeClassMethod(this->class_name, name, descriptor);
                    return native_fn(param_vars);
                }
                else if(fn.HasFlag<AccessFlags::Native>()) {
                    return Throw(u"java/lang/UnsatisfiedLinkError", MakeDotClassName(this->class_name) + u"." + name + descriptor);
                }
                for(const auto &attr: fn.GetAttributes()) {
                    if(attr.GetName() == AttributeName::Code) {
                        auto reader = attr.OpenRead();
                        CodeAttributeData code_attr(reader, this->pool);
                        auto self_type = this->FindSelf();
                        const bool is_sync = fn.HasFlag<AccessFlags::Synchronized>();
                        ExecutionScopeGuard guard(self_type, name, descriptor);
                        if(is_sync) {
                            this->monitor->Enter();
                        }
                        const auto ret = ExecuteStaticCode(code_attr.GetCode(), code_attr.GetMaxLocals(), code_attr.GetExceptionTable(), this->pool, param_vars);
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
        if(this->HasSuperClass()) {
            auto super_class = this->GetSuperClassType();
            if(super_class) {
                return super_class->CallClassMethod(name, descriptor, param_vars);
            }
        }
        return ExecutionResult::InvalidState();
    }

    bool ClassType::HasClassMethod(const String &name, const String &descriptor) {
        for(const auto &fn: this->invokables) {
            if(fn.HasFlag<AccessFlags::Static>() && (fn.GetName() == name) && (fn.GetDescriptor() == descriptor)) {
                return true;
            }
        }

        return false;
    }

    Ptr<Variable> ClassType::GetStaticField(const String &name, const String &descriptor) {
        // Just in case, call the static initializer if it hasn't been called yet
        const auto ret = this->EnsureStaticInitializerCalled();
        if(ret.IsInvalidOrThrown()) {
            return nullptr;
        }

        for(auto &field: this->static_fields) {
            if((field.GetName() == name) && (field.GetDescriptor() == descriptor)) {
                if(field.HasVariable()) {
                    return field.GetVariable();
                }
                else {
                    const auto field_v_type = GetVariableTypeByDescriptor(descriptor);
                    auto default_var = NewDefaultVariable(field_v_type);
                    field.SetVariable(default_var);
                    return default_var;
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

    void ClassType::SetStaticField(const String &name, const String &descriptor, Ptr<Variable> var) {
        // Just in case, call the static initializer if it hasn't been called yet
        const auto ret = this->EnsureStaticInitializerCalled();
        if(ret.IsInvalidOrThrown()) {
            return;
        }

        for(auto &field: this->static_fields) {
            if((field.GetName() == name) && (field.GetDescriptor() == descriptor)) {
                field.SetVariable(var);
            }
        }
    }

    bool ClassType::HasStaticField(const String &name, const String &descriptor) {
        for(const auto &field: this->static_fields) {
            if((field.GetName() == name) && (field.GetDescriptor() == descriptor)) {
                return true;
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

    Ptr<Variable> ClassType::GetStaticFieldByUnsafeOffset(const type::Integer offset) {
        if(offset < this->static_fields.size()) {
            const auto &field = this->static_fields.at(offset);
            return this->GetStaticField(field.GetName(), field.GetDescriptor());
        }

        return nullptr;
    }

    void ClassType::SetStaticFieldByUnsafeOffset(const type::Integer offset, Ptr<Variable> var) {
        if(offset < this->static_fields.size()) {
            const auto &field = this->static_fields.at(offset);
            this->SetStaticField(field.GetName(), field.GetDescriptor(), var);
        }
    }

    bool ClassType::CanCastTo(const String &class_name) {
        if(EqualClassNames(class_name, this->class_name)) {
            return true;
        }

        for(const auto &intf: this->interface_class_names) {
            if(EqualClassNames(class_name, intf)) {
                return true;
            }
        }

        if(this->HasSuperClass()) {
            return this->GetSuperClassType()->CanCastTo(class_name);
        }

        return false;
    }

    LineNumberTable ClassType::GetMethodLineNumberTable(const String &name, const String &descriptor) {
        for(const auto &fn: this->invokables) {
            if((fn.GetName() == name) && (fn.GetDescriptor() == descriptor)) {
                for(const auto &attr: fn.GetAttributes()) {
                    if(attr.GetName() == AttributeName::Code) {
                        auto reader = attr.OpenRead();
                        CodeAttributeData code_attr(reader, this->pool);
                        return code_attr.GetLineNumberTable();
                    }
                }
            }
        }

        if(this->HasSuperClass()) {
            auto super_class = this->GetSuperClassType();
            if(super_class) {
                return super_class->GetMethodLineNumberTable(name, descriptor);
            }
        }
        return {};
    }

    ClassInstance::ClassInstance(Ptr<ClassType> type) : class_type(type) {
        if(type->HasSuperClass()) {
            auto super_class_type = type->GetSuperClassType();
            if(super_class_type) {
                this->super_class_instance = ptr::New<ClassInstance>(super_class_type);
            }
        }
        for(const auto &intf_name: type->GetInterfaceClassNames()) {
            auto intf_type = rt::LocateClassType(intf_name);
            if(intf_type) {
                auto intf_instance = ptr::New<ClassInstance>(intf_type);
                this->interface_instances.push_back(intf_instance);
            }
        }
        for(const auto &field: type->GetFields()) {
            if(!field.HasFlag<AccessFlags::Static>()) {
                // Create non-static get/set fields
                this->member_fields.emplace_back(field.GetNameAndType(), field.GetAccessFlags(), field.GetAttributes(), type->GetConstantPool());
            }
        }
        for(const auto &fn: type->GetInvokables()) {
            if(!fn.HasFlag<AccessFlags::Static>()) {
                this->methods.emplace_back(fn.GetNameAndType(), fn.GetAccessFlags(), fn.GetAttributes(), type->GetConstantPool());
            }
        }
    }

    Ptr<ClassInstance> ClassInstance::GetInstanceByClassType(Ptr<ClassInstance> this_as_obj, const String &class_name) {
        if(EqualClassNames(class_name, this->class_type->GetClassName())) {
            return this_as_obj;
        }
        // Check interfaces - if the class name refers to an interface, then we should be implementing the method
        for(const auto &intf: this->class_type->GetInterfaceClassNames()) {
            if(EqualClassNames(class_name, intf)) {
                return this_as_obj;
            }
        }
        if(this->HasSuperClass()) {
            return this->super_class_instance->GetInstanceByClassType(this->super_class_instance, class_name);
        }
        return nullptr;
    }

    Ptr<ClassInstance> ClassInstance::GetInstanceByClassTypeAndMethodSpecial(Ptr<ClassInstance> this_as_obj, const String &class_name, const String &fn_name, const String &fn_descriptor) {
        // This means that the method belongs to the instance or to an implemented version of it
        if(EqualClassNames(class_name, this->class_type->GetClassName())) {
            // First: check self
            for(const auto &method: this->methods) {
                if((method.GetName() == fn_name) && (method.GetDescriptor() == fn_descriptor) && method.MethodIsInvokable()) {
                    return this_as_obj;
                }
            }

            // Next: check interfaces
            for(const auto &intf: this->interface_instances) {
                for(const auto &intf_method: intf->GetMethods()) {
                    if((intf_method.GetName() == fn_name) && (intf_method.GetDescriptor() == fn_descriptor) && intf_method.MethodIsInvokable()) {
                        return intf;
                    }
                }
            }
        }
        if(this->HasSuperClass()) {
            JAVM_LOG("Super class: '%s'", str::ToUtf8(this->super_class_instance->GetClassType()->GetClassName()).c_str());
            return this->super_class_instance->GetInstanceByClassTypeAndMethodSpecial(this->super_class_instance, class_name, fn_name, fn_descriptor);
        }
        return nullptr;
    }

    Ptr<ClassInstance> ClassInstance::GetInstanceByClassTypeAndMethodVirtualInterface(Ptr<ClassInstance> this_as_obj, const String &class_name, const String &fn_name, const String &fn_descriptor) {
        // First: check self
        for(const auto &method: this->methods) {
            if((method.GetName() == fn_name) && (method.GetDescriptor() == fn_descriptor) && method.MethodIsInvokable()) {
                return this_as_obj;
            }
        }
        // Next: check interfaces
        for(const auto &intf: this->interface_instances) {
            for(const auto &intf_method: intf->GetMethods()) {
                if((intf_method.GetName() == fn_name) && (intf_method.GetDescriptor() == fn_descriptor) && intf_method.MethodIsInvokable()) {
                    return intf;
                }
            }
        }
        if(this->HasSuperClass()) {
            JAVM_LOG("Super class: '%s'", str::ToUtf8(this->super_class_instance->GetClassType()->GetClassName()).c_str());
            return this->super_class_instance->GetInstanceByClassTypeAndMethodVirtualInterface(this->super_class_instance, class_name, fn_name, fn_descriptor);
        }
        return nullptr;
    }

    Ptr<Variable> ClassInstance::GetField(const String &name, const String &descriptor) {
        for(auto &field: this->member_fields) {
            if((field.GetName() == name) && (field.GetDescriptor() == descriptor)) {
                if(field.HasVariable()) {
                    return field.GetVariable();
                }
                else {
                    const auto field_v_type = GetVariableTypeByDescriptor(descriptor);
                    auto default_var = NewDefaultVariable(field_v_type);
                    field.SetVariable(default_var);
                    return default_var;
                }
            }
        }
        if(this->HasSuperClass()) {
            return this->super_class_instance->GetField(name, descriptor);
        }
        return nullptr;
    }

    void ClassInstance::SetField(const String &name, const String &descriptor, Ptr<Variable> var) {
        for(auto &field: this->member_fields) {
            if((field.GetName() == name) && (field.GetDescriptor() == descriptor)) {
                field.SetVariable(var);
                return;
            }
        }

        if(this->HasSuperClass()) {
            return this->super_class_instance->SetField(name, descriptor, var);
        }
    }

    bool ClassInstance::HasField(const String &name, const String &descriptor) {
        for(const auto &field: this->member_fields) {
            if((field.GetName() == name) && (field.GetDescriptor() == descriptor)) {
                return true;
            }
        }

        if(this->HasSuperClass()) {
            return this->super_class_instance->HasField(name, descriptor);
        }

        return false;
    }

    Ptr<Variable> ClassInstance::GetFieldByUnsafeOffset(const type::Integer offset) {
        if(offset < this->member_fields.size()) {
            const auto &field = this->member_fields.at(offset);
            return this->GetField(field.GetName(), field.GetDescriptor());
        }

        return nullptr;
    }

    void ClassInstance::SetFieldByUnsafeOffset(const type::Integer offset, Ptr<Variable> var) {
        if(offset < this->member_fields.size()) {
            const auto &field = this->member_fields.at(offset);
            this->SetField(field.GetName(), field.GetDescriptor(), var);
        }
    }

    ExecutionResult ClassInstance::CallInstanceMethod(const String &name, const String &descriptor, Ptr<Variable> this_as_var, const std::vector<Ptr<Variable>> &param_vars) {
        for(const auto &fn: this->methods) {
            if((fn.GetName() == name) && (fn.GetDescriptor() == descriptor)) {
                const auto class_name = this->class_type->GetClassName();
                if(native::HasNativeInstanceMethod(class_name, name, descriptor)) {
                    auto native_fn = native::FindNativeInstanceMethod(class_name, name, descriptor);
                    return native_fn(this_as_var, param_vars);
                }
                else if(fn.HasFlag<AccessFlags::Native>()) {
                    return Throw(u"java/lang/UnsatisfiedLinkError", MakeDotClassName(this->class_type->GetClassName()) + u"." + name + descriptor);
                }
                for(const auto &attr: fn.GetAttributes()) {
                    if(attr.GetName() == AttributeName::Code) {
                        auto reader = attr.OpenRead();
                        CodeAttributeData code_attr(reader, this->class_type->GetConstantPool());
                        const bool is_sync = fn.HasFlag<AccessFlags::Synchronized>();
                        ExecutionScopeGuard guard(this->class_type, name, descriptor);
                        if(is_sync) {
                            this->monitor->Enter();
                        }
                        const auto ret = ExecuteCode(code_attr.GetCode(), code_attr.GetMaxLocals(), code_attr.GetExceptionTable(), this_as_var, this->class_type->GetConstantPool(), param_vars);
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
        if(this->HasSuperClass()) {
            return this->super_class_instance->CallInstanceMethod(name, descriptor, this_as_var, param_vars);
        }
        return ExecutionResult::InvalidState();
    }

}