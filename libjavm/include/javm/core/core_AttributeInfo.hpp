
#pragma once
#include <javm/javm_Memory.hpp>

namespace javm::core {

    // 'Code' attribute contains the actual method's code
    #define JAVM_CODE_ATTRIBUTE_NAME "Code"

    class AttributeInfo {

        private:
            u16 name_index;
            u32 length;
            u8 *info;
            std::string processed_name;

        public:
            AttributeInfo(MemoryReader &reader) : name_index(0), length(0), info(nullptr) {
                this->name_index = BE(reader.Read<u16>());
                this->length = BE(reader.Read<u32>());
                if(this->length > 0) {
                    this->info = new u8[this->length]();
                    reader.ReadPointer(this->info, this->length * sizeof(u8));
                }
            }

            void Dispose() {
                if(this->info != nullptr) {
                    delete[] this->info;
                    this->info = nullptr;
                }
            }

            u16 GetNameIndex() {
                return this->name_index;
            }

            bool HasName() {
                return !this->processed_name.empty();
            }

            std::string GetName() {
                return this->processed_name;
            }

            void SetName(std::string name) {
                this->processed_name = name;
            }

            u8 *GetInfo() {
                return this->info;
            }

            size_t GetInfoLength() {
                return this->length;
            }

    };

}