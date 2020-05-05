//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef literaladd_hpp
#define literaladd_hpp

#include <vector>
#include "variables.hpp"
#include "gf2n.hpp"

namespace bal {
  
    // add carry to the end of the list
    // carry_size is a number of variables at the end of the list that are carry
    inline void add_append_carry_(const literalid_t arg, std::vector<literalid_t>& variables) {
        if (!literal_t__is_constant_0(arg)) {
            variables.push_back(arg);
        };
    };
    
    // process a carry or an operand
    // either add to variables array or increase the constant
    inline void add_append_variable_(const literalid_t arg,
                                     std::vector<literalid_t>& variables,
                                     std::vector<literalid_t>& carry_out_1,
                                     unsigned& constant) {
        if (literal_t__is_constant(arg)) {
            constant += literal_t__is_constant_1(arg) ? 1 : 0;
        } else {
            // check for duplicates including negation
            for (auto i = 0; i < variables.size(); i++) {
                if (literal_t__is_same_variable(arg, variables[i])) {
                    if (literal_t__is_negation_of(arg, variables[i])) {
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
    
    inline void add_count_variable_(const literalid_t arg, std::vector<literalid_t>& variables) {
        if (!literal_t__is_constant(arg)) {
            // check for duplicates including negation
            for (auto i = 0; i < variables.size(); i++) {
                if (literal_t__is_same_variable(arg, variables[i])) {
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
    
    // __ADD_NETWORK_REDUCE_DIAMETER
    // when generating adders, connect result of the one being encoded
    // to the last adder in the list rather than the next one;
    // if undefined, connect as an input to the next adder;
    // should be equivalent to reducing diameter of the circuit graph
    // empirically, makes solvers slower; to investigate further
    //#define __ADD_NETWORK_REDUCE_DIAMETER
    
    template<std::size_t N, class GF2E, void (encode_method)(typename GF2E::Formula& formula, literalid_t args[],
                                                             const std::size_t input_size,
                                                             const std::size_t output_size,
                                                             const unsigned constant, const bool b_2ndc1)>
    inline void literal_word_add(GF2NElement<N, GF2E>* result,
                                 const GF2NElement<N, GF2E>* const args[], const std::size_t args_size) {
        // formula instance to generate the clauses for
        // determined from the first variable argument and must be the same for all
        typename GF2E::Formula* p_formula = nullptr;
        
        // there is a constant carry and variable carry, for each carry bit
        // constant is a mix of constant operand values and constant carry
        unsigned constant = 0;
        // lists to keep carry
        std::vector<literalid_t> carry_in;
        std::vector<literalid_t> carry_out_1;
        std::vector<literalid_t> carry_out_21;
        std::vector<literalid_t> carry_out_22;
        
        // variables that are added
        std::vector<literalid_t> variables;
        
        for (auto i = 0; i < N; i++) {
            // determine number of variables
            // take carry out from the previous bit
            for (auto j = 0; j < carry_in.size(); j++) {
                add_append_variable_(carry_in[j], variables, carry_out_1, constant);
            };
            // filter out variables from args
            for (auto j = 0; j < args_size; j++) {
                const literalid_t args_ji = (*args[j])[i]->value;
                if (literal_t__is_variable(args_ji)) {
                    if (p_formula == nullptr) {
                        p_formula = (*args[j])[i]->formula;
                    } else {
                        _assert_level_1((*args[j])[i]->formula == p_formula);
                    };
                };
                
                add_append_variable_(args_ji, variables, carry_out_1, constant);
            };
            
            std::size_t input_size = variables.size();
            
            if ((i == N - 1) || input_size < 2) {
                // no need for carry for the last bit, only assign result bit
                // also, eor takes care of zero or single variable arguments;
                // make sure the args are sorted however
#if defined(XOR_ARGS_ORDER_ASCENDING)
                std::sort(variables.begin(), variables.end());
#elif defined(XOR_ARGS_ORDER_DESCENDING)
                std::sort(variables.begin(), variables.end(), [](const literalid_t a, const literalid_t b) { return a > b; });
#endif
                (*result)[i] = GF2E::from_literal(p_formula, eor(p_formula, variables.data(), input_size, constant & 0b1));
                
                if ((i < N - 1) && (input_size == 1) && (constant & 0b1)) {
                    add_append_carry_(variables[0], carry_out_1);
                };
            } else {
                // for SHA1/SHA256, 6 variables is the max possible
                // addition of 4 variables + 1 constant
                // i.e. 4 input + 2 carry in
                // for other purposes, ADD_MAP may be extended
                
                // determine number of variable to add at once
                // there are at least 2 variables
                // formula must always be assigned for a variable
                _assert_level_1(p_formula != nullptr);
                const std::size_t batch_size = p_formula->get_add_max_args();
#ifdef __ADD_NETWORK_REDUCE_DIAMETER
                // now take batches of variables and add them up
                // from low to high indexes, result is appended to the end
                std::size_t current_index = 0;
                
                while (current_index <  input_size) {
                    const std::size_t current_batch_size = (input_size - current_index) > batch_size ? batch_size : input_size - current_index;
                    
                    // add constant to the last batch
                    WordValue c0b1 = ((current_index + current_batch_size == input_size) && (constant & 0x1));
                    bool is_2nd_carry = (((current_batch_size > 3) || (current_batch_size == 3 && c0b1)) && (i < N - 2));
                    const addition_map_entry_t& map = ADD_MAP[current_batch_size][c0b1][is_2nd_carry];
                    
                    const std::size_t output_size = is_2nd_carry ? 3 : 2;
                    
                    std::vector<BIT*> input;
                    for (auto i = 0; i < current_batch_size; i++) {
                        input.push_back(variables[current_index + i]);
                    };
                    
                    r.new_instance(); // make new result bit
                    input.push_back(r);
                    carry_out_1.push_back(Ref<BIT>()); // make new carry out 1
                    carry_out_1.back().new_instance();
                    input.push_back(carry_out_1.back());
                    
                    if (is_2nd_carry) {
                        carry_out_22.push_back(Ref<BIT>()); // make new carry out 2
                        carry_out_22.back().new_instance();
                        input.push_back(carry_out_22.back());
                    };
                    
                    BIT::record_clauses(map.map, map.map_size, input.data(), current_batch_size, output_size);

                    if (current_index + current_batch_size < input_size) {
                        // more batches - append r to input variables list
                        add_append_carry_(r, carry_in); // to keep reference
                        variables.push_back(r);
                        input_size++;
                    };
                    
                    current_index += current_batch_size;
                };
#else
                // now take batches of variables and add them up
                // from high to low indexes, i.e. (input_size-batch_size, input_size-1) first
                std::size_t remaining_size = input_size;
                
                while (remaining_size > 0) {
                    input_size = batch_size > remaining_size ? remaining_size : batch_size;
                    remaining_size -= input_size;
                    
                    // add constant to the last batch
                    unsigned constant_bit = (remaining_size == 0 && constant & 0x1);
                    bool b_c2 = (((input_size > 3) || (input_size == 3 && constant_bit)) && (i < N - 2));
                    bool b_2nd_c1 = false;

                    const std::size_t output_size = b_c2 ? 3 : 2;
                    
                    variables.push_back(p_formula->new_variable_literal()); // make the result bit
                    variables.push_back(p_formula->new_variable_literal()); // mace the c1 bit
                    
                    if (b_c2) {
                        variables.push_back(p_formula->new_variable_literal()); // make the c2 bit
                    };
                    
                    literalid_t* const p_args = variables.data() + remaining_size;
                    literalid_t* const p_output = p_args + input_size;
                    
                    encode_method(*p_formula, p_args, input_size, output_size, constant_bit, b_2nd_c1);
                    
                    variables[remaining_size] = p_output[0];
                    carry_out_1.push_back(p_output[1]);
                    if (b_c2) {
                        carry_out_22.push_back(p_output[2]);
                    };
                    
                    if (remaining_size > 0) {
                        remaining_size++; // +1 since the first element is the adder's result
                        variables.resize(remaining_size);
                    };
                };
#endif
                // assign the result bit
                // this will be the first variables element remaining
                (*result)[i] = new GF2E(p_formula, variables[0]);
            };
            
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
    };
};

#endif /* literaladd_hpp */
