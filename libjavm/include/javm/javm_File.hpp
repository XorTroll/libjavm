
#pragma once
#include <javm/javm_Base.hpp>
#include <cstdio>

namespace javm {

    // Simple but useful wrapper to read a binary file

    class File {
        private:
            u8 *file_ptr;
            size_t file_size;
            std::string file_path;
            bool owns_ptr;

        public:
            File() : file_ptr(nullptr), file_size(0), owns_ptr(false) {}

            File(const std::string &path) : file_ptr(nullptr), file_size(0), file_path(path), owns_ptr(false) {
                auto f = fopen(path.c_str(), "rb");
                if(f) {
                    fseek(f, 0, SEEK_END);
                    const auto f_size = static_cast<size_t>(ftell(f));
                    rewind(f);
                    if(f_size > 0) {
                        this->file_ptr = new u8[f_size]();
                        if(fread(this->file_ptr, f_size, 1, f) == 1) {
                            this->file_size = f_size;
                            this->owns_ptr = true;
                        }
                        else {
                            delete[] file_ptr;
                            file_ptr = nullptr;
                        }
                    }
                    fclose(f);
                }
            }

            File(u8 *ptr, const size_t ptr_sz, const bool owns = false) : file_ptr(ptr), file_size(ptr_sz), owns_ptr(owns) {}

            virtual ~File() {
                if(this->owns_ptr) {
                    if(this->file_ptr != nullptr) {
                        delete[] this->file_ptr;
                        this->file_ptr = nullptr;
                    }
                }
            }

            inline u8 *GetFileData() {
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