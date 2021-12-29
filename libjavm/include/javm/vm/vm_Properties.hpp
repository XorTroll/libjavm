
#pragma once
#include <javm/javm_Base.hpp>

namespace javm::vm {

    using PropertyTable = std::map<String, String>;

    const PropertyTable InitialVmPropertyTable = {
        // TODO: any more initial properties only the VM itself should define?
        { u"java.vm.specification.version", u"1.8" }
    };

    PropertyTable &GetInitialSystemPropertyTable();
    void SetInitialSystemProperty(const String &key, const String &value);

    inline void SetInitialSystemProperties(const PropertyTable &props) {
        for(const auto &[key, value] : props) {
            SetInitialSystemProperty(key, value);
        }
    }

}