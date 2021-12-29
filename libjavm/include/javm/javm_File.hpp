
#pragma once
#include <javm/javm_Base.hpp>

namespace javm {

    // Simple but useful wrapper to read a binary file

    class File {
        private:
            const u8 *file_ptr;
            size_t file_size;
            std::string file_path;
            bool owns_ptr;

            void TryLoad();

        public:
            File() : file_ptr(nullptr), file_size(0), owns_ptr(false) {}

            File(const std::string &path) : file_ptr(nullptr), file_size(0), file_path(path), owns_ptr(false) {
                this->TryLoad();
            }

            File(const u8 *ptr, const size_t ptr_sz, const bool owns = false) : file_ptr(ptr), file_size(ptr_sz), owns_ptr(owns) {}

            virtual ~File() {
                if(this->owns_ptr) {
                    if(this->file_ptr != nullptr) {
                        delete[] this->file_ptr;
                        this->file_ptr = nullptr;
                    }
                }
            }

            inline const u8 *GetFileData() {
                return this->file_ptr;
            }

            inline size_t GetFileSize() {
                return this->file_size;
            }

            inline bool IsValid() {
                return (this->file_ptr != nullptr) && (this->file_size > 0);
            }

            inline std::string GetFilePath() {
                return this->file_path;
            }
    };

}