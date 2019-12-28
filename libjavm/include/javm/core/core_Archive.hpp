
#pragma once
#include <andyzip/zipfile_reader.hpp>
#include <javm/core/core_ClassFile.hpp>
#include <javm/core/core_ManifestFile.hpp>

namespace javm::core {

    #define JAVM_JAR_MANIFEST_FILE "META-INF/MANIFEST.MF"

    class Archive : public File {

        private:

            bool jar_valid;
            std::string main_class_name;
            std::vector<ClassFile> classes;

            void Load() {
                if(this->IsValid()) {
                    zipfile_reader reader(this->GetFileData(), this->GetFileData() + this->GetFileSize());
                    auto files = reader.filenames();
                    bool manifest_found = true;
                    std::vector<std::string> class_files;
                    for(auto &file: files) {
                        if(file == JAVM_JAR_MANIFEST_FILE) {
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
                        auto u8v = reader.read(JAVM_JAR_MANIFEST_FILE);
                        ManifestFile manifest(u8v.data(), u8v.size());
                        auto main_class = manifest.FindAttribute("Main-Class");
                        this->main_class_name = main_class;

                        for(auto &class_file: class_files) {
                            printf("Loading '%s'...\n", class_file.c_str());
                            auto classv = reader.read(class_file);
                            printf("Class size: %ld\n", classv.size());
                            auto &class_ref = this->classes.emplace_back(&classv[0], classv.size());
                            printf("Loaded...\n");
                        }
                    }
                }
            }

        public:
            using File::File;

            void Initialize() {
                this->jar_valid = false;
                this->Load();
            }

            std::string GetMainClass() {
                return this->main_class_name;
            }

            bool CanBeExecuted() {
                return !this->main_class_name.empty();
            }

            std::vector<ClassFile> &GetLoadedClasses() {
                return this->classes;
            }
    };

}