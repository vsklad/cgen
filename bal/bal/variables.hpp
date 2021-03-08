//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef variables_hpp
#define variables_hpp

#include <stdint.h>
#include <ostream>
#include "assertlevels.hpp"

namespace bal {
    // types and constants for propositional variables & literals
    // a variable is identified by an unsigned integer value
    // however, its maximal value excludes one bit
    // this is to enable 32 bit literals for some models
    
    typedef uint32_t variableid_t;
    
    // size of an array with variable values
    typedef variableid_t variables_size_t;

    static constexpr variableid_t VARIABLEID_MIN = 0;
    static constexpr variableid_t VARIABLEID_MAX = ((UINT32_MAX - 1) >> 1) - 1;
    static constexpr variableid_t VARIABLEID_UNASSIGNED = VARIABLEID_MAX + 1;
    static constexpr variables_size_t VARIABLEID_ERROR = VARIABLEID_UNASSIGNED;
    
    // literal is an expression which is a constant, a variable or its negation
    // represented as an unsigned int32
    // with the least significant bit indicating negation (0 - negated, 1 - normal)
    // this form simplifies ordering and comparison
    // variables are encoded as follows: literal = (variable + 1) << 1 | negation
    
    typedef uint32_t literalid_t;
    
    static constexpr literalid_t LITERALID_MAX = UINT32_MAX - 1;
    static constexpr literalid_t LITERALID_UNASSIGNED = UINT32_MAX;
    
    // the types allow storing constant values as well
    static constexpr literalid_t LITERAL_CONST_0 = 0;
    static constexpr literalid_t LITERAL_CONST_1 = 1;
    
#define literal_t__constant(value) ((value) ? bal::LITERAL_CONST_1 : bal::LITERAL_CONST_0)
    
// convert variable id to literal id
#define variable_t__literal_id_negated_onlyif(id, onlyif) \
    (bal::literalid_t)((((id) + 1) << 1) | ((onlyif) ? 0 : 1))
#define variable_t__literal_id(id) variable_t__literal_id_negated_onlyif(id, 0)
    
#define literal_t__is_negation(id) (bool)(((id) & 1) == 0)
#define literal_t__is_negation_of(id, other_id) (((id) ^ (other_id)) == 1)
#define literal_t__is_unnegated(id) (bool)(((id) & 1) == 1)
#define literal_t__is_constant(id) (bool)((id) <= bal::LITERAL_CONST_1)
#define literal_t__is_constant_0(id) (bool)((id) == bal::LITERAL_CONST_0)
#define literal_t__is_constant_1(id) (bool)((id) == bal::LITERAL_CONST_1)
#define literal_t__is_variable(id) (bool)((id) > bal::LITERAL_CONST_1 && (id) <= bal::LITERALID_MAX)
#define literal_t__is_unassigned(id) (bool)((id) == bal::LITERALID_UNASSIGNED)
#define literal_t__is_same_variable(lhs, rhs) (bool)((((lhs) ^ (rhs)) >> 1) == 0)
#define literal_t__variable_id(id) (bal::variableid_t)(((id) >> 1) - 1)
#define literal_t__negated_onlyif(id, onlyif) ((id) ^ ((onlyif) ? 1 : 0))
#define literal_t__negated(id) literal_t__negated_onlyif(id, 1)
#define literal_t__unnegated(id) ((id) | 1)
#define literal_t__sequence_next(id, step_size) (id) + ((step_size) << 1)
#define literal_t__sequence_next_is_valid(id, step_size) \
    (literal_t__is_variable(id) && ( \
        ((step_size) <= 0 && literal_t__variable_id(id) - bal::VARIABLEID_MIN >= -(step_size)) || \
        ((step_size) >  0 && bal::VARIABLEID_MAX - literal_t__variable_id(id) >=  (step_size))))

// note that the below modifies the id
#define literal_t__negate_onlyif(id, onlyif) (id) = literal_t__negated_onlyif(id, onlyif);
#define literal_t__negate(id) literal_t__negate_onlyif(id, 1);
#define literal_t__unnegate(id) (id) |= 1;
    
// returns literal_id with the sign applied from value
#define literal_t__substitute_literal(value, literal_id) \
    literal_t__negated_onlyif(literal_id, ~(value) & 1)
// converts variable_id into literal with the sign applied from value
#define literal_t__substitute_variable(value, variable_id) \
    literal_t__substitute_literal(value, variable_t__literal_id(variable_id))
// looks up value variable and applies value sign to the result
#define literal_t__lookup(variables, value) \
    literal_t__substitute_literal(value, variables[((value) >> 1) - 1])
// well, just exclusive or
#define literal_t__constant_eor(x, y) ((x) ^ (y))
#define literal_t__constant_eor_lhs(x, y) ((x) ^= (y))

    class literal_t {
    private:
        literalid_t id_;
    
    public:
        inline literal_t(const literalid_t literalid): id_(literalid) {};
         
        // static versions

        inline static literalid_t resolve(const literalid_t* const table, literalid_t value) {
            // TODO: check assignment of self-negated
            literalid_t result = value;
            
            while (literal_t__is_variable(value)) {
                result = table[literal_t__variable_id(result)];
                result = (result == LITERALID_UNASSIGNED) ? LITERALID_UNASSIGNED : literal_t__substitute_literal(value, result);
                if (result == value) {
                    break;
                } else {
                    value = result;
                };
            };
            
            return result;
        };
        
        // DIMACS compatible representation
        
        friend std::ostream& operator << (std::ostream& stream, const literal_t& value) {
            if (literal_t__is_variable(value.id_)) {
                if (literal_t__is_negation(value.id_)) {
                    stream << "-";
                };
                stream << std::dec << (literal_t__variable_id(value.id_) + 1);
            } else if (literal_t__is_unassigned(value.id_)) {
                stream << "*";
            } else {
                stream << (literal_t__is_constant_1(value.id_) ? "0b1" : "0b0");
            };
            return stream;
        };
    };
    
    // DIMACS compatible representation
    inline variableid_t variable_t__from_uint(const uint32_t value) {
        return (value > 0 && value <= VARIABLEID_MAX + 1) ? (value - 1) : VARIABLEID_UNASSIGNED;
    };
    
    inline variableid_t variable_t__from_cstr(const char * const value) {
        return variable_t__from_uint((uint32_t)std::strtoul(value, nullptr, 10));
    };
    
    inline literalid_t literal_t__from_sint(const int value) {
        return (value > 0) ? (value << 1) | 0x1 : ((-value) << 1);
    };
    
    inline literalid_t literal_t__from_cstr(const char * const value) {
        return literal_t__from_sint(std::atoi(value));
    };
     
    class VariableGenerator {
    private:
        variableid_t next_ = VARIABLEID_MIN;
    protected:
        inline void reset(const variableid_t start_value) {
            _assert_level_0(start_value < VARIABLEID_MAX);
            next_ = start_value;
        };
        inline variableid_t next() const { return next_; };
    public:
        virtual variableid_t new_variable() { return next_++; };
        virtual literalid_t new_variable_literal() { return variable_t__literal_id(new_variable()); };
        virtual variableid_t last_variable() const { return next_ - 1; };
        
        inline void generate_unassigned_variable_literals(literalid_t* const data, const size_t size) {
            for (auto p_item = data; p_item < data + size; p_item++) {
                if (*p_item == LITERALID_UNASSIGNED) {
                    *p_item = new_variable_literal();
                };
            };
        };
    };
};

#endif /* variables_hpp */
