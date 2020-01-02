
#pragma once
#include <javm/core/core_CodeAttribute.hpp>
#include <javm/core/core_Values.hpp>
#include <functional>
#include <typeinfo>

namespace javm::core {

    class ClassObject;

    void MachineThrowExceptionWithMessage(void *machine, std::string message);
    void MachineThrowExceptionWithType(void *machine, std::string class_name, std::string message);
    void MachineThrowExceptionWithInstance(void *machine, Value value);

    template<bool CallCtor, typename ...Args>
    Value MachineCreateNewClass(void *machine, std::string name, Args &&...args);

    class Frame {

        private:
            std::vector<Value> stack;
            std::vector<Value> locals;
            CodeAttribute *code_attr;
            Value cur_class_obj;
            size_t offset;
            void *machine;

        public:
            Frame(void *mach) : code_attr(nullptr), cur_class_obj(nullptr), offset(0), machine(mach) {}
            
            Frame(Value cur_class, void *mach) : code_attr(nullptr), cur_class_obj(cur_class), offset(0), machine(mach) {}
            
            Frame(CodeAttribute *code, Value cur_class, void *mach) : code_attr(code), cur_class_obj(cur_class), offset(0), machine(mach) {
                this->locals.push_back(this->cur_class_obj);
                this->locals.reserve(code->GetMaxLocals());
                for(u16 i = 0; i < code->GetMaxLocals(); i++) {
                    this->locals.push_back(CreateNullValue());
                }
            }

            template<typename T, typename ...Args>
            void CreateSetLocal(u32 index, Args &&...args) {
                if(index < this->locals.size()) {
                    this->SetLocal(index, CreateNewValue<T>(args...));
                }
            }
            
            template<typename T>
            void SetLocalReference(u32 index, T *local_var) {
                if(index < this->locals.size()) {
                    this->SetLocal(index, CreateExistingValue<T>(local_var));
                }
            }

            void SetLocal(u32 index, Value local_var) {
                if(index < this->locals.size()) {
                    this->locals[index] = local_var;
                }
            }

            template<typename T>
            T *GetLocalReference(u32 index) {
                if(index >= this->locals.size()) {
                    return nullptr;
                }
                return this->GetLocal(index)->GetReference<T>();
            }

            template<typename T>
            T GetLocalValue(u32 index) {
                if(index >= this->locals.size()) {
                    return T();
                }
                return this->GetLocal(index)->Get<T>();
            }

            Value GetLocal(u32 index) {
                if(index >= this->locals.size()) {
                    return CreateNullValue();
                }
                return this->locals[index];
            }

            void Push(Value var) {
                stack.push_back(var);
                // printf("Pushing pointer: %p\n", var->GetAddress());
            }

            template<typename T, typename ...Args>
            void CreatePush(Args ...args) {
                this->Push(CreateNewValue<T>(args...));
            }

            template<typename T>
            void PushReference(T *var) {
                this->Push(CreateExistingValue<T>(var));
            }

            ClassObject *GetCurrentClass() {
                return this->cur_class_obj->GetReference<ClassObject>();
            }

            Value Pop() { // In this case we don't dispose the value, since Pop is used for holders which will be used again
                auto copy = this->stack.back();
                this->stack.pop_back();
                return copy;
            }

            template<typename T>
            T *PopReference() {
                auto value = this->Pop();
                return value->GetReference<T>();
            }

            template<typename T>
            T PopValue() { // PopValue = dispose and remove last element from stack, and return its value
                auto value = this->Pop();
                T copy = T();
                auto ptr = value->GetReference<T>();
                if(ptr != nullptr) {
                    memcpy(&copy, ptr, sizeof(copy));
                }
                return copy;
            }

            bool StackMaximum() {
                if(this->code_attr == nullptr) {
                    return false;
                }
                return this->stack.size() > (this->code_attr->GetMaxStack() + 1);
            }

            size_t &GetOffset() {
                return this->offset;
            }

            u8 *GetCode() {
                if(this->code_attr == nullptr) {
                    return nullptr;
                }
                return this->code_attr->GetCode();
            }

            template<typename T>
            T *GetCodeAt(size_t offset) {
                auto base_code = (u8*)this->GetCode();
                if(base_code == nullptr) {
                    return nullptr;
                }
                return (T*)&base_code[offset];
            }

            template<typename T>
            T Read(bool forward = true) {
                T t = T();
                auto ref = this->GetCodeAt<T>(this->offset);
                if(ref == nullptr) {
                    return t;
                }
                memcpy(&t, ref, sizeof(T));
                if(forward) {
                    this->offset += sizeof(T);
                }
                return t;
            }

            void ThrowExceptionWithMessage(std::string message) {
                MachineThrowExceptionWithMessage(this->machine, message);
            }

            void ThrowExceptionWithType(std::string class_name, std::string message) {
                MachineThrowExceptionWithType(this->machine, class_name, message);
            }

            void ThrowExceptionWithInstance(Value value) {
                MachineThrowExceptionWithInstance(this->machine, value);
            }

            template<bool CallCtor, typename ...Args>
            Value CreateNewClass(std::string name, Args &&...args) {
                return MachineCreateNewClass<CallCtor>(this->machine, name, args...);
            }

            template<bool CallCtor, typename C, typename ...Args>
            Value CreateNewClassWith(std::string name, std::function<void(C*)> ref_fn, Args &&...args) {
                auto class_val = this->CreateNewClass<CallCtor>(name, args...);
                auto class_ref = class_val->template GetReference<C>();
                ref_fn(class_ref);
                return class_val;
            }

            void *GetMachinePointer() {
                return this->machine;
            }
    };

}