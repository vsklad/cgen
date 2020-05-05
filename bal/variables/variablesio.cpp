//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include <iomanip>
#include "container.hpp"
#include "variablesio.hpp"

namespace bal {

    // determines if there is a sequence of variables in the data array
    // supports both asc and desc, determines step size
    // if no elements, returns sequence_size = 0, sequence_step = 0
    // if no sequence, returns sequence_size = 1, sequence_step = 0
    // otherwise returns sequence_size and step_size
    // sequence_step = 0 means the same variable is repeated
    
    // returns positive number of elements if asc, negative if desc
    // 1 if no sequence, 0 if no elements or a constant
    inline void get_variables_sequence_(const literalid_t* const data, const size_t size,
                                              size_t& sequence_size, int& sequence_step) {
        sequence_size = 0;
        sequence_step = 0;
        
        if (size > 0 && !literal_t__is_constant(data[0])) {
            sequence_size = 1;
            if (size > 1) {
                if (literal_t__is_variable(data[0]) && literal_t__is_variable(data[1])) {
                    // determine direction
                    sequence_step = data[sequence_size] - data[sequence_size-1];
                    // difference 1 means opposit negation, i.e. no sequence
                    if ((sequence_step & 0x1) == 0) {
                        // determine length, already have 2
                        sequence_size = 2;
                        while (sequence_size < size) {
                            if (literal_t__is_variable(data[sequence_size])) {
                                if (data[sequence_size - 1] + sequence_step == data[sequence_size]) {
                                    sequence_size++;
                                    continue;
                                };
                            };
                            break;
                        };
                        // two variables with step > 1 (2) is not a sequence
                        if (sequence_size == 2 && sequence_step != 2 && sequence_step != -2) {
                            sequence_size = 1;
                        };
                    };
                    // 2 between literals, 1 between variables
                    sequence_step >>= 1;
                } else if (literal_t__is_unassigned(data[0]) && literal_t__is_unassigned(data[1])) {
                    // simply count the unassigned literals
                    while (sequence_size < size && literal_t__is_unassigned(data[sequence_size])) {
                        sequence_size++;
                    };
                };
            };
        };
    };
    
    inline void get_variables_range_sequence_(const literalid_t* const data, const size_t size,
                                                    const size_t range_size,
                                                    size_t& sequence_size, int& sequence_step) {
        assert(size % range_size == 0); // data size is a multiple of range_size
        
        sequence_size = size > 0 ? 1 : 0;
        sequence_step = 0;
        
        if (size > range_size) {
            // assume the step from the first pair of variables from the first and second ranges
            // stop when this step is not the same at any point
            // non-zero step is only applicable to variables; it is zero for everything else
            bool b_first_variable_found = false;
            const literalid_t* p_prev = data;
            const literalid_t* p_curr = p_prev + range_size;
            
            while (p_curr < data + size) {
                bool b_sequence_step_ok = true;
                const literalid_t* p_next = p_curr + range_size;
                
                while(p_curr < p_next) {
                    if (literal_t__is_variable(*p_curr) && literal_t__is_variable(*p_prev)) {
                        if (!b_first_variable_found) {
                            sequence_step = *p_curr - *p_prev;
                            b_first_variable_found = true;
                        } else if (*p_curr - *p_prev != sequence_step) {
                            b_sequence_step_ok = false;
                            break;
                        };
                    } else if (*p_curr != *p_prev) {
                        b_sequence_step_ok = false;
                        break;
                    };
                    p_prev++;
                    p_curr++;
                };
                
                if (!b_sequence_step_ok) {
                    break;
                };
                
                sequence_size++;
                p_next += range_size;
            };
            
            // 2 between literals, 1 between variables
            sequence_step >>= 1;
        };
    };
    
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
        bool bracket_1_open = false;
        
        size_t i = 0;
        while (i < elements_size) {
            if (i > 0) {
                stream << ", ";
            };
            
            // check and process the possible sequence of elements
            size_t elements_sequence_size = 1;
            int elements_sequence_step = 0;
            get_variables_range_sequence_(literalid, size - (i * element_size), element_size,
                                                    elements_sequence_size, elements_sequence_step);
            
            if (i == 0 && !bracket_1_open && size > element_size * elements_sequence_size) {
                stream << "{";
                bracket_1_open = true;
            };

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
                        constant = (constant << 1) | (literal_t__is_constant_1(*(literalid + k)) ? 1 : 0);
                        k++;
                        continue;
                    };
                    break;
                };
                if (k > 0) {
                    // constants that have number of bits mod 4 != 0, require brackets
                    // because they are split into bin and hex
                    if (j == 0 && ((k < element_size) || (k > 3 && k % 4 != 0)) && !bracket_2_open) {
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
                get_variables_sequence_(literalid, element_size - j, k, sequence_step);
                
                // elements_sequence_size == 1 && k > 1 means the sequence is within element
                // therefore must be bracketed in order to avoid confusion
                if (i == 0 && j == 0 && !bracket_1_open && elements_sequence_size == 1 && k > 1) {
                    stream << "{";
                    bracket_1_open = true;
                };
                
                if (k > 0) {
                    // k < element_size means there are more elements to output
                    // k > 1 sequence within element must always be bracketed
                    if (!bracket_2_open && j == 0 && (k < element_size || k > 1)) {
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
        
        if (bracket_1_open) {
            stream << "}";
        };
        return stream;
    };
    
    inline bool VariableTextReader::is_variable_element_sequence_() {
        skip_space();
        return is_token(TextReader::ttSymbol) && is_symbol('/');
    };
    
    inline void VariableTextReader::read_variable_element_sequence_(unsigned int& sequence_size, signed int& step_size) {
        sequence_size = 1;
        step_size = 0;
        
        skip_space();
        if (is_symbol('/')) {
            skip_symbol();
            skip_space();
            sequence_size = read_uint32(1);
            skip_space();
            if (is_symbol('/')) {
                skip_symbol();
                skip_space();
                step_size = read_sint32();
                skip_space();
            };
        };
    };
    
    inline void VariableTextReader::read_variable_element_item_(Container<literalid_t>& value) {
        container_size_t item_size = 1;
        if (is_token(TextReader::ttHex) || is_token(TextReader::ttBin)) {
            const char* const token = TextReader::get_current_token();
            const size_t token_len = get_current_token_len();
            _assert_level_1(token_len > 2 && token_len <= CONTAINER_SIZE_MAX); // prefix
            
            item_size = (container_size_t)token_len - 2;
            if (is_token(TextReader::ttHex)) {
                item_size <<= 2;
            };
            value.reserve(item_size);
            literalid_t* p_literal = value.data_ + value.size_;
            
            if (is_token(TextReader::ttHex)) {
                for (auto i = 2; i < token_len; i++) {
                    char h = _hex_value(token[i]);
                    p_literal[0] = literal_t__constant(h & 0b1000);
                    p_literal[1] = literal_t__constant(h & 0b0100);
                    p_literal[2] = literal_t__constant(h & 0b0010);
                    p_literal[3] = literal_t__constant(h & 0b0001);
                    p_literal += 4;
                };
            } else { // if (is_token(TextReader::ttBin))
                for (auto i = 2; i < token_len; i++) {
                    *p_literal = literal_t__constant(_bin_value(token[i]));
                    p_literal++;
                };
            };
            skip_token();
        } else if (is_token(TextReader::ttSymbol) && is_symbol('*')) {
            value.reserve(item_size);
            value.data_[value.size_] = LITERALID_UNASSIGNED;
            skip_token();
        } else { // variables
            bool b_negated = false;
            if (is_symbol('-')) {
                b_negated = true;
                skip_symbol();
            };
            
            if (b_negated && (is_token(TextReader::ttHex) || is_token(TextReader::ttBin))) {
                parse_error("Negative constant values not supported");
            };
            
            variableid_t variable_id = read_uint32();
            if (variable_id == 0) {
                parse_error("Variable number may not be equal to 0");
            } else if (variable_id > VARIABLEID_MAX) {
                parse_error("Variable number is out of range");
            };
            literalid_t literal_id = variable_t__literal_id_negated_onlyif(variable_id - 1, b_negated);
            
            value.reserve(1);
            value.data_[value.size_] = literal_id;
        };
        
        value.size_ += item_size;
    };
    
    inline void VariableTextReader::read_variable_elements_sequence_(Container<literalid_t>& value,
                                                                     const variables_size_t element_size) {
        _assert_level_0(element_size > 0);
        _assert_level_0(value.size_ >= element_size);
        
        unsigned int sequence_size = 1;
        signed int step_size = 0;
        read_variable_element_sequence_(sequence_size, step_size);
        
        if (step_size != 0) {
            // require at least one variable within value
            bool b_no_variables = true;
            for (auto i = 0; (i < element_size) && b_no_variables; i++) {
                b_no_variables &= !literal_t__is_variable(value.data_[value.size_ - i - 1]);
            };
            if (b_no_variables) {
                parse_error("Sequence step can be non-zero for variable numbers sequence only");
            };
        };
        
        if (sequence_size > 1) {
            value.reserve((sequence_size - 1) * element_size);
            
            for (auto i = 0; i < sequence_size - 1; i++) {
                literalid_t* p_element = value.data_ + value.size_ + (i * element_size);
                for (auto j = 0; j < element_size; j++) {
                    // adjust variables, preserve constants/unassigned
                    const literalid_t literal = *(p_element + j - element_size);
                    if (literal_t__is_variable(literal)) {
                        if (literal_t__sequence_next_is_valid(literal, step_size)) {
                            *(p_element + j) = literal_t__sequence_next(literal, step_size);
                        } else {
                            parse_error("Sequence produces an invalid variable number");
                        };
                    } else {
                        *(p_element + j) = literal;
                    };
                };
                p_element ++;
            };
            
            value.size_ += (sequence_size - 1) * element_size;
        };
    };
    
    // treat sequence as a single element unless the value is within {}
    inline container_size_t VariableTextReader::read_variable_element_(Container<literalid_t>& value, const unsigned level) {
        const container_size_t baseline_size = value.size_;
        container_size_t element_size = 1;
        
        const bool b_brackets = is_token(TextReader::ttSymbol) && is_symbol('{');
        if (b_brackets) {
            skip_symbol();
            skip_space();
        
            element_size = read_variable_element_(value, level + 1);
        
            // read more elements
            skip_space();
            while (is_symbol(',')) {
                skip_symbol();
                skip_space();
                
                const container_size_t other_size = read_variable_element_(value, level + 1);
                if (other_size != element_size) {
                    // there is no structure, assume a single element
                    element_size = 0;
                };
                
                skip_space();
            };
            
            read_symbol('}');
        } else {
            read_variable_element_item_(value);
            element_size = value.size_ - baseline_size;
        };
        
        if (!b_brackets || element_size == 0 || level >= 1 || is_variable_element_sequence_()) {
            element_size = value.size_ - baseline_size;
        };
        
        read_variable_elements_sequence_(value, element_size);
        
        return level <= 1 ? element_size : value.size_ - baseline_size;
    };
    
    VariablesArray VariableTextReader::read_variable_value() {
        Container<literalid_t> value;
        const container_size_t element_size = read_variable_element_(value);
        return VariablesArray(std::move(value), element_size);
    };
};
