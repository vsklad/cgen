//
//  Propositional Logic Engine (PLE) Library
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include "vector.hpp"
#include "variablesio.hpp"

namespace ple {

    void write_sequence_parameters_(std::ostream& stream,
                                    const size_t sequence_size, const int sequence_step) {
        if (sequence_size > 1) {
            stream << "/" << std::dec << sequence_size;
            if (sequence_step != 0) {
                stream << "/" << std::dec << sequence_step;
            };
        };
    };
    
    std::ostream& operator << (std::ostream& stream, const VariablesArray& array) {
        const variableid_t size = array.size();
        const variableid_t element_size = array.element_size();
        const variableid_t elements_size = size / element_size;
        
        assert(elements_size * element_size == size);
        
        const literalid_t* literalid = array.data();
        if (size > element_size) {
            stream << "{";
        };
        
        size_t i = 0;
        while (i < elements_size) {
            if (i > 0) {
                stream << ", ";
            };
            
            // check and process the possible sequence of elements
            size_t elements_sequence_size = 1;
            int elements_sequence_step = 0;
            literal_t::get_variables_range_sequence(literalid, size - (i * element_size), element_size,
                                                    elements_sequence_size, elements_sequence_step);
            
            bool bracket_2_open = false;
            
            size_t j = 0;
            while(j < element_size) {
                size_t k;
                
                // output constant as much as possible, limit to 64 bit, align to 4 bits
                // look ahead bit by bit and compress
                uint64_t constant = 0;
                k = 0;
                while ((k < 64) && (j + k) < element_size) {
                    if (literal_t__is_constant(*(literalid + k))) {
                        constant = (constant << 1) | literal_t__is_constant_1(*(literalid + k));
                        k++;
                        continue;
                    };
                    break;
                };
                if (k > 0) {
                    if (j == 0 && k < element_size && !bracket_2_open) {
                        stream << "{";
                        bracket_2_open = true;
                    } else if (j > 0) {
                        stream << ", ";
                    };
                    
                    // output the most significant bits to align to 4 from the lower end of the element
                    // if less than 4 bits total, output everything here
                    const size_t k_prefix = (k <= 3) ? k : ((element_size - j) & 0x3);
                    if (k_prefix > 0 && k > 0) {
                        uint64_t prefix_constant = constant >> (k > k_prefix ? k - k_prefix : 0);
                        stream << "0b";
                        stream << ((k_prefix > 2 && k > 2) ? (prefix_constant & 0b100) ? "1" : "0" : "");
                        stream << ((k_prefix > 1 && k > 1) ? (prefix_constant & 0b010) ? "1" : "0" : "");
                        stream <<                           ((prefix_constant & 0b001) ? "1" : "0");
                        // zero out b_prefix most significant bits
                        constant &= 0xFFFFFFFFFFFFFFFFull >> (64 - k + k_prefix);
                    };
                    
                    if (k > k_prefix) {
                        const size_t k_suffix = (k - k_prefix) & 0x3;
                        if (k - k_prefix > k_suffix) {
                            // output the main part of the constant aligned to 4 bits
                            if (k_prefix > 0) {
                                stream << ", "; // separate from the prefix
                            };
                            stream << "0x" << std::setfill('0') << std::setw((int)((k - k_prefix) >> 2));
                            stream << std::hex << (constant >> k_suffix);
                        };
                        
                        if (k_suffix > 0) {
                            // output the remaining least significant bits
                            if (k > k_suffix) {
                                stream << ", "; // separate from the hex constant
                            };
                            stream << "0b";
                            stream << (k_suffix > 2 ? (constant & 0b100) ? "1" : "0" : "");
                            stream << (k_suffix > 1 ? (constant & 0b010) ? "1" : "0" : "");
                            stream <<                ((constant & 0b001) ? "1" : "0");
                        };
                    };
                    
                    // skip the processed literals
                    literalid += k;
                    j += k;
                };
                
                // output variables as much as possible; compress sequential
                int sequence_step = 0;
                literal_t::get_variables_sequence(literalid, element_size - j, k, sequence_step);
                
                if (k > 0) {
                    if (!bracket_2_open && j == 0 && (k < element_size || elements_sequence_size > 1)) {
                        stream << "{";
                        bracket_2_open = true;
                    } else if (j > 0) {
                        stream << ", ";
                    };
                    stream << literal_t(*literalid);
                    write_sequence_parameters_(stream, k, sequence_step);
                    // skip the processed literals
                    literalid += k;
                    j += k;
                };
            };
            
            if (bracket_2_open) {
                stream << "}";
            };
            
            write_sequence_parameters_(stream, elements_sequence_size, elements_sequence_step);
            
            // skip processed elements
            i += elements_sequence_size;
            literalid += (elements_sequence_size - 1) * element_size;
        };
        
        if (elements_size > 1) {
            stream << "}";
        };
        return stream;
    };
    
    inline void VariableTextReader::read_variable_sequence_(unsigned int& sequence_size, signed int& step_size) {
        sequence_size = 1;
        step_size = 0;
        
        skip_space();
        if (is_symbol('/')) {
            skip_symbol();
            skip_space();
            sequence_size = read_uint32();
            skip_space();
            if (is_symbol('/')) {
                skip_symbol();
                skip_space();
                step_size = read_sint32();
                skip_space();
            };
        };
    };
    
    inline void VariableTextReader::read_variable_element_hex_(vector<literalid_t>& value) {
        size_t token_len = get_current_token_len();
        assert(token_len > 2 && token_len <= vector<literalid_t>::VECTOR_SIZE_MAX); // prefix
        const vector<literalid_t>::vector_size_t value_size =
            ((vector<literalid_t>::vector_size_t)token_len - 2) << 2;
        value.reserve(value_size);
        
        literalid_t* p_current = value.data_ + value.size_;
        const char* const token = get_current_token();
        
        if (token[0] == '-') {
            parse_error("Negative hexadecimal values not supported");
        };
        
        for (auto i = 2; i < token_len; i++) {
            char h = _hex_value(token[i]);
            p_current[0] = literal_t__constant(h & 0b1000);
            p_current[1] = literal_t__constant(h & 0b0100);
            p_current[2] = literal_t__constant(h & 0b0010);
            p_current[3] = literal_t__constant(h & 0b0001);
            p_current += 4;
        };
        
        value.size_ += value_size;
        skip_token();
    };
    
    inline void VariableTextReader::read_variable_element_bin_(vector<literalid_t>& value) {
        size_t token_len = get_current_token_len();
        assert(token_len > 2 && token_len <= vector<literalid_t>::VECTOR_SIZE_MAX); // prefix
        const vector<literalid_t>::vector_size_t value_size =
            (vector<literalid_t>::vector_size_t)token_len - 2;
        value.reserve(value_size);
        
        literalid_t* p_current = value.data_ + value.size_;
        const char* const token = get_current_token();

        if (token[0] == '-') {
            parse_error("Negative binary values not supported");
        };
        
        for (auto i = 2; i < token_len; i++) {
            *p_current = literal_t__constant(_bin_value(token[i]));
            p_current++;
        };
        
        value.size_ += value_size;
        skip_token();
    };
    
    inline void VariableTextReader::read_variable_element_var_(vector<literalid_t>& value) {
        bool b_negated = false;
        if (is_symbol('-')) {
            b_negated = true;
            skip_symbol();
        };
        
        variableid_t variable_id = read_uint32();
        if (variable_id == 0) {
            parse_error("Veriable number may not be equal to 0");
        } else if (variable_id > VARIABLEID_MAX) {
            parse_error("Veriable number is out of range");
        };
        
        value.reserve(1);
        literalid_t* p_literal = value.data_ + value.size_;
        *p_literal = variable_t__literal_id_negated_onlyif(variable_id - 1, b_negated);
        
        unsigned int sequence_size = 1;
        signed int step_size = 0;
        read_variable_sequence_(sequence_size, step_size);
        
        if (sequence_size > 1) {
            value.reserve(sequence_size);
            p_literal = value.data_ + value.size_;
            for (literalid_t* p_more = p_literal + 1; p_more < p_literal + sequence_size; p_more++) {
                *p_more = literal_t__sequence_next(*(p_more - 1), step_size);
            };
        };
        
        value.size_ += sequence_size;
    };
    
    inline void VariableTextReader::read_variable_element_item_(vector<literalid_t>& value) {
        skip_space();
        if (is_token(TextReader::ttBin)) {
            read_variable_element_bin_(value);
        } else if (is_token(TextReader::ttHex)) {
            read_variable_element_hex_(value);
        } else { // variable(s)
            read_variable_element_var_(value);
        };
    };
    
    inline void VariableTextReader::read_variable_elements_sequence_(vector<literalid_t>& value,
                                                                     const variableid_t element_size) {
        unsigned int sequence_size = 1;
        signed int step_size = 0;
        read_variable_sequence_(sequence_size, step_size);
        
        if (sequence_size > 1) {
            assert(value.size_ >= element_size);
            value.reserve((sequence_size - 1) * element_size);
            
            for (auto i = 0; i < sequence_size - 1; i++) {
                literalid_t* p_element = value.data_ + value.size_ + (i * element_size);
                for (auto j = 0; j < element_size; j++) {
                    *(p_element + j) = literal_t__sequence_next(*(p_element + j - element_size), step_size);
                };
                p_element ++;
            };
            
            value.size_ += (sequence_size - 1) * element_size;
        };
    };
    
    inline void VariableTextReader::read_variable_element_(vector<literalid_t>& value,
                                                           variableid_t& element_size) {
        const literalid_t baseline_size = value.size_;
        
        bool is_value_bracket = false;
        if (is_symbol('{')) {
            skip_symbol();
            skip_space();
            is_value_bracket = true;
        };
        
        // the first element
        read_variable_element_item_(value);
        
        if (is_value_bracket) {
            // read more elements
            skip_space();
            while (is_symbol(',')) {
                skip_symbol();
                skip_space();
                read_variable_element_item_(value);
                skip_space();
            };
            
            read_symbol('}');
            skip_space();
        };
        
        if (element_size > 0) {
            if (value.size_ - baseline_size != element_size) {
                parse_error("All variable elements must have the same number of binary variables");
            };
        } else {
            element_size = value.size_; // take size from the first element
        };
        
        read_variable_elements_sequence_(value, element_size);
    };
    
    VariablesArray VariableTextReader::read_variable_value() {
        bool is_value_bracket = false;
        if (is_symbol('{')) {
            skip_symbol();
            skip_space();
            is_value_bracket = true;
        };
        
        vector<literalid_t> value;

        // the first element
        variableid_t element_size = 0;
        read_variable_element_(value, element_size);
        
        if (is_value_bracket) {
            // read more elements
            skip_space();
            while (is_symbol(',')) {
                skip_symbol();
                skip_space();
                read_variable_element_(value, element_size);
                skip_space();
            };
            
            read_symbol('}');
            skip_space();
        };
        
        read_variable_elements_sequence_(value, value.size_);
        
        return VariablesArray(value.size_ / element_size, element_size, value.data_);
    };
    
};
