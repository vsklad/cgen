//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef variablesarray_hpp
#define variablesarray_hpp

#include <string>
#include "assertlevels.hpp"
#include "container.hpp"
#include "variables.hpp"

namespace bal {

    // words stored to the array in big endian format, i.e. with most significant bit first
    // at the same time, other arrays come sequentially, with aray index going from low to high
    
    class VariablesArray: private Container<literalid_t> {
    private:
        container_size_t element_size_;
        
    public:
        VariablesArray(const container_size_t elements_size, const container_size_t element_size):
            Container<literalid_t>(elements_size * element_size), element_size_(element_size) {
                assert(elements_size * element_size <= VARIABLEID_MAX);
        };
        
        VariablesArray(const container_size_t element_size): VariablesArray(0, element_size) {};
        VariablesArray(): VariablesArray(0, 1) {};
        
        // methods for assigning with uint32 constants
        
        inline VariablesArray(const std::initializer_list<const uint32_t> &values):
            VariablesArray((container_size_t)values.size(), 32) {
            this->operator=(values);
        };
        
        inline VariablesArray& operator = (const std::initializer_list<const uint32_t> &values) {
            assert(element_size_ == 32);
            assert((values.size() << 5) <= VARIABLEID_MAX);
            resize((container_size_t)(values.size() << 5));
            
            for (container_size_t i = 0; i < values.size(); i++) {
                uint32_t value_i = *(values.begin() + i);
                for (container_size_t j = 0; j < element_size_; j++) {
                    data_[i * element_size_ + j] = literal_t__constant(value_i & (0x1 << (element_size_ - j - 1)));
                };
            };
            return *this;
        };
        
        // assign from null terminated character string
        template<class charT, class charT_traits = std::char_traits<charT>>
        inline VariablesArray(const charT* const s): VariablesArray((container_size_t)charT_traits::length(s), sizeof(charT) << 3) {
            this->operator=(s);
        };
        
        template<class charT, class charT_traits = std::char_traits<charT>>
        inline VariablesArray& operator = (const charT* const s) {
            static constexpr std::size_t charT_size_bits = sizeof(charT) << 3;
            
            const std::size_t size_char = charT_traits::length(s);
            const std::size_t size_bits = size_char * charT_size_bits;
            
            _assert_level_0(size_bits <= VARIABLEID_MAX);
            _assert_level_1(size_bits % element_size_ == 0);
            resize((container_size_t)size_bits);
            
            // encode bit by bit for each byte
            bal::literalid_t* p_value = data_;
            for (auto i = 0; i < size_char; i++) {
                for (auto j = 0; j < charT_size_bits; j++) {
                    *p_value = literal_t__constant(s[i] & (1 << (charT_size_bits - 1 - j)));
                    p_value++;
                };
            };
            return *this;
        };
        
        // assigning from another instance
        
        VariablesArray(const VariablesArray& other)
            :Container<literalid_t>(other), element_size_(other.element_size_) {};
        VariablesArray(VariablesArray&& other)
            :Container<literalid_t>(std::move(other)), element_size_(other.element_size_) {};
        VariablesArray(const Container<literalid_t>& other, const container_size_t element_size)
            :Container<literalid_t>(other), element_size_(element_size) {};
        VariablesArray(Container<literalid_t>&& other, const container_size_t element_size)
            :Container<literalid_t>(std::move(other)), element_size_(element_size) {};
        
        inline VariablesArray& operator = (const VariablesArray& other) {
            Container<literalid_t>::operator=(other);
            element_size_ = other.element_size_;
            return *this;
        };
        
        inline VariablesArray& operator = (VariablesArray&& other) noexcept {
            Container<literalid_t>::operator=(std::move(other));
            element_size_ = other.element_size_;
            return *this;
        };
        
        // append the provided element to the array
        // expand the array if necessary to accommodate the index
        inline void expand_append_element(const variables_size_t index, const VariablesArray& value) {
            assert(value.size_ == element_size_);
            if ((index + 1) * element_size_ > size_) {
                // fill in the missing elements with unassigned literals
                append(LITERALID_UNASSIGNED, (index + 1) * element_size_ - size_);
            };
            std::copy(value.data_, value.data_ + value.size_, data_ + (index * element_size_));
        };
        
        // initialize all variables sequentially
        inline void assign_sequence(const container_size_t first = 0) {
            for (container_size_t i = 0; i < size_; i++) {
                data_[i] = variable_t__literal_id(first + i);
            };
        };
        
        // initialize all variables unassigned
        inline void assign_unassigned() {
            for (container_size_t i = 0; i < size_; i++) {
                data_[i] = LITERALID_UNASSIGNED;
            };
        };

        // initialize variables listed in <template_> with values from <value>
        // require that any constant values in <template_> are also the same constants in <value>
        // handle asignments of variables to variables and variables to constants
        //    1=3 && 2=3 => 3=1 && 3=2 => 3=1 && 2=1
        //    3=* && 2=3 => 3=2
        //    3=0b1 && 2=3 => 3=2 => 2=0b1
        //    3=-3 && 2=3 => 3=2 => 3=-2
        // return number of changed variables
        // return VARIABLEID_ERROR if the same variable is being assigned to two different values
        inline variables_size_t assign_template_from(const VariablesArray& template_, const VariablesArray& value) {
            _assert_level_1(template_.size_ == value.size_);
            variables_size_t changes_count = 0;
            for (container_size_t i = 0; i < template_.size_; i++) {
                literalid_t lhs_literal = template_.data_[i];
                literalid_t rhs_literal = value.data_[i];
                
                // an unassigned literal means there is nothing to assign or to assign to
                // equalence means there is nothing to assign as well
                while (!literal_t__is_unassigned(rhs_literal) &&
                       !literal_t__is_unassigned(lhs_literal) && lhs_literal != rhs_literal) {
                    
                    if (literal_t__is_variable(lhs_literal)) {
                        rhs_literal = literal_t__substitute_literal(lhs_literal, rhs_literal);
                        literal_t__unnegate(lhs_literal);
                        
                        const variableid_t variableid = literal_t__variable_id(lhs_literal);
                        _assert_level_1(variableid < size_);

                        // the value must not be set OR it cannot be set to two different values OR
                        // for variables, higher variable id gets assigned with the lower one
                        // i.e. swap variables if necessary
                        if (data_[variableid] != rhs_literal) {
                            if (literal_t__is_variable(rhs_literal) && rhs_literal > lhs_literal) {
                                std::swap(rhs_literal, lhs_literal);
                                continue;
                            } else if (data_[variableid] == lhs_literal || literal_t__is_unassigned(data_[variableid])) {
                                data_[variableid] = rhs_literal;
                                changes_count++;
                            } else if (literal_t__is_negation_of(data_[variableid], lhs_literal)) {
                                data_[variableid] = literal_t__negated(rhs_literal);
                                changes_count++;
                            } else if (literal_t__is_constant(data_[variableid]) || literal_t__is_variable(data_[variableid])) {
                                lhs_literal = data_[variableid];
                                continue;
                            } else {
                                return VARIABLEID_ERROR;
                            };
                        };
                    } else if (literal_t__is_constant(lhs_literal) && literal_t__is_variable(rhs_literal)) {
                        // { const = var } => { var = const }
                        std::swap(rhs_literal, lhs_literal);
                        continue;
                    } else if (lhs_literal != rhs_literal) {
                        // any remaining literals must match whatever they are
                        return VARIABLEID_ERROR;
                    };
                    break;
                };
            };
            return changes_count;
        };
        
        // set values from template_ to value
        // if src contains a variable, resolve it from self
        // template_ and value can be the same instance
        inline void assign_template_into(const VariablesArray& template_, VariablesArray& value) const {
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
                
        typedef enum { aftmComplete, aftmConstant, aftmDifference } assign_from_template_mode_t;
        inline void assign_from_template(const VariablesArray& values, const VariablesArray& template_,
                                         const assign_from_template_mode_t mode) {
            // resize to match
            resize(template_.size_);
            element_size_ = template_.element_size_;
            
            if (mode == aftmComplete) {
                values.assign_template_into(template_, *this);
            } else {
                for (auto i = 0; i < size_; i++) {
                    literalid_t value = LITERALID_UNASSIGNED;
                    if (literal_t__is_variable(template_.data_[i])) {
                        assert(literal_t__variable_id(template_.data_[i]) < values.size_);
                        value = literal_t__lookup(values.data_, template_.data_[i]);
                    } else {
                        value = template_.data_[i];
                    };
                    
                    if ((mode == aftmConstant && literal_t__is_constant(value)) ||
                        (mode == aftmDifference && value != template_.data_[i])) {
                        data_[i] = value;
                    } else {
                        data_[i] = LITERALID_UNASSIGNED;
                    };
                };
            };
        };
        
        inline bool contains(const variableid_t variable_id) const {
            for (container_size_t i = 0; i < size_; i++) {
                if (literal_t__variable_id(data_[i]) == variable_id) {
                    return true;
                };
            };
            return false;
        };
                
        inline literalid_t* const data() { return data_; };
        inline const literalid_t* const data() const { return data_; };
        inline container_size_t size() const { return size_; };
        inline container_size_t element_size() const { return element_size_; };
    };
    
};

#endif /* variablesarray_hpp */
