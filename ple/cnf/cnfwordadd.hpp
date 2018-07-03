//
//  Propositional Logic Engine (PLE) Library
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfwordadd_hpp
#define cnfwordadd_hpp

#include "word.hpp"
#include "cnfencoderbit.hpp"
#include "cnfaddmap.hpp"

namespace ple {
    
    // TODO: optimize cases where x = y = c ?
    // TODO: check what remainst of a constant ?
    template<WordSize WORD_SIZE>
    inline void word_add2(Word<WORD_SIZE, CnfEncoderBit>* r, const Word<WORD_SIZE, CnfEncoderBit>* x, const Word<WORD_SIZE, CnfEncoderBit>* y) {
        
        Ref<CnfEncoderBit> c;
        c.new_instance();
        
        for (auto i = 0; i < WORD_SIZE; i++) {
            Ref<CnfEncoderBit> r_i;
            r_i.new_instance();
            
            if (i < WORD_SIZE - 1) {
                if ((*x)[i]->is_constant() || (*y)[i]->is_constant() || c->is_constant() || (*x)[i] == (*y)[i] || (*x)[i] == c || (*y)[i] == c) {
                    Ref<CnfEncoderBit> c_out;
                    c_out.new_instance();
                    c_out->maj((*x)[i], (*y)[i], c);
                    
                    const CnfEncoderBit* const args[3] = {(*x)[i], (*y)[i], c};
                    r_i->eor(args, 3);
                    
                    c = c_out;
                } else {
                    CnfEncoderBit* const args[5] = {(*x)[i], (*y)[i], c, r_i, c};
                    CnfEncoderBit::record_clauses(ADD_21_FC, ADD_21_FC_SIZE, args, 3, 2);
                };
            } else {
                const CnfEncoderBit* const args[3] = {(*x)[i], (*y)[i], c};
                r_i->eor(args, 3);
            };
            (*r)[i] = r_i;
        };
    };
    
    // add carry to the end of the list
    // carry_size is a number of variables at the end of the list that are carry
    inline void add_append_carry_(const Ref<CnfEncoderBit>& arg, std::vector<Ref<CnfEncoderBit>>& variables) {
        if (!arg->literal().is_constant_0()) {
            variables.push_back(arg);
        };
    };
    
    // process a carry or an operand
    // either add to variables array or increase the constant
    inline void add_append_variable_(const Ref<CnfEncoderBit>& arg, std::vector<CnfEncoderBit*>& variables, std::vector<Ref<CnfEncoderBit>>& carry_out_1, WordValue& constant) {
        if (arg->is_constant()) {
            constant += (BitValue)*arg;
        } else {
            // check for duplicates including negation
            for (auto i = 0; i < variables.size(); i++) {
                if (arg->literal().variable_id() == variables[i]->literal().variable_id()) {
                    if (arg->literal().is_negation_of(variables[i]->literal())) {
                        // if negation, x + !x <=> 1; remove variables[i] and add 1
                        constant += 1;
                    } else {
                        add_append_carry_(arg, carry_out_1);
                    };
                    
                    // remove the variable
                    if (variables.size() > 1) {
                        variables[i] = variables[variables.size() - 1];
                    };
                    variables.resize(variables.size() - 1);
                    
                    return;
                };
            };
            variables.push_back(arg);
        };
    };
    
    template<WordSize WORD_SIZE>
    inline Ref<Word<WORD_SIZE, CnfEncoderBit>> add(const RefArray<Word<WORD_SIZE, CnfEncoderBit>> &args) {
        
        Ref<Word<WORD_SIZE, CnfEncoderBit>> result;
        result.new_instance();
        
        assert(args.size() <= 5); // this implementation only support 5, also see cnfaddmap.hpp
        
        // result of the bit add
        Ref<CnfEncoderBit> r;
        // there is a constant carry and variable carry, for each carry bit
        // constant is a mix of constant operand values and constant carry
        WordValue constant = 0;
        // lists to keep carry
        std::vector<Ref<CnfEncoderBit>> carry_in;
        std::vector<Ref<CnfEncoderBit>> carry_out_1;
        std::vector<Ref<CnfEncoderBit>> carry_out_21;
        std::vector<Ref<CnfEncoderBit>> carry_out_22;
        
        // variables buffer
        // all elements must actually have Ref among args, r, c1, c12 or c22
        std::vector<CnfEncoderBit*> variables;
        
        for (auto i = 0; i < WORD_SIZE; i++) {
            // determine number of variables
            // take carry out from the previous bit
            for (ArgsSize j = 0; j < carry_in.size(); j++) {
                add_append_variable_(carry_in[j], variables, carry_out_1, constant);
            };
            // filter out variables from args
            for (ArgsSize j = 0; j < args.size(); j++) {
                add_append_variable_((*args[j])[i], variables, carry_out_1, constant);
            };
            
            ArgsSize input_size = variables.size();
            
            if (i == WORD_SIZE - 1) {
                // no need for carry for the last bit, only assign result bit
                r.new_instance();
                r->eor(variables.data(), input_size);
                if (constant & 0x1) {
                    r->inv(r);
                };
            } else {
                if (input_size == 0) {
                    // all operands are constant, take the lowest bit
                    r = BitValue(constant & 0x1);
                } else if (input_size == 1) {
                    if (constant & 0b1) {
                        // result is the inversion of the single variable argument
                        r.new_instance();
                        r->inv(variables[0]);
                        add_append_carry_(variables[0], carry_out_1);
                    } else {
                        // result is the single variable argument
                        r = variables[0];
                    };
                } else {
                    // for SHA1, 6 variables is the max possible
                    // addition of 4 variables + 1 constant
                    // i.e. 4 input + 2 carry in
                    // for other purposes, ADD_MAP may be extended
                    //assert(input_size < 7);
                    
                    // determine number of variable to add at once
                    // there are at least 2 variables
                    // cnf must always be assigned for a variable
                    const ArgsSize batch_size = variables[0]->cnf()->get_add_max_args();
                    
                    // now take batches of variables and add them up
                    // from high to low indexes, i.e. (input_size-batch_size, input_size-1) first
                    ArgsSize remaining_size = input_size;
                    
                    while (remaining_size > 0) {
                        input_size = batch_size > remaining_size ? remaining_size : batch_size;
                        remaining_size -= input_size;
                        
                        // add constant to the last batch
                        WordValue c0b1 = (remaining_size == 0 && constant & 0x1);
                        bool is_2nd_carry = (((input_size > 3) || (input_size == 3 && c0b1)) && (i < WORD_SIZE - 2));
                        const addition_map_entry_t& map = ADD_MAP[input_size][c0b1][is_2nd_carry];
                        
                        const ArgsSize output_size = is_2nd_carry ? 3 : 2;
                        
                        r.new_instance(); // make new result bit
                        variables.push_back(r);
                        carry_out_1.push_back(Ref<CnfEncoderBit>()); // make new carry out 1
                        carry_out_1.back().new_instance();
                        variables.push_back(carry_out_1.back());
                        
                        if (is_2nd_carry) {
                            carry_out_22.push_back(Ref<CnfEncoderBit>()); // make new carry out 2
                            carry_out_22.back().new_instance();
                            variables.push_back(carry_out_22.back());
                        };
                        
                        CnfEncoderBit::record_clauses(map.map, map.map_size, variables.data() + remaining_size, input_size, output_size);
                        
                        if (remaining_size > 0) {
                            // more batches - remove carry out values but keep r as carry in
                            add_append_carry_(r, carry_in);
                            remaining_size++;
                            variables.resize(remaining_size);
                            variables.back() = r;
                        };
                    };
                };
            };
            
            // assign the result bit; at the end as may overwrite one of the operands
            (*result)[i] = r;
            
            // shift carry for the next iteration; move references to keep them
            carry_in.clear();
            carry_in.swap(carry_out_1);
            carry_in.insert(carry_in.end(), carry_out_21.begin(), carry_out_21.end());
            carry_out_21.clear();
            carry_out_21.swap(carry_out_22);
            
            // variables from the current iteration
            variables.clear();
            
            // remaining bits of the constant passed to the next round
            constant >>= 1;
        };
        
        return result;
    };
};

#endif /* cnfwordadd_hpp */
