
#pragma once
#include <javm/rt/rt_ClassSource.hpp>

namespace javm::rt {

    void AddClassSource(Ptr<ClassSource> cs);
    void RemoveClassSource(Ptr<ClassSource> cs);
    void ResetClassSources();

    template<typename CS, typename ...Args>
    inline Ptr<CS> CreateAddClassSource(Args &&...args) {
        static_assert(std::is_base_of_v<ClassSource, CS>, "Invalid class source");
        auto cs = ptr::New<CS>(args...);
        AddClassSource(cs);
        return cs;
    }

    Ptr<vm::ClassType> LocateClassType(const String &class_name);
    void ResetCachedClassTypes();

}