#include <javm/rt/rt_JavaArchiveSource.hpp>

namespace javm::rt {

    void ManifestFile::Load() {
        if(this->IsValid()) {
            auto manifest_str = new char[this->GetFileSize() + 1]();
            memcpy(manifest_str, this->GetFileData(), this->GetFileSize());
            std::istringstream strm(manifest_str);
            std::string tmpline;
            while(std::getline(strm, tmpline)) {
                if(tmpline.empty()) {
                    continue;
                }
                if(tmpline.find(':') == std::string::npos) {
                    continue;
                }
                auto attr_name = tmpline.substr(0, tmpline.find_first_of(':'));
                auto attr_value = tmpline.substr(tmpline.find_first_of(':') + 1);
                while(attr_value.front() == ' ') {
                    attr_value.erase(attr_value.begin());
                }
                attr_value.erase(std::remove(attr_value.begin(), attr_value.end(), '\n'), attr_value.end());
                attr_value.erase(std::remove(attr_value.begin(), attr_value.end(), '\r'), attr_value.end());
                this->attributes.insert(std::make_pair(attr_name, attr_value));
            }
            delete[] manifest_str;
        }
    }

    void JavaArchiveSource::Load() {
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

    Ptr<vm::ClassType> JavaArchiveSource::LocateClassType(const String &find_class_name) {
        for(auto &class_file: this->cached_class_files) {
            if(vm::EqualClassNames(find_class_name, class_file->GetClassName())) {
                return class_file->LocateClassType(find_class_name);
            }
        }
        try {
            auto reader = this->OpenSelf();
            auto v_data = reader.read(str::ToUtf8(vm::MakeSlashClassName(find_class_name) + u".class"));
            auto class_src = ptr::New<JavaClassFileSource>(v_data.data(), v_data.size());
            this->cached_class_files.push_back(class_src);
            return class_src->LocateClassType(find_class_name);
        }
        catch(std::exception &ex) {}
        return nullptr;
    }

    void JavaArchiveSource::ResetCachedClassTypes() {
        for(auto &cs: this->cached_class_files) {
            cs->ResetCachedClassTypes();
        }
    }

    std::vector<Ptr<vm::ClassType>> JavaArchiveSource::GetClassTypes() {
        // Create a list with all the types from our sources
        std::vector<Ptr<vm::ClassType>> list;
        for(const auto &cs: this->cached_class_files) {
            const auto cs_types = cs->GetClassTypes();
            list.insert(list.end(), cs_types.begin(), cs_types.end());
        }
        return list;
    }

}