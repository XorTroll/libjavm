
#pragma once
#include <javm/vm/vm_TypeBase.hpp>
#include <javm/vm/vm_Sync.hpp>
#include <javm/vm/vm_Attributes.hpp>
#include <javm/native/native_NativeCode.hpp>

namespace javm::vm {

    class ClassBaseField : public AccessFlagsItem, public AttributesItem {
        private:
            NameAndTypeData nat_data;

        public:
            ClassBaseField(const NameAndTypeData nat, const u16 flags, const std::vector<AttributeInfo> &attrs, ConstantPool &pool) : nat_data(nat) {
                this->SetAccessFlags(flags);
                this->SetAttributes(attrs, pool);
            }

            inline NameAndTypeData GetNameAndType() const {
                return this->nat_data;
            }

            inline String GetName() const {
                return this->nat_data.processed_name;
            }

            inline String GetDescriptor() const {
                return this->nat_data.processed_desc;
            }

            inline bool MethodIsInvokable() const {
                // Is invokable: is native or has Code attribute
                if(this->HasFlag<AccessFlags::Native>()) {
                    return true;
                }

                for(const auto &attr: this->GetAttributes()) {
                    if(attr.GetName() == AttributeName::Code) {
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

            inline void SetVariable(Ptr<Variable> new_var) {
                this->var = new_var;
            }

            inline Ptr<Variable> GetVariable() const {
                return this->var;
            }

            inline bool HasVariable() {
                return ptr::IsValid(this->var);
            }
    };

    class ClassInvokable : public ClassBaseField {
        public:
            using ClassBaseField::ClassBaseField;
    };

    class MonitoredItem {
        protected:
            Ptr<Monitor> monitor;

        public:
            MonitoredItem() : monitor(ptr::New<Monitor>()) {}

            inline Ptr<Monitor> GetMonitor() {
                return this->monitor;
            }
    };

    class ClassType : public AccessFlagsItem, public MonitoredItem {
        private:
            String class_name;
            String super_class_name;
            String source_file;
            std::vector<String> interface_class_names;
            std::vector<ClassBaseField> fields;
            std::vector<ClassBaseField> invokables;
            std::vector<ClassField> static_fields;
            bool static_block_called;
            bool static_block_enabled;
            ConstantPool pool;

        public:
            ClassType(const String &name, const String &super_name, const String &source_file, const std::vector<String> &interface_names, const std::vector<ClassBaseField> &fields, const std::vector<ClassBaseField> &invokables, const u16 flags, ConstantPool pool);

            inline String GetClassName() {
                return this->class_name;
            }

            inline String GetSuperClassName() {
                return this->super_class_name;
            }

            inline String GetSourceFile() {
                return this->source_file;
            }

            inline bool HasSuperClass() {
                return !this->super_class_name.empty();
            }

            Ptr<ClassType> FindSelf();
            Ptr<ClassType> GetSuperClassType();

            inline std::vector<String> &GetInterfaceClassNames() {
                return this->interface_class_names;
            }

            inline bool HasInterfaces() {
                return !this->interface_class_names.empty();
            }

            inline std::vector<ClassBaseField> &GetRawFields() {
                return this->fields;
            }

            inline std::vector<ClassBaseField> &GetRawInvokables() {
                return this->invokables;
            }

            type::Integer GetRawFieldUnsafeOffset(const String &name, const String &descriptor);
            bool IsRawFieldStatic(const String &name, const String &descriptor);

            inline void EnableStaticInitializer() {
                this->static_block_enabled = true;
            }
            
            inline void DisableStaticInitializer() {
                this->static_block_enabled = false;
            }

            ExecutionResult EnsureStaticInitializerCalled();

            inline std::vector<ClassBaseField> &GetFields() {
                return this->fields;
            }

            inline std::vector<ClassBaseField> &GetInvokables() {
                return this->invokables;
            }

            inline ConstantPool &GetConstantPool() {
                return this->pool;
            }

            ExecutionResult CallClassMethod(const String &name, const String &descriptor, const std::vector<Ptr<Variable>> &param_vars);
            bool HasClassMethod(const String &name, const String &descriptor);

            template<typename ...JArgs>
            inline ExecutionResult CallClassMethod(const String &name, const String &descriptor, JArgs &&...java_args) {
                const std::vector<Ptr<Variable>> param_vars = { std::forward<JArgs>(java_args)... };
                return this->CallClassMethod(name, descriptor, param_vars);
            }

            Ptr<Variable> GetStaticField(const String &name, const String &descriptor);
            void SetStaticField(const String &name, const String &descriptor, Ptr<Variable> var);
            bool HasStaticField(const String &name, const String &descriptor);

            Ptr<Variable> GetStaticFieldByUnsafeOffset(const type::Integer offset);
            void SetStaticFieldByUnsafeOffset(const type::Integer offset, Ptr<Variable> var);

            bool CanCastTo(const String &class_name);

            LineNumberTable GetMethodLineNumberTable(const String &name, const String &descriptor);
    };

    class ClassInstance : public MonitoredItem {
        private:
            Ptr<ClassType> class_type;
            Ptr<ClassInstance> super_class_instance;
            std::vector<Ptr<ClassInstance>> interface_instances;
            std::vector<ClassField> member_fields;
            std::vector<ClassInvokable> methods;

        public:
            ClassInstance(Ptr<ClassType> type);

            inline Ptr<ClassType> GetClassType() {
                return this->class_type;
            }

            inline bool HasSuperClass() {
                return ptr::IsValid(this->super_class_instance);
            }

            inline std::vector<ClassInvokable> GetMethods() {
                return this->methods;
            }

            inline Ptr<ClassInstance> GetSuperClassInstance() {
                return this->super_class_instance;
            }

            Ptr<ClassInstance> GetInstanceByClassType(Ptr<ClassInstance> this_as_obj, const String &class_name);
            Ptr<ClassInstance> GetInstanceByClassTypeAndMethodSpecial(Ptr<ClassInstance> this_as_obj, const String &class_name, const String &fn_name, const String &fn_descriptor);
            Ptr<ClassInstance> GetInstanceByClassTypeAndMethodVirtualInterface(Ptr<ClassInstance> this_as_obj, const String &class_name, const String &fn_name, const String &fn_descriptor);

            Ptr<Variable> GetField(const String &name, const String &descriptor);
            void SetField(const String &name, const String &descriptor, Ptr<Variable> var);
            bool HasField(const String &name, const String &descriptor);

            Ptr<Variable> GetFieldByUnsafeOffset(const type::Integer offset);
            void SetFieldByUnsafeOffset(const type::Integer offset, Ptr<Variable> var);

            ExecutionResult CallInstanceMethod(const String &name, const String &descriptor, Ptr<Variable> this_as_var, const std::vector<Ptr<Variable>> &param_vars);

            template<typename ...JArgs>
            inline ExecutionResult CallInstanceMethod(const String &name, const String &descriptor, Ptr<Variable> this_as_var, JArgs &&...java_args) {
                const std::vector<Ptr<Variable>> param_vars = { std::forward<JArgs>(java_args)... };
                return this->CallInstanceMethod(name, descriptor, this_as_var, param_vars);
            }

            template<typename ...JArgs>
            inline ExecutionResult CallConstructor(Ptr<Variable> this_as_var, const String &descriptor, JArgs &&...java_args) {
                return this->CallInstanceMethod(u"<init>", descriptor, this_as_var, java_args...);
            }
    };

}