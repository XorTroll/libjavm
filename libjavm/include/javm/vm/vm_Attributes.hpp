
#pragma once
#include <javm/javm_Memory.hpp>
#include <javm/vm/vm_ConstantPool.hpp>

namespace javm::vm {

    class AttributeInfo : public ConstantNameItem {
        private:
            u32 length;
            u8 *attribute_data;

        public:
            AttributeInfo(MemoryReader &reader) : length(0), attribute_data(nullptr) {
                this->SetNameIndex(BE(reader.Read<u16>()));
                this->length = BE(reader.Read<u32>());
                if(this->length > 0) {
                    this->attribute_data = new u8[this->length]();
                    reader.ReadPointer(this->attribute_data, this->length);
                }
            }

            void Dispose() {
                if(this->attribute_data != nullptr) {
                    delete[] this->attribute_data;
                    this->attribute_data = nullptr;
                }
            }

            inline u8 *GetInfo() const {
                return this->attribute_data;
            }

            inline size_t GetInfoLength() const {
                return this->length;
            }
    };

    struct ExceptionTableEntry {
        u16 start_pc;
        u16 end_pc;
        u16 handler_pc;
        u16 catch_exc_type_index;
    };

    class CodeAttributeData {
        private:
            u16 max_stack;
            u16 max_locals;
            u32 code_len;
            u8 *code;
            std::vector<ExceptionTableEntry> exc_table;

        public:
            CodeAttributeData(MemoryReader &reader) : max_stack(0), max_locals(0), code_len(0), code(nullptr) {
                this->max_stack = BE(reader.Read<u16>());
                this->max_locals = BE(reader.Read<u16>());
                this->code_len = BE(reader.Read<u32>());
                if(this->code_len > 0) {
                    this->code = new u8[this->code_len]();
                    reader.ReadPointer(this->code, this->code_len);
                }

                const auto exc_table_len = BE(reader.Read<u16>());
                this->exc_table.reserve(exc_table_len);
                for(u32 i = 0; i < exc_table_len; i++) {
                    const ExceptionTableEntry exc_table_entry = {
                        .start_pc = BE(reader.Read<u16>()),
                        .end_pc = BE(reader.Read<u16>()),
                        .handler_pc = BE(reader.Read<u16>()),
                        .catch_exc_type_index = BE(reader.Read<u16>())
                    };
                    this->exc_table.push_back(exc_table_entry);
                }
            }

            ~CodeAttributeData() {
                if(this->code != nullptr) {
                    delete[] this->code;
                    this->code = nullptr;
                }
            }

            inline u16 GetMaxLocals() {
                return this->max_locals;
            }

            inline u16 GetMaxStack() {
                return this->max_stack;
            }

            inline size_t GetCodeLength() {
                return this->code_len;
            }

            inline u8 *GetCode() {
                return this->code;
            }

            inline std::vector<ExceptionTableEntry> &GetExceptionTable() {
                return this->exc_table;
            }
    };

    struct ConstAnnotationValue {
        u16 const_value_index;
        type::Integer processed_int;
        type::Float processed_flt;
        type::Double processed_dbl;
        type::Long processed_lng;
        String processed_str;
    };
    
    struct EnumConstAnnotationValue {
        u16 type_name_index;
        String processed_type_name;
        u16 const_name_index;
        String processed_const_name;
    };

    struct ClassInfoAnnotationValue {
        u16 class_info_index;
    };

    struct AnnotationAnnotationValue {
        u16 type_index;
        String processed_type;
    };

    struct Value {
        u16 name_index;
        String processed_name;
        u8 tag;
        ConstAnnotationValue const_value;
        EnumConstAnnotationValue enum_const_value;
        ClassInfoAnnotationValue class_info_value;
        AnnotationAnnotationValue annotation_value;
    };

    struct Annotation {
        u16 type_index;
        String processed_type;
        std::vector<Value> values;
    };

    class RuntimeAnnotationsAttributeData {
        private:
            u32 len;
            std::vector<Annotation> annotations;
            u16 type_index;
            String type;

            Annotation ReadAnnotationImpl(MemoryReader &reader) {
                Annotation annot = {};
                annot.type_index = BE(reader.Read<u16>());
                const auto val_count = BE(reader.Read<u16>());
                JAVM_LOG("[annotations] Value count: %d", val_count);
                for(u16 i = 0; i < val_count; i++) {
                    Value val = {};
                    val.name_index = BE(reader.Read<u16>());
                    val.tag = reader.Read<u8>();
                    JAVM_LOG("[annotations] Reading annotation of tag '%c'...", (char)val.tag);
                    switch(static_cast<char>(val.tag)) {
                        case AnnotationTagType::Byte: 
                        case AnnotationTagType::Char: 
                        case AnnotationTagType::Float: 
                        case AnnotationTagType::Double: 
                        case AnnotationTagType::Integer: 
                        case AnnotationTagType::Short: 
                        case AnnotationTagType::Boolean: 
                        case AnnotationTagType::Long: 
                        case AnnotationTagType::String: {
                            val.const_value.const_value_index = BE(reader.Read<u16>());
                            break;
                        }
                        case AnnotationTagType::Enum: {
                            val.enum_const_value.type_name_index = BE(reader.Read<u16>());
                            val.enum_const_value.const_name_index = BE(reader.Read<u16>());
                            break;
                        }
                        case AnnotationTagType::Class: {
                            val.class_info_value.class_info_index = BE(reader.Read<u16>());
                            break;
                        }
                        case AnnotationTagType::Annotation: {
                            JAVM_LOG("[annotations] whoops - annot inside annot...");
                            auto annot = ReadAnnotationImpl(reader);
                            break;
                        }
                        default: {
                            // TODO
                            JAVM_LOG("[annotations] Unhandled type: '%c'...", (char)val.tag);
                            break;
                        }
                    }
                }
                return annot;
            }

        public:
            RuntimeAnnotationsAttributeData(MemoryReader &reader) {
                const auto annot_count = BE(reader.Read<u16>());
                JAVM_LOG("[annotations] Annotation count: %d", annot_count);
                for(u16 i = 0; i < annot_count; i++) {
                    auto annot = ReadAnnotationImpl(reader);
                    this->annotations.push_back(annot);
                }
            }

            void ProcessAnnotations(ConstantPool &pool) {
                for(auto &annot: this->annotations) {
                    auto type_data_item = pool.GetItemAt(annot.type_index, vm::ConstantPoolTag::Utf8);
                    if(type_data_item) {
                        auto &type_data = type_data_item->GetUtf8Data();
                        annot.processed_type = StrUtils::FromUtf8(type_data.utf8_str);
                        JAVM_LOG("[annotations] Annotation name: '%s'...", StrUtils::ToUtf8(annot.processed_type).c_str());
                    }
                    for(auto &val: annot.values) {
                        JAVM_LOG("[annotations] Processing tag '%c'...", (char)val.tag);
                        switch(static_cast<char>(val.tag)) {
                            case AnnotationTagType::Byte:
                            case AnnotationTagType::Char:
                            case AnnotationTagType::Integer: 
                            case AnnotationTagType::Short: 
                            case AnnotationTagType::Boolean: {
                                auto int_data_item = pool.GetItemAt(val.const_value.const_value_index, vm::ConstantPoolTag::Integer);
                                if(int_data_item) {
                                    auto &int_data = int_data_item->GetIntegerData();
                                    val.const_value.processed_int = int_data.integer;
                                }
                                break;
                            }
                            case AnnotationTagType::Float: {
                                auto flt_data_item = pool.GetItemAt(val.const_value.const_value_index, vm::ConstantPoolTag::Float);
                                if(flt_data_item) {
                                    auto &flt_data = flt_data_item->GetFloatData();
                                    val.const_value.processed_flt = flt_data.flt;
                                }
                                break;
                            }
                            case AnnotationTagType::Double: {
                                auto dbl_data_item = pool.GetItemAt(val.const_value.const_value_index, vm::ConstantPoolTag::Double);
                                if(dbl_data_item) {
                                    auto &dbl_data = dbl_data_item->GetDoubleData();
                                    val.const_value.processed_dbl = dbl_data.dbl;
                                }
                                break;
                            }
                            case AnnotationTagType::Long: {
                                auto lng_data_item = pool.GetItemAt(val.const_value.const_value_index, vm::ConstantPoolTag::Long);
                                if(lng_data_item) {
                                    auto &lng_data = lng_data_item->GetLongData();
                                    val.const_value.processed_lng = lng_data.lng;
                                }
                                break;
                            }
                            case AnnotationTagType::String: {
                                auto str_data_item = pool.GetItemAt(val.const_value.const_value_index, vm::ConstantPoolTag::Utf8);
                                if(str_data_item) {
                                    auto &str_data = str_data_item->GetUtf8Data();
                                    val.const_value.processed_str = StrUtils::FromUtf8(str_data.utf8_str);
                                }
                                break;
                            }
                            case AnnotationTagType::Enum: {
                                auto name_data_item = pool.GetItemAt(val.enum_const_value.type_name_index, vm::ConstantPoolTag::Utf8);
                                if(name_data_item) {
                                    auto &name_data = name_data_item->GetUtf8Data();
                                    val.enum_const_value.processed_type_name = StrUtils::FromUtf8(name_data.utf8_str);
                                }
                                name_data_item = pool.GetItemAt(val.enum_const_value.const_name_index, vm::ConstantPoolTag::Utf8);
                                if(name_data_item) {
                                    auto &name_data = name_data_item->GetUtf8Data();
                                    val.enum_const_value.processed_const_name = StrUtils::FromUtf8(name_data.utf8_str);
                                }
                                break;
                            }
                            case AnnotationTagType::Class: {
                                JAVM_LOG("[annotations] whoops - TODO...");
                                break;
                            }
                            case AnnotationTagType::Annotation: {
                                JAVM_LOG("[annotations] whoops - TODO...");
                                break;
                            }
                            default: {
                                // TODO
                                JAVM_LOG("[annotations] Processing unhandled type: '%c'...", (char)val.tag);
                                break;
                            }
                        }
                    }
                }
            }

            inline std::vector<Annotation> &GetAnnotations() {
                return this->annotations;
            }

            bool HasAnnotation(const String &type) {
                for(const auto &annot: this->annotations) {
                    if(annot.processed_type == type) {
                        return true;
                    }
                }

                return false;
            }
    };

    class AttributesItem {
        private:
            std::vector<AttributeInfo> attributes;
            std::vector<RuntimeAnnotationsAttributeData> annotation_infos;

            void ProcessAttributeInfoArray(ConstantPool &pool) {
                for(auto &info: this->attributes) {
                    auto name_data_item = pool.GetItemAt(info.GetNameIndex(), vm::ConstantPoolTag::Utf8);
                    if(name_data_item) {
                        auto &name_data = name_data_item->GetUtf8Data();
                        info.SetName(StrUtils::FromUtf8(name_data.utf8_str));
                    }
                }
            }

        public:
            void SetAttributes(const std::vector<AttributeInfo> &attrs, ConstantPool &pool) {
                this->attributes = attrs;
                this->ProcessAttributeInfoArray(pool);
                this->ProcessAttributes(pool);
            }

            void ProcessAttributes(ConstantPool &pool) {
                for(auto &attr: this->attributes) {
                    if(attr.GetName() == AttributeType::RuntimeVisibleAnnotations) {
                        MemoryReader reader(attr.GetInfo(), attr.GetInfoLength());
                        RuntimeAnnotationsAttributeData runtime_annot(reader);
                        runtime_annot.ProcessAnnotations(pool);
                        this->annotation_infos.push_back(runtime_annot);
                    }
                }
            }

            inline std::vector<AttributeInfo> &GetAttributes() {
                return this->attributes;
            }

            inline std::vector<RuntimeAnnotationsAttributeData> &GetRuntimeAttributeAnnotations() {
                return this->annotation_infos;
            }

            bool HasAnnotation(const String &type) {
                for(auto &rt_annot: this->annotation_infos) {
                    for(auto &annot: rt_annot.GetAnnotations()) {
                        if(annot.processed_type == type) {
                            return true;
                        }
                    }
                }
                return false;
            }
    };

    class FieldInfo : public ConstantNameItem, public ConstantDescriptorItem, public AccessFlagsItem, public AttributesItem {
        public:
            FieldInfo(MemoryReader &reader, ConstantPool &pool) {
                this->SetAccessFlags(BE(reader.Read<u16>()));
                this->SetNameIndex(BE(reader.Read<u16>()));
                this->SetDescriptorIndex(BE(reader.Read<u16>()));

                std::vector<AttributeInfo> attrs;
                const auto attribute_count = BE(reader.Read<u16>());
                attrs.reserve(attribute_count);
                for(u32 i = 0; i < attribute_count; i++) {
                    attrs.emplace_back(reader);
                }
                this->SetAttributes(attrs, pool);
            }
    };

}