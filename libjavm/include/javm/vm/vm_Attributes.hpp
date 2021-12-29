
#pragma once
#include <javm/javm_Memory.hpp>
#include <javm/vm/vm_ConstantPool.hpp>

namespace javm::vm {

    class AttributeInfo : public ConstantNameItem {
        private:
            u32 length;
            u8 *attribute_data;

        public:
            AttributeInfo(MemoryReader &reader);

            // TODO: make this dtor, or actually call it where it should be disposed?
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

            inline MemoryReader OpenRead() const {
                return MemoryReader(this->attribute_data, this->length);
            }
    };

    struct ExceptionTableEntry {
        u16 start_code_offset;
        u16 end_code_offset;
        u16 handler_code_offset;
        u16 catch_exc_type_index;
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

    class RuntimeVisibleAnnotationsAttributeData {
        private:
            u32 len;
            std::vector<Annotation> annotations;
            u16 type_index;
            String type;

        public:
            RuntimeVisibleAnnotationsAttributeData(MemoryReader &reader);

            void ProcessAnnotations(ConstantPool &pool);

            inline const std::vector<Annotation> &GetAnnotations() const {
                return this->annotations;
            }

            inline bool HasAnnotation(const String &type) {
                for(const auto &annot: this->annotations) {
                    if(annot.processed_type == type) {
                        return true;
                    }
                }

                return false;
            }
    };

    class SourceFileAttributeData {
        private:
            u16 source_file_index;
            String processed_source_file;

        public:
            SourceFileAttributeData(MemoryReader &reader) {
                this->source_file_index = BE(reader.Read<u16>());
            }

            void Process(ConstantPool &pool);

            inline String GetSourceFile() {
                return this->processed_source_file;
            }
    };

    struct LineNumberTableEntry {
        u16 start_code_offset;
        u16 line_no;
    };

    struct LineNumberTable {
        std::vector<LineNumberTableEntry> table;

        u16 FindLineNumber(const u16 code_offset) const;
    };

    class LineNumberTableAttributeData {
        private:
            LineNumberTable line_no_table;

        public:
            LineNumberTableAttributeData(MemoryReader &reader) {
                const auto table_count = BE(reader.Read<u16>());
                this->line_no_table.table.reserve(table_count);

                for(u16 i = 0; i < table_count; i++) {
                    const LineNumberTableEntry entry = {
                        .start_code_offset = BE(reader.Read<u16>()),
                        .line_no = BE(reader.Read<u16>())
                    };
                    this->line_no_table.table.push_back(entry);
                }
            }

            inline LineNumberTable &GetLineNumberTable() {
                return this->line_no_table;
            }
    };

    class AttributesItem {
        private:
            std::vector<AttributeInfo> attributes;
            std::vector<RuntimeVisibleAnnotationsAttributeData> annotation_infos;

            void ProcessAttributeInfoArray(ConstantPool &pool);
            void ProcessAttributes(ConstantPool &pool);

        public:
            inline void SetAttributes(const std::vector<AttributeInfo> &attrs, ConstantPool &pool) {
                this->attributes = attrs;
                this->ProcessAttributeInfoArray(pool);
                this->ProcessAttributes(pool);
            }

            inline const std::vector<AttributeInfo> &GetAttributes() const {
                return this->attributes;
            }

            inline const std::vector<RuntimeVisibleAnnotationsAttributeData> &GetRuntimeAttributeAnnotations() const {
                return this->annotation_infos;
            }

            inline bool HasAnnotation(const String &type) const {
                for(const auto &rt_annot: this->annotation_infos) {
                    for(const auto &annot: rt_annot.GetAnnotations()) {
                        if(annot.processed_type == type) {
                            return true;
                        }
                    }
                }

                return false;
            }
    };

    class CodeAttributeData : AttributesItem {
        private:
            u16 max_stack;
            u16 max_locals;
            u32 code_len;
            u8 *code;
            std::vector<ExceptionTableEntry> exc_table;

        public:
            CodeAttributeData(MemoryReader &reader, ConstantPool &pool);

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

            LineNumberTable GetLineNumberTable();
    };

    class FieldInfo : public ConstantNameItem, public ConstantDescriptorItem, public AccessFlagsItem, public AttributesItem {
        public:
            FieldInfo(MemoryReader &reader, ConstantPool &pool);
    };

}