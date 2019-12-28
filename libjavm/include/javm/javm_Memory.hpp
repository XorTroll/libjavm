
#pragma once
#include <javm/javm_Base.hpp>
#include <cstring>

namespace javm {

    class MemoryReader {

        private:
            u8 *inner_ptr;
            size_t ptr_size;
            size_t offset;

        public:
            MemoryReader(u8 *ptr, size_t len) : inner_ptr(ptr), ptr_size(len), offset(0) {}

            template<typename T>
            T Read(bool forward = true) {
                T t = T();
                memcpy(&t, &this->inner_ptr[this->offset], sizeof(T));
                if(forward) {
                    this->offset += sizeof(T);
                }
                return t;
            }

            template<typename T>
            void ReadPointer(T *ptr, size_t size_bytes, bool forward = true) {
                memcpy(ptr, &this->inner_ptr[this->offset], size_bytes);
                if(forward) {
                    this->offset += size_bytes;
                }
            }

    };
}