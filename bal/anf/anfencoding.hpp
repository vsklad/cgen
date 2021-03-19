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
    literalid_t __add2(Anf* const formula, const literalid_t r, const literalid_t x, const literalid_t y,
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
            const literalid_t x_i = (*x)[i]->value;
            const literalid_t y_i = (*y)[i]->value;
            update_formula(formula, (*x)[i]);
            update_formula(formula, (*y)[i]);
            
            // formula instance is undefined when all operands are constant
            // but the result can be easily calculated and no extra variables required
            if (literal_t__is_constant(x_i) && literal_t__is_constant(y_i) &&
                literal_t__is_constant(r_p) && literal_t__is_constant(x_p) && literal_t__is_constant(y_p)) {
                r_p = x_i ^ y_i ^ x_p ^ y_p ^ (x_p & y_p) ^ (x_p & r_p) ^ (y_p & r_p);
                _assert_level_2(literal_t__is_constant(r_p));
            } else {
                r_p = __add2(formula, LITERALID_UNASSIGNED, x_i, y_i, r_p, x_p, y_p);
                _assert_level_2(literal_t__is_constant(r_p) || literal_t__is_variable(r_p));
            };
            
            (*result)[i] = Literal<Anf>::from_literal(formula, r_p);
            
            x_p = x_i;
            y_p = y_i;
        };
        
        return result;
    };
};

#endif /* anfencoding_hpp */
