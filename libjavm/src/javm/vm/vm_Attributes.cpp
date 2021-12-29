#include <javm/javm_VM.hpp>

namespace javm::vm {

    namespace {

        Annotation ReadAnnotation(MemoryReader &reader) {
            Annotation annot = {};
            annot.type_index = BE(reader.Read<u16>());
            const auto val_count = BE(reader.Read<u16>());
            JAVM_LOG("[annotations] Value count: %d", val_count);

            for(u16 i = 0; i < val_count; i++) {
                Value val = {};
                val.name_index = BE(reader.Read<u16>());
                val.tag = reader.Read<u8>();
                JAVM_LOG("[annotations] Reading annotation of tag '%c'...", static_cast<char>(val.tag));
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
                        JAVM_LOG("[annotations] annot inside annot...");
                        auto annot = ReadAnnotation(reader);
                        break;
                    }
                    default: {
                        // TODO
                        JAVM_LOG("[annotations] Tried to read invalid annotation!");
                        break;
                    }
                }
            }
            return annot;
        }

    }

    AttributeInfo::AttributeInfo(MemoryReader &reader) : length(0), attribute_data(nullptr) {
        this->SetNameIndex(BE(reader.Read<u16>()));
        this->length = BE(reader.Read<u32>());
        if(this->length > 0) {
            this->attribute_data = new u8[this->length]();
            reader.ReadPointer(this->attribute_data, this->length);
        }
    }

    CodeAttributeData::CodeAttributeData(MemoryReader &reader, ConstantPool &pool) : max_stack(0), max_locals(0), code_len(0), code(nullptr) {
        this->max_stack = BE(reader.Read<u16>());
        this->max_locals = BE(reader.Read<u16>());
        this->code_len = BE(reader.Read<u32>());
        if(this->code_len > 0) {
            this->code = new u8[this->code_len]();
            reader.ReadPointer(this->code, this->code_len);
        }

        const auto exc_table_len = BE(reader.Read<u16>());
        this->exc_table.reserve(exc_table_len);
        for(u16 i = 0; i < exc_table_len; i++) {
            const ExceptionTableEntry exc_table_entry = {
                .start_code_offset = BE(reader.Read<u16>()),
                .end_code_offset = BE(reader.Read<u16>()),
                .handler_code_offset = BE(reader.Read<u16>()),
                .catch_exc_type_index = BE(reader.Read<u16>())
            };
            this->exc_table.push_back(exc_table_entry);
        }

        const auto attr_count = BE(reader.Read<u16>());
        std::vector<AttributeInfo> attrs;
        attrs.reserve(attr_count);
        for(u16 i = 0; i < attr_count; i++) {
            attrs.emplace_back(reader);
        }
        this->SetAttributes(attrs, pool);
    }

    LineNumberTable CodeAttributeData::GetLineNumberTable() {
        LineNumberTable total_line_no_table;
        for(const auto &attr : this->GetAttributes()) {
            if(attr.GetName() == AttributeName::LineNumberTable) {
                auto reader = attr.OpenRead();
                LineNumberTableAttributeData line_no_table_attr(reader);
                const auto line_no_table = line_no_table_attr.GetLineNumberTable();
                total_line_no_table.table.insert(total_line_no_table.table.end(), line_no_table.table.begin(), line_no_table.table.end());
            }
        }
        return total_line_no_table;
    }

    RuntimeVisibleAnnotationsAttributeData::RuntimeVisibleAnnotationsAttributeData(MemoryReader &reader) {
        const auto annot_count = BE(reader.Read<u16>());
        JAVM_LOG("[annotations] Annotation count: %d", annot_count);
        for(u16 i = 0; i < annot_count; i++) {
            const auto annot = ReadAnnotation(reader);
            this->annotations.push_back(annot);
        }
    }

    void RuntimeVisibleAnnotationsAttributeData::ProcessAnnotations(ConstantPool &pool) {
        for(auto &annot: this->annotations) {
            auto type_data_item = pool.GetItemAt(annot.type_index, vm::ConstantPoolTag::Utf8);
            if(type_data_item) {
                const auto &type_data = type_data_item->GetUtf8Data();
                annot.processed_type = str::FromUtf8(type_data.utf8_str);
                JAVM_LOG("[annotations] Annotation name: '%s'...", str::ToUtf8(annot.processed_type).c_str());
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
                            const auto &int_data = int_data_item->GetIntegerData();
                            val.const_value.processed_int = int_data.integer;
                        }
                        break;
                    }
                    case AnnotationTagType::Float: {
                        auto flt_data_item = pool.GetItemAt(val.const_value.const_value_index, vm::ConstantPoolTag::Float);
                        if(flt_data_item) {
                            const auto &flt_data = flt_data_item->GetFloatData();
                            val.const_value.processed_flt = flt_data.flt;
                        }
                        break;
                    }
                    case AnnotationTagType::Double: {
                        auto dbl_data_item = pool.GetItemAt(val.const_value.const_value_index, vm::ConstantPoolTag::Double);
                        if(dbl_data_item) {
                            const auto &dbl_data = dbl_data_item->GetDoubleData();
                            val.const_value.processed_dbl = dbl_data.dbl;
                        }
                        break;
                    }
                    case AnnotationTagType::Long: {
                        auto lng_data_item = pool.GetItemAt(val.const_value.const_value_index, vm::ConstantPoolTag::Long);
                        if(lng_data_item) {
                            const auto &lng_data = lng_data_item->GetLongData();
                            val.const_value.processed_lng = lng_data.lng;
                        }
                        break;
                    }
                    case AnnotationTagType::String: {
                        auto str_data_item = pool.GetItemAt(val.const_value.const_value_index, vm::ConstantPoolTag::Utf8);
                        if(str_data_item) {
                            const auto &str_data = str_data_item->GetUtf8Data();
                            val.const_value.processed_str = str::FromUtf8(str_data.utf8_str);
                        }
                        break;
                    }
                    case AnnotationTagType::Enum: {
                        auto name_data_item = pool.GetItemAt(val.enum_const_value.type_name_index, vm::ConstantPoolTag::Utf8);
                        if(name_data_item) {
                            const auto &name_data = name_data_item->GetUtf8Data();
                            val.enum_const_value.processed_type_name = str::FromUtf8(name_data.utf8_str);
                        }
                        name_data_item = pool.GetItemAt(val.enum_const_value.const_name_index, vm::ConstantPoolTag::Utf8);
                        if(name_data_item) {
                            const auto &name_data = name_data_item->GetUtf8Data();
                            val.enum_const_value.processed_const_name = str::FromUtf8(name_data.utf8_str);
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
                        JAVM_LOG("[annotations] Tried to process unhandled annotation!");
                        break;
                    }
                }
            }
        }
    }

    void SourceFileAttributeData::Process(ConstantPool &pool) {
        auto source_file_item = pool.GetItemAt(this->source_file_index, ConstantPoolTag::Utf8);
        if(source_file_item) {
            const auto source_file_data = source_file_item->GetUtf8Data();
            this->processed_source_file = str::FromUtf8(source_file_data.utf8_str);
        } 
    }

    u16 LineNumberTable::FindLineNumber(const u16 code_offset) const {
        LineNumberTableEntry cur_entry = { 0, 0 };
        for(const auto &entry : this->table) {
            if(entry.start_code_offset <= code_offset) {
                if(entry.start_code_offset >= cur_entry.start_code_offset) {
                    cur_entry = entry;
                }
            }
        }

        return cur_entry.line_no;
    }

    void AttributesItem::ProcessAttributeInfoArray(ConstantPool &pool) {
        for(auto &info: this->attributes) {
            auto name_data_item = pool.GetItemAt(info.GetNameIndex(), vm::ConstantPoolTag::Utf8);
            if(name_data_item) {
                const auto &name_data = name_data_item->GetUtf8Data();
                info.SetName(str::FromUtf8(name_data.utf8_str));
            }
        }
    }

    void AttributesItem::ProcessAttributes(ConstantPool &pool) {
        for(const auto &attr: this->attributes) {
            if(attr.GetName() == AttributeName::RuntimeVisibleAnnotations) {
                auto reader = attr.OpenRead();
                RuntimeVisibleAnnotationsAttributeData runtime_annot(reader);
                runtime_annot.ProcessAnnotations(pool);
                this->annotation_infos.push_back(runtime_annot);
            }
        }
    }

    FieldInfo::FieldInfo(MemoryReader &reader, ConstantPool &pool) {
        this->SetAccessFlags(BE(reader.Read<u16>()));
        this->SetNameIndex(BE(reader.Read<u16>()));
        this->SetDescriptorIndex(BE(reader.Read<u16>()));

        const auto attribute_count = BE(reader.Read<u16>());
        std::vector<AttributeInfo> attrs;
        attrs.reserve(attribute_count);
        for(u32 i = 0; i < attribute_count; i++) {
            attrs.emplace_back(reader);
        }
        this->SetAttributes(attrs, pool);
    }

}