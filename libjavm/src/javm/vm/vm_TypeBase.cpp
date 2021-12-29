#include <javm/javm_VM.hpp>

namespace javm::vm {

    namespace {

        const std::map<VariableType, String> PrimitiveTypeNameTable = {
            { VariableType::Byte, u"byte" },
            { VariableType::Boolean, u"boolean" },
            { VariableType::Short, u"short" },
            { VariableType::Character, u"char" },
            { VariableType::Integer, u"int" },
            { VariableType::Long, u"long" },
            { VariableType::Float, u"float" },
            { VariableType::Double, u"double" }
        };

        const std::map<VariableType, String> PrimitiveTypeDescriptorTable = {
            { VariableType::Byte, u"B" },
            { VariableType::Boolean, u"Z" },
            { VariableType::Short, u"S" },
            { VariableType::Character, u"C" },
            { VariableType::Integer, u"I" },
            { VariableType::Long, u"J" },
            { VariableType::Float, u"F" },
            { VariableType::Double, u"D" }
        };

    }

    bool IsPrimitiveType(const String &type_name) {
        for(const auto &[type, name]: PrimitiveTypeNameTable) {
            if(name == type_name) {
                return true;
            }
        }

        auto type_name_copy = type_name;
        while(type_name_copy.front() == u'[') {
            type_name_copy.erase(0, 1);
        }
        for(const auto &[type, name]: PrimitiveTypeDescriptorTable) {
            if(name == type_name_copy) {
                return true;
            }
        }

        return false;
    }

    String GetPrimitiveTypeDescriptor(const VariableType type) {
        if(PrimitiveTypeDescriptorTable.find(type) != PrimitiveTypeDescriptorTable.end()) {
            return PrimitiveTypeDescriptorTable.at(type);
        }

        // TODO: handle this
        return u"<no-desc>";
    }

    String GetPrimitiveTypeName(const VariableType type) {
        if(PrimitiveTypeNameTable.find(type) != PrimitiveTypeNameTable.end()) {
            return PrimitiveTypeNameTable.at(type);
        }

        // TODO: handle this
        return u"<no-name>";
    }

    VariableType GetPrimitiveVariableTypeByName(const String &type_name) {
        for(const auto &[type, name] : PrimitiveTypeNameTable) {
            if(name == type_name) {
                return type;
            }
        }

        return VariableType::Invalid;
    }

    VariableType GetPrimitiveVariableTypeByDescriptor(const String &type_descriptor) {
        for(const auto &[type, descriptor] : PrimitiveTypeDescriptorTable) {
            if(descriptor == type_descriptor) {
                return type;
            }
        }

        return VariableType::Invalid;
    }
    
    VariableType GetVariableTypeByDescriptor(const String &type_descriptor) {
        const auto primitive_type = GetPrimitiveVariableTypeByDescriptor(type_descriptor);
        if(primitive_type != VariableType::Invalid) {
            return primitive_type;
        }

        if((type_descriptor.front() == u'L') && (type_descriptor.back() == u';')) {
            return VariableType::ClassInstance;
        }
        else if(type_descriptor.front() == u'[') {
            return VariableType::Array;
        }

        return VariableType::Invalid;
    }

}