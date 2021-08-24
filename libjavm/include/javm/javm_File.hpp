
#pragma once
#include <javm/javm_Base.hpp>
#include <cstdio>

namespace javm {

    // Simple but useful wrapper to read a binary file

    // Unlike the rest of the library, this type uses std::string FWIW

    class File {

        private:
            u8 *file_ptr;
            size_t file_size;
            std::string file_path;
            bool owns_ptr;

        public:
            File() : file_ptr(nullptr), file_size(0), owns_ptr(false) {}

            File(const std::string &path) : file_ptr(nullptr), file_size(0), file_path(path), owns_ptr(false) {
                FILE *f = fopen(path.c_str(), "rb");
                if(f) {
                    fseek(f, 0, SEEK_END);
                    size_t fsize = ftell(f);
                    rewind(f);
                    if(fsize > 0) {
                        this->file_ptr = new u8[fsize]();
                        fread(this->file_ptr, 1, fsize, f);
                        this->file_size = fsize;
                        this->owns_ptr = true;
                    }
                    fclose(f);
                }
            }

            File(u8 *ptr, size_t ptr_sz, bool owns = false) : file_ptr(ptr), file_size(ptr_sz), owns_ptr(owns) {}

            virtual ~File() {
                if(this->owns_ptr) {
                    if(this->file_ptr != nullptr) {
                        delete[] this->file_ptr;
                        this->file_ptr = nullptr;
                    }
                }
            }

            u8 *GetFileData() {
                return this->file_ptr;
            }

            size_t GetFileSize() {
                return this->file_size;
            }

            bool IsValid() {
                if(this->file_size <= 0) {
                    return false;
                }
                return this->file_ptr != nullptr;
            }

            std::string GetFilePath() {
                return this->file_path;
            }
    };

}