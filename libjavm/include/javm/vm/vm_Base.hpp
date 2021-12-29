
#pragma once
#include <javm/javm_Base.hpp>
#include <javm/javm_String.hpp>

namespace javm::vm {

    enum class AccessFlags : u16 {
        None = 0x0000,
        Public = 0x0001,
        Private = 0x0002,
        Protected = 0x0004,
        Static = 0x0008,
        Final = 0x0010,
        Synchronized = 0x0020,
        Super = 0x0020,
        Volatile = 0x0040,
        Bridge = 0x0040,
        VariableArgs = 0x0080,
        Transient = 0x0080,
        Native = 0x0100,
        Interface = 0x0200,
        Abstract = 0x0400,
        Strict = 0x0800,
        Synthetic = 0x1000,
        Annotation = 0x2000,
        Enum = 0x4000,
        Miranda = 0x8000,
        ReflectMask = 0xFFFF
    };
    JAVM_DEFINE_FLAG_ENUM(AccessFlags, u16)

    struct AttributeName {
        static inline constexpr auto Code = u"Code";
        static inline constexpr auto LineNumberTable = u"LineNumberTable";
        static inline constexpr auto SourceFile = u"SourceFile";
        static inline constexpr auto RuntimeVisibleAnnotations = u"RuntimeVisibleAnnotations";
    };

    struct AnnotationTagType {
        static inline constexpr char Byte = 'B';
        static inline constexpr char Char = 'C';
        static inline constexpr char Float = 'F';
        static inline constexpr char Double = 'D';
        static inline constexpr char Integer = 'I';
        static inline constexpr char Long = 'J';
        static inline constexpr char Short = 'S';
        static inline constexpr char Boolean = 'Z';
        static inline constexpr char String = 's';
        static inline constexpr char Enum = 'e';
        static inline constexpr char Class = 'c';
        static inline constexpr char Annotation = '@';
        static inline constexpr char Array = '[';
    };

}