
#pragma once
#include <andyzip/zipfile_reader.hpp>
#include <javm/core/core_ClassFile.hpp>
#include <javm/core/core_ManifestFile.hpp>
#include <memory>

namespace javm::core {

    #define JAVM_ARCHIVE_MANIFEST_FILE "META-INF/MANIFEST.MF"

    #define JAVM_ARCHIVE_MAIN_CLASS_ATTRIBUTE "Main-Class"

    class Archive : public File {

        private:

            bool jar_valid;
            std::string main_class_name;
            std::vector<std::unique_ptr<ClassFile>> classes;

            void Load() {
                if(this->IsValid()) {
                    zipfile_reader reader(this->GetFileData(), this->GetFileData() + this->GetFileSize());
                    auto files = reader.filenames();
                    bool manifest_found = true;
                    std::vector<std::string> class_files;
                    for(auto &file: files) {
                        if(file == JAVM_ARCHIVE_MANIFEST_FILE) {
                            manifest_found = true;
                        }
                        else {
                            if(file.length() > 6) {
                                if(file.substr(file.length() - 6) == ".class") {
                                    class_files.push_back(file);
                                }
                            }
                        }
                    }
                    if(manifest_found) {
                        auto u8v = reader.read(JAVM_ARCHIVE_MANIFEST_FILE);
                        ManifestFile manifest(u8v.data(), u8v.size());
                        auto main_class = manifest.FindAttribute(JAVM_ARCHIVE_MAIN_CLASS_ATTRIBUTE);
                        this->main_class_name = main_class;

                        for(auto &class_file: class_files) {
                            auto classv = reader.read(class_file);
                            this->classes.push_back(std::make_unique<ClassFile>(&classv[0], classv.size()));
                        }
                    }
                }
            }

        public:
            using File::File;

            Archive(std::string path) : File(path) {
                this->Load();
            }
            
            Archive(u8 *ptr, size_t ptr_sz, bool owns = false) : File(ptr, ptr_sz, owns) {
                this->Load();
            }

            std::string GetMainClass() {
                return this->main_class_name;
            }

            bool CanBeExecuted() {
                return !this->main_class_name.empty();
            }

            std::vector<std::unique_ptr<ClassFile>> &GetLoadedClasses() {
                return this->classes;
            }
    };

}