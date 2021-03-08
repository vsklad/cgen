//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef anfencoding_hpp
#define anfencoding_hpp

#include "referenceable.hpp"
#include "variables.hpp"
#include "gf2n.hpp"
#include "literal.hpp"
#include "anf.hpp"

namespace bal {

    void  eor(Anf* const formula, const literalid_t r, const literalid_t args[], const std::size_t args_size);
    void con2(Anf* const formula, const literalid_t r, const literalid_t x, const literalid_t y);
    void dis2(Anf* const formula, const literalid_t r, const literalid_t x, const literalid_t y);
    void  maj(Anf* const formula, const literalid_t r, const literalid_t x, const literalid_t y, const literalid_t z);
    void   ch(Anf* const formula, const literalid_t r, const literalid_t x, const literalid_t y, const literalid_t z);
    
    void add2(Anf* const formula, const literalid_t r, const literalid_t x, const literalid_t y,
              const literalid_t r_prev, const literalid_t x_prev, const literalid_t y_prev);
    
    //  Elimination of carry variables
    //
    // r[i] = x[i] + y[i] + c[i-1]  ==>  c[i-1] = x[i] + y[i] + r[i]
    // c[i] = maj(x[i], y[i], c[i-1]) ==>
    //        x[i]y[i] + x[i]c[i-1] + y[i]c[i-1] ==>
    //        x[i]y[i] + x[i](x[i] + y[i] + r[i]) + y[i](x[i] + y[i] + r[i]) ==>
    //        x[i]y[i] + x[i] + x[i]y[i] + x[i]r[i] + x[i]y[i] + y[i] + y[i]r[i] ==>
    //        x[i] + y[i] + x[i]y[i] + x[i]r[i] + y[i]r[i]
    // r[i+1] = x[i+1] + y[i+1] + c[i] ==>
    //          x[i+1] + y[i+1] + x[i] + y[i] + x[i]y[i] + x[i]r[i] + y[i]r[i]
    
    template<std::size_t N>
    Ref<GF2NElement<N, Literal<Anf>>> add2(GF2NElement<N, Literal<Anf>>* r,
                                           const GF2NElement<N, Literal<Anf>>* x,
                                           const GF2NElement<N, Literal<Anf>>* y) {
        static_assert(N > 0, "trivial");
        _assert_level_0(x != nullptr && y != nullptr);
        Ref<GF2NElement<N, Literal<Anf>>> result = new_instance_if_unassigned(r);
        
        // because it is possible that result == one of the operands
        literalid_t x_p = (*x)[0]->value;      // previous x
        literalid_t y_p = (*y)[0]->value;      // previous y
        (*result)[0] = eor2((*result)[0].data(), (*x)[0].data(), (*y)[0].data());
        literalid_t r_p = (*result)[0]->value; // previous r
        
        Anf* formula = (*result)[0]->formula;
        update_formula(formula, (*x)[0]);
        update_formula(formula, (*y)[0]);
        
        for (auto i = 1; i < N; i++) {
            literalid_t r_i;
            const literalid_t x_i = (*x)[i]->value;
            const literalid_t y_i = (*y)[i]->value;
            update_formula(formula, (*x)[i]);
            update_formula(formula, (*y)[i]);
            
            // attempt to compute result
            // if it is expressed as a single literal then there is no need for expressions
            // x_i + y_i + x_p + y_p + x_p * y_p + x_p * r_p + y_p * r_p
            bool b_calculated = false;
            r_i = x_i;
            if (literal_t__is_constant(r_i) || literal_t__is_constant(y_i) || literal_t__is_same_variable(r_i, y_i)) {
                r_i ^= y_i;
                if (literal_t__is_constant(r_i) || literal_t__is_constant(x_p) || literal_t__is_same_variable(r_i, x_p)) {
                    r_i ^= x_p;
                    if (literal_t__is_constant(r_i) || literal_t__is_constant(y_p) || literal_t__is_same_variable(r_i, y_p)) {
                        r_i ^= y_p;
                        literalid_t r_exp;
                        if (literal_t__is_constant_0(x_p) || literal_t__is_constant_0(y_p) || literal_t__is_negation_of(x_p, y_p)) {
                            r_exp = LITERAL_CONST_0;
                        } else if (literal_t__is_constant_1(y_p) || literal_t__is_same_variable(x_p, y_p)) {
                            r_exp = x_p;
                        } else if (literal_t__is_constant_1(x_p)) {
                            r_exp = y_p;
                        } else {
                            r_exp = LITERALID_UNASSIGNED;
                        };
                        
                        if (!literal_t__is_unassigned(r_exp) &&
                            (literal_t__is_constant(r_i) || literal_t__is_constant(r_exp) || literal_t__is_same_variable(r_i, r_exp))) {
                            r_i ^= r_exp;
                            
                            if (literal_t__is_constant_0(x_p) || literal_t__is_constant_0(r_p) || literal_t__is_negation_of(x_p, r_p)) {
                                r_exp = LITERAL_CONST_0;
                            } else if (literal_t__is_constant_1(r_p) || literal_t__is_same_variable(x_p, r_p)) {
                                r_exp = x_p;
                            } else if (literal_t__is_constant_1(x_p)) {
                                r_exp = r_p;
                            } else {
                                r_exp = LITERALID_UNASSIGNED;
                            };
                            
                            if (!literal_t__is_unassigned(r_exp) &&
                                (literal_t__is_constant(r_i) || literal_t__is_constant(r_exp) || literal_t__is_same_variable(r_i, r_exp))) {
                                r_i ^= r_exp;

                                if (literal_t__is_constant_0(y_p) || literal_t__is_constant_0(r_p) || literal_t__is_negation_of(y_p, r_p)) {
                                    r_exp = LITERAL_CONST_0;
                                } else if (literal_t__is_constant_1(r_p) || literal_t__is_same_variable(y_p, r_p)) {
                                    r_exp = y_p;
                                } else if (literal_t__is_constant_1(y_p)) {
                                    r_exp = r_p;
                                } else {
                                    r_exp = LITERALID_UNASSIGNED;
                                };
                                
                                if (!literal_t__is_unassigned(r_exp) &&
                                    (literal_t__is_constant(r_i) || literal_t__is_constant(r_exp) || literal_t__is_same_variable(r_i, r_exp))) {
                                    r_i ^= r_exp;
                                    b_calculated = true;
                                };
                            };
                        };
                    };
                };
            };
                
            if (b_calculated) {
                _assert_level_1(!literal_t__is_unassigned(r_i));
                (*result)[i] = Literal<Anf>::from_literal(formula, r_i);
            } else {
                _assert_level_1(formula != nullptr);
                r_i = formula->new_variable_literal();
                add2(formula, r_i, x_i, y_i, r_p, x_p, y_p);
                (*result)[i] = new Literal<Anf>(formula, r_i);
            };
            
            x_p = x_i;
            y_p = y_i;
            r_p = r_i;
        };
        
        return result;
    };
};

#endif /* anfencoding_hpp */
