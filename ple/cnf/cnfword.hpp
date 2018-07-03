//
//  Propositional Logic Engine (PLE) Library
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfword_hpp
#define cnfword_hpp

namespace ple {
    
    // specialized templates for CnfEncoderBit
    // conversion into literals array
    // words stored to the array in big endian format, i.e. with most significant bit first
    // at the same time, other arrays come sequentially, with aray index going from low to high
    
    template<WordSize WORD_SIZE>
    inline void Word2VariablesArray(const Ref<Word<WORD_SIZE, CnfEncoderBit>>& value, VariablesArray& array) {
        literalid_t literals[WORD_SIZE];
        for (variableid_t i = 0; i < WORD_SIZE; i++) {
            assert((*value)[i] != nullptr);
            literals[i] = (*value)[WORD_SIZE - i - 1]->literal().id();
        };
        array.assign(literals, WORD_SIZE, 0);
    };

    template<WordSize WORD_SIZE>
    inline void Words2VariablesArray(const RefArray<Word<WORD_SIZE, CnfEncoderBit>>& value, VariablesArray& array) {
        for (variableid_t i = 0; i < value.size(); i++) {
            VariablesArray w(WORD_SIZE);
            Word2VariablesArray(value[i], w);
            array.assign_element(w, i);
        };
    };
};

#endif /* cnfword_hpp */
