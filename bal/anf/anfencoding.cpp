//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include "anfencoding.hpp"

namespace bal {
    
    // assume all arguments are variables, no duplicates
    // assume size at least two arguments
    void eor(Anf* const formula, const literalid_t r, const literalid_t args[], const std::size_t args_size) {
        _assert_level_0(formula != nullptr);
        _assert_level_0(literal_t__is_variable(r) || literal_t__is_unassigned(r));
        formula->append_equation();
        for (auto i = 0; i < args_size; i++) {
            _assert_level_0(literal_t__is_variable(args[i]));
            formula->append_equation_term(args[i]);
        };
        formula->complete_equation(r, literal_t__is_unassigned(r));
    };
    
    void con2(Anf* const formula, const literalid_t r, const literalid_t x, const literalid_t y) {
        _assert_level_0(formula != nullptr);
        _assert_level_0(literal_t__is_variable(r) || literal_t__is_unassigned(r));
        formula->append_equation();
        formula->append_equation_term(x, y);
        formula->complete_equation(r, literal_t__is_unassigned(r));
    };
    
    // x + y + xy
    void dis2(Anf* const formula, const literalid_t r, const literalid_t x, const literalid_t y) {
        _assert_level_0(formula != nullptr);
        _assert_level_0(literal_t__is_variable(r) || literal_t__is_unassigned(r));
        formula->append_equation();
        formula->append_equation_term(x);
        formula->append_equation_term(y);
        formula->append_equation_term(x, y);
        formula->complete_equation(r, literal_t__is_unassigned(r));
    };
    
    // xy ∨ xz ∨ yz
    void maj(Anf* const formula, const literalid_t r, const literalid_t x, const literalid_t y, const literalid_t z) {
        _assert_level_0(formula != nullptr);
        _assert_level_0(literal_t__is_variable(r) || literal_t__is_unassigned(r));
        formula->append_equation();
        formula->append_equation_term(x, y);
        formula->append_equation_term(x, z);
        formula->append_equation_term(y, z);
        formula->complete_equation(r, literal_t__is_unassigned(r));
    };
    
    // x&y ^ ~x&z = xy ^ xz ^ z
    void ch(Anf* const formula, const literalid_t r, const literalid_t x, const literalid_t y, const literalid_t z) {
        _assert_level_0(formula != nullptr);
        _assert_level_0(literal_t__is_variable(r) || literal_t__is_unassigned(r));
        formula->append_equation();
        formula->append_equation_term(x, y);
        formula->append_equation_term(x, z);
        formula->append_equation_term(z);
        formula->complete_equation(r, literal_t__is_unassigned(r));
    };
    
    // see __add2()
    void add2(Anf* const formula, const literalid_t r, const literalid_t x, const literalid_t y,
              const literalid_t r_prev, const literalid_t x_prev, const literalid_t y_prev) {
        _assert_level_0(formula != nullptr);
        _assert_level_0(literal_t__is_variable(r) || literal_t__is_unassigned(r));
        __add2(formula, r, x, y, r_prev, x_prev, y_prev);
    };

    // x + y + x_prev + y_prev + x_prev * y_prev + x_prev * r_prev + y_prev * r_prev
    // see Ref<GF2NElement<N, Literal<Anf>>> add2()
    literalid_t __add2(Anf* const formula, const literalid_t r, const literalid_t x, const literalid_t y,
                       const literalid_t r_prev, const literalid_t x_prev, const literalid_t y_prev) {
        _assert_level_0(formula != nullptr);
        formula->append_equation();
        formula->append_equation_term(x);
        formula->append_equation_term(y);
        formula->append_equation_term(x_prev);
        formula->append_equation_term(y_prev);
        formula->append_equation_term(x_prev, y_prev);
        formula->append_equation_term(x_prev, r_prev);
        formula->append_equation_term(y_prev, r_prev);
        return formula->complete_equation(r, literal_t__is_unassigned(r));
    };
};
