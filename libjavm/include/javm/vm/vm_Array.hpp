
#pragma once
#include <javm/javm_Base.hpp>
#include <javm/vm/vm_Class.hpp>

namespace javm::vm {

    // Note: the inner object inside an array is only used to call Object methods and for its monitor

    class Array {
        private:
            VariableType type;
            Ptr<ClassType> class_type;
            std::vector<Ptr<Variable>> inner_array;
            u32 length;
            u32 dimensions;
            Ptr<ClassInstance> inner_object;

            Ptr<Variable> MakeDefaultVariable();
            static Ptr<ClassInstance> CreateInnerObject();

        public:
            Array(VariableType type, const u32 length, const u32 dimensions = 1) : type(type), inner_array(length), length(length), dimensions(dimensions), inner_object(CreateInnerObject()) {}

            Array(Ptr<ClassType> type, const u32 length, const u32 dimensions = 1) : type(VariableType::ClassInstance), class_type(type), inner_array(length), length(length), dimensions(dimensions), inner_object(CreateInnerObject()) {}

            inline VariableType GetVariableType() {
                return this->type;
            }

            inline bool IsClassInstanceArray() {
                return ptr::IsValid(this->class_type);
            }

            inline bool IsMultiArray() {
                return this->dimensions > 1;
            }

            inline Ptr<ClassType> GetClassType() {
                return this->class_type;
            }

            inline Ptr<ClassInstance> GetObjectInstance() {
                return this->inner_object;
            }

            bool CanCastTo(const String &class_name);

            inline u32 GetLength() {
                return this->length;
            }

            inline u32 GetDimensions() {
                return this->dimensions;
            }

            Ptr<Variable> GetAt(const u32 idx);
            bool SetAt(const u32 idx, Ptr<Variable> var);

            inline ExecutionResult CallInstanceMethod(const String &name, const String &descriptor, Ptr<Variable> this_as_var, const std::vector<Ptr<Variable>> &param_vars) {
                return this->inner_object->CallInstanceMethod(name, descriptor, this_as_var, param_vars);
            }
    };

}