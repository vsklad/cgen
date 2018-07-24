//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef anfwordadd_hpp
#define anfwordadd_hpp

namespace ple {

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

    template<WordSize WORD_SIZE>
    inline void word_add2(Word<WORD_SIZE, AnfEncoderBit>* r, const Word<WORD_SIZE, AnfEncoderBit>* x, const Word<WORD_SIZE, AnfEncoderBit>* y) {
        Ref<AnfEncoderBit> x_p; // previous x
        Ref<AnfEncoderBit> y_p; // previous y
        Ref<AnfEncoderBit> r_p; // previous r
        Ref<AnfEncoderBit> x_i;
        Ref<AnfEncoderBit> y_i;
        Ref<AnfEncoderBit> r_i;
        
        for (auto i = 0; i < WORD_SIZE; i++) {
            x_i = (*x)[i];
            y_i = (*y)[i];
            r_i.new_instance();
            
            if (i == 0) {
                r_i->eor2(x_i, y_i);
            } else {
                r_i->add2_no_carry(x_i, y_i, x_p, y_p, r_p);
            };
            
            x_p = x_i;
            y_p = y_i;
            r_p = r_i;
            
            (*r)[i] = r_i;
        };
    };
 
}

#endif /* anfwordadd_hpp */
