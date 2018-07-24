//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef variablesarray_hpp
#define variablesarray_hpp

#include <iostream>
#include <iomanip>
#include "vector.hpp"
#include "variables.hpp"

namespace ple {

    // words stored to the array in big endian format, i.e. with most significant bit first
    // at the same time, other arrays come sequentially, with aray index going from low to high
    
    class VariablesArray: private vector<literalid_t> {
    private:
        variableid_t element_size_;
        
    public:
        VariablesArray(const variableid_t size, const variableid_t element_size):
            vector<literalid_t>(size * element_size), element_size_(element_size) {
                assert(element_size <= size_);
        };
        
        VariablesArray(const variableid_t element_size): VariablesArray(1, element_size) {};
        VariablesArray(): VariablesArray(0) {};
        
        // set new size that accomodates the specified number of elements
        inline void expand_elements(const variableid_t elements_size) {
            if (size_ < elements_size * element_size_) {
                // initialize
                append(0, (elements_size * element_size_) - size_);
            };
        };
        
        // methods for assigning with uint32 constants
        
        inline VariablesArray(const std::initializer_list<const uint32_t> &values):
            VariablesArray((variableid_t)values.size(), 32) {
            assert(values.size() <= VARIABLEID_MAX);
            assign(values);
        };
        
        inline VariablesArray& operator = (const std::initializer_list<const uint32_t> &values) {
            assign(values);
            return *this;
        };
        
        void assign(const std::initializer_list<const uint32_t> &values) {
            assert(element_size_ == 32);
            assert((size_ >> 5) == values.size());
            for (variableid_t i = 0; i < values.size(); i++) {
                uint32_t value_i = *(values.begin() + i);
                for (variableid_t j = 0; j < element_size_; j++) {
                    data_[i * element_size_ + j] = literal_t__constant(value_i & (0x1 << (element_size_ - j - 1)));
                };
            };
        };
        
        // assigning with literals

        inline VariablesArray(const variableid_t size, const variableid_t element_size,
                              const literalid_t* const values):
            VariablesArray(size, element_size) {
            assign(values, size_, 0);
        };
        
        // set multiple elements without regard to element_size
        void assign(const literalid_t* const src, const variableid_t src_size, const variableid_t dst_index) {
            assert(dst_index < size_ || dst_index == 0);
            size_t copy_size = (src_size < (size_ - dst_index) ? src_size : (size_ - dst_index));
            std::copy(src, src + copy_size, data_ + dst_index);
        };
        
        // assigning from another instance
        
        inline VariablesArray(const VariablesArray& other):
            vector<literalid_t>(other.size_), element_size_(other.element_size_) {
            if (other.size_ > 0) {
                assign(other);
            };
        };
        
        inline VariablesArray& operator = (const VariablesArray& other) {
            reset(other.size_);
            size_ = other.size_;
            element_size_ = other.element_size_;
            assign(other);
            return *this;
        };
        
        void assign(const VariablesArray& src, const variableid_t src_index = 0, const variableid_t dst_index = 0) {
            assert(src_index < src.size_);
            assign(src.data_, src.size_ - src_index, dst_index);
        };

        void assign_element(const VariablesArray& src, const variableid_t index) {
            assert(src.size_ == src.element_size_);
            assign_element(src.data_, src.size_, index);
        };
        
        void assign_element(const literalid_t* const src, const variableid_t src_size, const variableid_t index) {
            assert(src_size == element_size_);
            assert((index + 1) * element_size_ <= size_);
            std::copy(src, src + src_size, data_ + (index * element_size_));
        };
        
        // initialize all variables sequentially
        void assign_sequence(const variableid_t first = 0) {
            for (variableid_t i = 0; i < size_; i++) {
                data_[i] = literal_t(variable_t(first + i)).id();
            };
        };
        
        // initialize all variables sequentially
        void assign_unassigned() {
            for (variableid_t i = 0; i < size_; i++) {
                data_[i] = LITERALID_UNASSIGNED;
            };
        };

        // initialize variables listed in <template_> with values from <value>
        // require that any constant values in <template_> are also the same constants in <value>
        void assign_template_from(const VariablesArray& template_, const VariablesArray& value) {
            assert(template_.size_ == value.size_);
            for (variableid_t i = 0; i < template_.size_; i++) {
                if (literal_t__is_variable(template_.data_[i])) {
                    variableid_t variableid = literal_t__variable_id(template_.data_[i]);
                    assert(variableid < size_);

                    // the value must not be set OR it cannot be set to two different values
                    assert(data_[variableid] == literal_t__substitute_literal(template_.data_[i], value.data_[i]) ||
                           data_[variableid] == variable_t__literal_id(variableid));
                    
                    data_[variableid] = literal_t__substitute_literal(template_.data_[i], value.data_[i]);
                } else {
                    // check that the value matches the template and ignore
                    assert(template_.data_[i] == value.data_[i]);
                };
            };
        };
        
        // set values from template_ to value
        // if src contains a variable, resolve it from self
        void assign_template_into(const VariablesArray& template_, VariablesArray& value) const {
            assert(template_.size_ == value.size_);
            const literalid_t* src = template_.data_;
            literalid_t* dst = value.data_;
            const literalid_t* dst_end = value.data_ + value.size_;
            while (dst < dst_end) {
                if (literal_t__is_variable(*src)) {
                    assert(literal_t__variable_id(*src) < size_);
                    *dst = literal_t__lookup(data_, *src);
                } else {
                    *dst = *src;
                };
                src++;
                dst++;
            };
        };
                
        inline literalid_t* const data() { return data_; };
        inline const literalid_t* const data() const { return data_; };
        inline const variableid_t size() const { return size_; };
        inline const variableid_t element_size() const { return element_size_; };
    };
    
};

#endif /* variablesarray_hpp */
