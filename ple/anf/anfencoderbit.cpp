//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include "anfencoderbit.hpp"

namespace ple {

    // PropositionalLogicEntity

    // optimize to exclude all additional variables for any number of parameters
    // optimize out constants if any
    AnfEncoderBit* const AnfEncoderBit::eor(const AnfEncoderBit* const args[], const ArgsSize args_size) {
        assert(anf_ != nullptr);
        anf_->append_equation();
        for (auto i = 0; i < args_size; i++) {
            anf_->append_equation_term(args[i]->value_);
        };
        value_ = anf_->complete_equation();
        return this;
    };

    // PropositionalLogicEntity2

    AnfEncoderBit* const AnfEncoderBit::con2(const AnfEncoderBit* const x, const AnfEncoderBit* const y) {
        assert(anf_ != nullptr);
        anf_->append_equation();
        anf_->append_equation_term(x->value_, y->value_);
        value_ = anf_->complete_equation();
        return this;
    };

    // x + y + xy
    AnfEncoderBit* const AnfEncoderBit::dis2(const AnfEncoderBit* const x, const AnfEncoderBit* const y) {
        assert(anf_ != nullptr);
        anf_->append_equation();
        anf_->append_equation_term(x->value_);
        anf_->append_equation_term(y->value_);
        anf_->append_equation_term(x->value_, y->value_);
        value_ = anf_->complete_equation();
        return this;
    };

    AnfEncoderBit* const AnfEncoderBit::eor2(const AnfEncoderBit* const x, const AnfEncoderBit* const y) {
        const AnfEncoderBit* const args[] = {x, y};
        return eor(args, 2);
    };

    // xy ∨ xz ∨ yz
    AnfEncoderBit* const AnfEncoderBit::maj(const AnfEncoderBit* const x,
                                            const AnfEncoderBit* const y, const AnfEncoderBit* const z) {
        assert(anf_ != nullptr);
        anf_->append_equation();
        anf_->append_equation_term(x->value_, y->value_);
        anf_->append_equation_term(x->value_, z->value_);
        anf_->append_equation_term(y->value_, z->value_);
        value_ = anf_->complete_equation();
        return this;
    };

    // x&y ^ ~x&z = xy ^ xz ^ z
    AnfEncoderBit* const AnfEncoderBit::ch(const AnfEncoderBit* const x,
                                    const AnfEncoderBit* const y, const AnfEncoderBit* const z) {
        assert(anf_ != nullptr);
        anf_->append_equation();
        anf_->append_equation_term(x->value_, y->value_);
        anf_->append_equation_term(x->value_, z->value_);
        anf_->append_equation_term(z->value_);
        value_ = anf_->complete_equation();
        return this;
    };

    // x + y + x_prev * y_prev + x_prev * r_prev + y_prev * r_prev
    // see anfwordadd.hpp
    void AnfEncoderBit::add2_no_carry(const AnfEncoderBit* const x, const AnfEncoderBit* const y,
                       const AnfEncoderBit* const x_prev, const AnfEncoderBit* const y_prev,
                       const AnfEncoderBit* const r_prev) {
        assert(anf_ != nullptr);
        anf_->append_equation();
        anf_->append_equation_term(x->value_);
        anf_->append_equation_term(y->value_);
        anf_->append_equation_term(x_prev->value_);
        anf_->append_equation_term(y_prev->value_);
        anf_->append_equation_term(x_prev->value_, y_prev->value_);
        anf_->append_equation_term(x_prev->value_, r_prev->value_);
        anf_->append_equation_term(y_prev->value_, r_prev->value_);
        value_ = anf_->complete_equation();
    };
    
}
