
#pragma once
#include <javm/javm_Memory.hpp>

namespace javm::core {

    class CodeAttribute {

        private:
            u16 max_stack;
            u16 max_locals;
            u32 code_len;
            u8 *code;

        public:
            CodeAttribute(MemoryReader &reader) : max_stack(0), max_locals(0), code_len(0), code(nullptr) {
                this->max_stack = BE(reader.Read<u16>());
                this->max_locals = BE(reader.Read<u16>());
                this->code_len = BE(reader.Read<u32>());
                if(this->code_len > 0) {
                    this->code = new u8[this->code_len]();
                    reader.ReadPointer(this->code, this->code_len * sizeof(u8));
                }
            }

            void Dispose() {
                if(this->code != nullptr) {
                    delete[] this->code;
                    this->code = nullptr;
                }
            }

            u16 GetMaxLocals() {
                return this->max_locals;
            }

            u16 GetMaxStack() {
                return this->max_stack;
            }

            size_t GetCodeLength() {
                return this->code_len;
            }

            u8 *GetCode() {
                return this->code;
            }
    };

}