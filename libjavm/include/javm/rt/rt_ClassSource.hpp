
#pragma once
#include <javm/javm_File.hpp>
#include <javm/vm/vm_Class.hpp>

namespace javm::rt {

    class ClassSource {
        public:
            virtual Ptr<vm::ClassType> LocateClassType(const String &find_class_name) = 0;
            virtual void ResetCachedClassTypes() = 0;
            virtual std::vector<Ptr<vm::ClassType>> GetClassTypes() = 0;
    };

}