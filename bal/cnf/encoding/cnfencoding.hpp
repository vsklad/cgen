//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfencoding_hpp
#define cnfencoding_hpp

#include "gf2n.hpp"
#include "literal.hpp"
#include "literaladd.hpp"
#include "cnfaddmap.hpp"
#include "cnf.hpp"

namespace bal {
    
    constexpr unsigned MAX_XOR_SIZE = 32;
    
    void  eor(Cnf* const formula, const literalid_t r, const literalid_t args[], const std::size_t args_size);
    void con2(Cnf* const formula, const literalid_t r, const literalid_t x, const literalid_t y);
    void dis2(Cnf* const formula, const literalid_t r, const literalid_t x, const literalid_t y);
    void   ch(Cnf* const formula, const literalid_t r, const literalid_t x, const literalid_t y, const literalid_t z);
    void  maj(Cnf* const formula, const literalid_t r, const literalid_t x, const literalid_t y, const literalid_t z);

    void add(Cnf& formula, literalid_t args[],
                      const std::size_t input_size, const std::size_t output_size,
                      const literalid_t constant, const bool b_2nd_c1);
    
    template<std::size_t N>
    Ref<GF2NElement<N, Literal<Cnf>>> add2(GF2NElement<N, Literal<Cnf>>* const r,
                                            const GF2NElement<N, Literal<Cnf>>* const x,
                                            const GF2NElement<N, Literal<Cnf>>* const y) {
        Ref<GF2NElement<N, Literal<Cnf>>> result = new_instance_if_unassigned(r);
        const GF2NElement<N, Literal<Cnf>>* args[2] = { x, y };
        literal_word_add<N, Literal<Cnf>, add>(result, args, 2);
        return result;
    };

    template<std::size_t N>
    Ref<GF2NElement<N, Literal<Cnf>>> add(GF2NElement<N, Literal<Cnf>>* const r,
                                           const GF2NElement<N, Literal<Cnf>>* const args[],
                                           const std::size_t args_size) {
        Ref<GF2NElement<N, Literal<Cnf>>> result = new_instance_if_unassigned(r);
        literal_word_add<N, Literal<Cnf>, add>(result, args, args_size);
        return result;
    };
};

#endif /* cnfencoding_hpp */
