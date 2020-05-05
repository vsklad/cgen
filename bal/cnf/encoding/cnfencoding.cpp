//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include "cnfencoding.hpp"

namespace bal {
 
    // assume all arguments are variables, no duplicates
    // assume size at least two arguments
    void eor(Cnf* const formula, const literalid_t r, const literalid_t args[], const std::size_t args_size) {
        // more than one variable, generate 2^(N-1) clauses
        _assert_level_0(formula != nullptr);
        _assert_level_1(args_size > 1 && args_size < MAX_XOR_SIZE); // args_size + 1 for result
        literalid_t clause_args[args_size + 1];
        
        for (uint32_t i = 0; i < (1 << args_size); i++) {
            // i bits can be used to determine the negation and the result
            literalid_t constant = 0b0;
            for (auto j = 0; j < args_size; j++) {
                clause_args[j] = literal_t__negated_onlyif(args[j], i & (0x1 << j));
                literal_t__negate_onlyif(constant, i & (0x1 << j));
            };
            clause_args[args_size] = literal_t__negated_onlyif(r, literal_t__is_constant_0(constant));
            formula->append_clause(clause_args, args_size + 1);
        };
    };
    
    void con2(Cnf* const formula, const literalid_t r, const literalid_t x, const literalid_t y) {
        _assert_level_1(formula != nullptr);
        _assert_level_1(literal_t__is_variable(r));
        _assert_level_1(literal_t__is_variable(x));
        _assert_level_1(literal_t__is_variable(y));
        formula->append_clause_l(literal_t__negated(x), literal_t__negated(y), r);
        formula->append_clause_l(x, literal_t__negated(r));
        formula->append_clause_l(y, literal_t__negated(r));
    };
    
    void dis2(Cnf* const formula, const literalid_t r, const literalid_t x, const literalid_t y) {
        _assert_level_1(formula != nullptr);
        _assert_level_1(literal_t__is_variable(r));
        _assert_level_1(literal_t__is_variable(x));
        _assert_level_1(literal_t__is_variable(y));
        formula->append_clause_l(x, y, literal_t__negated(r));
        formula->append_clause_l(literal_t__negated(x), r);
        formula->append_clause_l(literal_t__negated(y), r);
    };
    
    // x&y ^ ~x&z
    // (¬f ∨ ¬x ∨ y) ∧ (¬f ∨ x ∨ z) ∧ (¬f ∨ y ∨ z) ∧ (f ∨ ¬x ∨ ¬y) ∧
    // (f ∨ x ∨ ¬z) ∧ (f ∨ ¬y ∨ ¬z)
    void ch(Cnf* const formula, const literalid_t r, const literalid_t x, const literalid_t y, const literalid_t z) {
        _assert_level_1(formula != nullptr);
        _assert_level_1(literal_t__is_variable(r));
        _assert_level_1(literal_t__is_variable(x));
        _assert_level_1(literal_t__is_variable(y));
        _assert_level_1(literal_t__is_variable(z));
        formula->append_clause_l(literal_t__negated(x), y, literal_t__negated(r));
        formula->append_clause_l(x, z, literal_t__negated(r));
        formula->append_clause_l(literal_t__negated(x), literal_t__negated(y), r);
        formula->append_clause_l(x, literal_t__negated(z), r);
    };
    
    // xy ∨ xz ∨ yz
    // (¬f ∨ x ∨ y) ∧ (¬f ∨ x ∨ z) ∧ (¬f ∨ y ∨ z) ∧
    // (f ∨ ¬x ∨ ¬y) ∧ (f ∨ ¬x ∨ ¬z) ∧ (f ∨ ¬y ∨ ¬z)
    void maj(Cnf* const formula, const literalid_t r, const literalid_t x, const literalid_t y, const literalid_t z) {
        _assert_level_1(formula != nullptr);
        _assert_level_1(literal_t__is_variable(r));
        _assert_level_1(literal_t__is_variable(x));
        _assert_level_1(literal_t__is_variable(y));
        _assert_level_1(literal_t__is_variable(z));
        formula->append_clause_l(x, y, literal_t__negated(r));
        formula->append_clause_l(x, z, literal_t__negated(r));
        formula->append_clause_l(y, z, literal_t__negated(r));
        formula->append_clause_l(literal_t__negated(x), literal_t__negated(y), r);
        formula->append_clause_l(literal_t__negated(x), literal_t__negated(z), r);
        formula->append_clause_l(literal_t__negated(y), literal_t__negated(z), r);
    };
    
    // constant is an additional constant operand, can be ignored if 0b0
    // b_2nd_c1 means that a second first carry should be produced instead of a second carry
    // see cnfaddmap.hpp
    void add(Cnf& formula, literalid_t args[],
             const std::size_t input_size, const std::size_t output_size,
             const literalid_t constant, const bool b_2nd_c1) {
        _assert_level_0(input_size > 1);
        _assert_level_0(output_size >= 1 && output_size <= 3);
        _assert_level_0(literal_t__is_constant(constant) && (constant == 0 || constant == 1));
        _assert_level_0(!b_2nd_c1 || input_size == 3);
        _assert_level_0(!b_2nd_c1 || output_size == 2);
        
        bool is_2nd_carry = output_size == 3 && !b_2nd_c1;
        const addition_map_entry_t* p_map = b_2nd_c1 ? &ADD_31_C1_I0O2xC1_MAP : &(ADD_MAP[input_size][constant][is_2nd_carry]);
        
        formula.record_clauses(p_map->map, p_map->map_size, args, input_size, output_size);
    };
    
}
