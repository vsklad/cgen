//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef variables_hpp
#define variables_hpp

#include <assert.h>
#include <stdint.h>
#include <ostream>

namespace ple {
    // types and constants for propositional variables & literals
    // a variable is identified by an unsigned integer value
    // however, its maximal value excludes one bit
    // this is to enable 32 bit literals for some models
    
    typedef uint32_t variableid_t;
    
    // size of an array with variable values
    typedef variableid_t variables_size_t;

    static constexpr variableid_t VARIABLEID_MIN = 0;
    static constexpr variableid_t VARIABLEID_MAX = ((UINT32_MAX - 1) >> 1) - 1;
    
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
    
#define literal_t__constant(value) ((value) ? ple::LITERAL_CONST_1 : ple::LITERAL_CONST_0)
    
// convert variable id to literal id
#define variable_t__literal_id_negated_onlyif(id, onlyif) \
    (ple::literalid_t)((((id) + 1) << 1) | ((onlyif) ? 0 : 1))
#define variable_t__literal_id(id) variable_t__literal_id_negated_onlyif(id, 0)
    
#define literal_t__is_negation(id) (bool)(((id) & 1) == 0)
#define literal_t__is_negation_of(id, other_id) (((id) ^ (other_id)) == 1)
#define literal_t__is_constant(id) (bool)((id) <= ple::LITERAL_CONST_1)
#define literal_t__is_constant_0(id) (bool)((id) == LITERAL_CONST_0)
#define literal_t__is_constant_1(id) (bool)((id) == LITERAL_CONST_1)
#define literal_t__is_variable(id) (bool)((id) > LITERAL_CONST_1 && (id) <= LITERALID_MAX)
#define literal_t__is_unassigned(id) (bool)((id) == LITERALID_UNASSIGNED)
#define literal_t__is_same_variable(lhs, rhs) (bool)((((lhs) ^ (rhs)) >> 1) == 0)
#define literal_t__variable_id(id) (ple::variableid_t)(((id) >> 1) - 1)
#define literal_t__negated_onlyif(id, onlyif) ((id) ^ (onlyif ? 1 : 0))
#define literal_t__negated(id) literal_t__negated_onlyif(id, 1)
#define literal_t__unnegated(id) ((id) | 1)
#define literal_t__sequence_next(id, step_size) (id) + ((step_size) << 1)
    
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
 
    class literal_t;
    
    class variable_t {
    public:
        const variableid_t id;
        
        inline variable_t(const variableid_t variableid);
        inline variable_t(const literal_t literal);
    };

    class literal_t {
        friend class variable_t;

    private:
        literalid_t id_;
    
    public:
        inline literal_t(): id_(LITERAL_CONST_0) {};
        inline literal_t(const literalid_t literalid): id_(literalid) {};
        inline literal_t(const variable_t variable): id_(variable_t__literal_id(variable.id)) {};
        
        inline const literalid_t id() const { return id_; };
        
        inline const variableid_t variable_id() const {
            return literal_t__variable_id(id_);
        };
        
        inline bool is_constant() const {
            return literal_t__is_constant(id_);
        };
        
        inline bool is_constant_0() const {
            return literal_t__is_constant_0(id_);
        };
        
        inline bool is_constant_1() const {
            return literal_t__is_constant_1(id_);
        };
        
        inline bool is_variable() const {
            return literal_t__is_variable(id_);
        };
        
        inline bool is_unassigned() const {
            return literal_t__is_unassigned(id_);
        };
        
        inline bool is_negation() const {
            return literal_t__is_negation(id_);
        };
        
        inline bool is_negation_of(const literal_t& other) const {
            return literal_t__is_negation_of(id_, other.id_);
        };
        
        inline void negate(bool onlyif = true) {
            literal_t__negate_onlyif(id_, onlyif);
        };

        inline const literalid_t negated(bool onlyif = true) const {
            return literal_t__negated_onlyif(id_, onlyif);
        };
        
        inline bool operator == (const literal_t& other) const {
            return id_ == other.id_;
        };
        
        inline bool operator != (const literal_t& other) const {
            return id_ != other.id_;
        };
        
        // static versions
        
        inline static const literal_t constant(const bool value) {
            return literal_t(literal_t__constant(value));
        };
        
        // determines if there is a sequence of variables in the data array
        // supports both asc and desc, determines step size
        // if no elements, returns sequence_size = 0, sequence_step = 0
        // if no sequence, returns sequence_size = 1, sequence_step = 0
        // otherwise returns sequence_size and step_size
        // sequence_step = 0 means the same variable is repeated
        
        // returns positive number of elements if asc, negative if desc
        // 1 if no sequence, 0 if no elements or a constant
        inline static void get_variables_sequence(const literalid_t* const data, const size_t size,
                                                  size_t& sequence_size, int& sequence_step) {
            sequence_size = 0;
            sequence_step = 0;
            
            if (size > 0 && !literal_t(data[0]).is_constant()) {
                sequence_size = 1;
                // determine direction
                if (size > 1 && literal_t(data[1]).is_variable()) {
                    sequence_step = data[sequence_size] - data[sequence_size-1];
                    // difference 1 means opposit negation, i.e. no sequence
                    if ((sequence_step & 0x1) == 0) {
                        // determine length, already have 2
                        sequence_size = 2;
                        while (sequence_size < size) {
                            if (literal_t(data[sequence_size]).is_variable()) {
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
                };
            };
        };
        
        inline static void get_variables_range_sequence(const literalid_t* const data, const size_t size,
                                                        const size_t range_size,
                                                        size_t& sequence_size, int& sequence_step) {
            assert(size % range_size == 0); // data size is a multiple of range_size
            
            sequence_size = size > 0 ? 1 : 0;
            sequence_step = 0;
            
            if (size > range_size) {
                // assume the step from the first elements of the first and second ranges
                // stop when this step is not the same at any point
                sequence_step = data[range_size] - data[0];
                
                const literalid_t* p_prev = data;
                const literalid_t* p_curr = p_prev + range_size;
                
                while (p_curr < data + size) {
                    bool b_sequence_step_applies = true;
                    const literalid_t* p_next = p_curr + range_size;
                    
                    while(p_curr < p_next) {
                        if (*p_curr - *p_prev != sequence_step) {
                            b_sequence_step_applies = false;
                            break;
                        };
                        p_prev++;
                        p_curr++;
                    };
                    
                    if (!b_sequence_step_applies) {
                        break;
                    };
                    
                    sequence_size++;
                    p_next += range_size;
                };
                
                // 2 between literals, 1 between variables
                sequence_step >>= 1;
            };
        };

        inline static literalid_t resolve(const literalid_t* const table, literalid_t value) {
            // TODO: check assignment of self-negated
            literalid_t result = value;
            
            while (literal_t__is_variable(value)) {
                result = table[(result >> 1) - 1];
                result = (result == LITERALID_UNASSIGNED) ? LITERALID_UNASSIGNED : result ^ (~value & 1);
                if (result == value) {
                    break;
                } else {
                    value = result;
                };
            };
            
            return result;
        };
        
        // DIMACS compatible representation
        
        inline static const literalid_t signed_encode(int32_t value) {
            return value > 0 ? (value << 1) | 0x1 : (-value << 1);
        };
        
        friend std::ostream& operator << (std::ostream& stream, const literal_t& value) {
            if (value.is_variable()) {
                if (value.is_negation()) {
                    stream << "-";
                };
                stream << std::dec << (value.variable_id() + 1);
            } else if (value.is_unassigned()) {
                stream << "*";
            } else {
                stream << (value.is_constant_1() ? "0b1" : "0b0");
            };
            return stream;
        };
    };
    
    // check variableid_t range
    inline variable_t::variable_t(const variableid_t variableid): id(variableid) {
        assert(variableid <= VARIABLEID_MAX);
    };

    // check for constants, remove negation, convert id
    inline variable_t::variable_t(const literal_t literal): id(literal.variable_id()) {};

    class VariableGenerator {
    private:
        variableid_t next_ = VARIABLEID_MIN;
    protected:
        inline void reset(const variableid_t start_value) { next_ = start_value; };
        inline variableid_t next() const { return next_; };
    public:
        virtual const variableid_t new_variable() { return next_++; };
        virtual const literalid_t new_variable_literal() { return variable_t__literal_id(new_variable()); };
        virtual const variableid_t last_variable() const { return next_ - 1; };
        
        inline void generate_unassigned(literalid_t* const data, const size_t size) {
            for (auto p_item = data; p_item < data + size; p_item++) {
                if (*p_item == LITERALID_UNASSIGNED) {
                    *p_item = new_variable_literal();
                };
            };
        };
    };
};

#endif /* variables_hpp */
