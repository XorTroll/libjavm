
#pragma once
#include <javm/javm_Base.hpp>
#include <cstring>

namespace javm {

    class MemoryReader {
        private:
            const u8 *inner_ptr;
            size_t ptr_size;
            size_t offset;

        public:
            MemoryReader(const u8 *ptr, const size_t len) : inner_ptr(ptr), ptr_size(len), offset(0) {}

            template<typename T>
            T Read(bool forward = true) {
                T t = *(T*)(&this->inner_ptr[this->offset]);
                if(forward) {
                    this->offset += sizeof(T);
                }
                return t;
            }

            template<typename T>
            void ReadPointer(T *ptr, const size_t size_bytes, const bool forward = true) {
                memcpy(ptr, &this->inner_ptr[this->offset], size_bytes);
                if(forward) {
                    this->offset += size_bytes;
                }
            }

            inline size_t GetOffset() {
                return this->offset;
            }
    };

}