#include <javm/javm_VM.hpp>

namespace javm::rt {

    namespace {

        std::vector<Ptr<ClassSource>> g_ClassSourceList;

    }

    void AddClassSource(Ptr<ClassSource> cs) {
        g_ClassSourceList.push_back(cs);
    }

    void RemoveClassSource(Ptr<ClassSource> cs) {
        g_ClassSourceList.erase(std::remove(g_ClassSourceList.begin(), g_ClassSourceList.end(), cs), g_ClassSourceList.end()); 
    }

    void ResetClassSources() {
        g_ClassSourceList.clear();
    }

    Ptr<vm::ClassType> LocateClassType(const String &class_name) {
        const auto slash_class_name = vm::MakeSlashClassName(class_name);
        for(const auto &source: g_ClassSourceList) {
            auto class_ptr = source->LocateClassType(slash_class_name);
            if(class_ptr) {
                return class_ptr;
            }
        }

        return nullptr;
    }

    void ResetCachedClassTypes() {
        for(auto &source: g_ClassSourceList) {
            source->ResetCachedClassTypes();
        }
    }

}