
#pragma once
#include <javm/rt/rt_JavaClassFileSource.hpp>
#include <javm/javm_ManifestFile.hpp>
#include <andyzip/zipfile_reader.hpp>

namespace javm::rt {

    class JavaArchiveSource : public ClassSource, public File {
        private:
            bool archive_valid;
            String main_class_name;
            std::vector<Ptr<JavaClassFileSource>> cached_class_files;

            inline zipfile_reader OpenSelf() {
                return zipfile_reader(this->GetFileData(), this->GetFileData() + this->GetFileSize());
            }

            void Load() {
                if(this->IsValid()) {
                    try {
                        auto reader = this->OpenSelf();
                        auto v_data = reader.read("META-INF/MANIFEST.MF");
                        ManifestFile manifest(v_data.data(), v_data.size());

                        this->main_class_name = manifest.FindAttribute("Main-Class");
                        this->archive_valid = true;
                    }
                    catch(std::exception&) {
                        this->archive_valid = false;
                    }
                }
            }

        public:
            using File::File;

            JavaArchiveSource(const std::string &path) : File(path), archive_valid(false) {
                this->Load();
            }
            
            JavaArchiveSource(u8 *ptr, const size_t ptr_sz, const bool owns = false) : File(ptr, ptr_sz, owns), archive_valid(false) {
                this->Load();
            }

            inline String GetMainClass() {
                return this->main_class_name;
            }

            inline bool CanBeExecuted() {
                if(!this->IsValid()) {
                    return false;
                }
                return !this->main_class_name.empty();
            }

            inline bool IsArchiveValid() {
                return this->IsValid() && this->archive_valid;
            }

            virtual Ptr<vm::ClassType> LocateClassType(const String &find_class_name) override {
                for(auto &class_file: this->cached_class_files) {
                    if(vm::ClassUtils::EqualClassNames(find_class_name, class_file->GetClassName())) {
                        return class_file->LocateClassType(find_class_name);
                    }
                }
                try {
                    auto reader = this->OpenSelf();
                    auto v_data = reader.read(StrUtils::ToUtf8(vm::ClassUtils::MakeSlashClassName(find_class_name) + u".class"));
                    auto class_src = ptr::New<JavaClassFileSource>(v_data.data(), v_data.size());
                    this->cached_class_files.push_back(class_src);
                    return class_src->LocateClassType(find_class_name);
                }
                catch(std::exception &ex) {}
                return nullptr;
            }

            virtual void ResetCachedClassTypes() override {
                for(auto &cs: this->cached_class_files) {
                    cs->ResetCachedClassTypes();
                }
            }

            virtual std::vector<Ptr<vm::ClassType>> GetClassTypes() override {
                // Create a list with all the types from our sources
                std::vector<Ptr<vm::ClassType>> list;
                for(auto &cs: this->cached_class_files) {
                    const auto cs_types = cs->GetClassTypes();
                    list.insert(list.end(), cs_types.begin(), cs_types.end());
                }
                return list;
            }
    };

}