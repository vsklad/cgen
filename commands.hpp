//
//  CNFGen
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef commands_hpp
#define commands_hpp

#include "shared.hpp"
#include "cnf.hpp"
#include "cnfencoderbit.hpp"
#include "cnftracer.hpp"
#include "cnfword.hpp"
#include "cnfwordadd.hpp"
#include "cnfoptimizer.hpp"

void load(ple::Cnf& cnf, const char* const file_name);
void save(ple::Cnf& cnf, const char* const file_name);

void generate_variable_random(const char* const name,
                              const CnfGenVariableInfo& src, ple::VariablesArray& dst);
void merge_variable_value(const std::string name,
                          const CnfGenVariableInfo& src, ple::VariablesArray& dst,
                          bool apply_except, const ple::variables_size_t variables_size);

template<class SHA>
void encode(const uint32_t rounds, const CnfGenVariablesMap& variables_map,
            const uint32_t add_max_args, const uint32_t xor_max_args,
            const char* const output_file_name) {
    if (strlen(output_file_name) == 0) {
        throw std::invalid_argument(ERROR_MISSING_OUTPUT_FILE_NAME);
    };
    
    if (rounds == 0 || rounds > SHA::ROUNDS_NUMBER) {
        throw std::invalid_argument(ERROR_ROUNDS_RANGE);
    };
    
    const CnfGenVariableInfo* p_M_value = nullptr;
    const CnfGenVariableInfo* p_H_value = nullptr;
    
    // validate variables
    for (CnfGenVariablesMap::const_iterator it = variables_map.begin(); it != variables_map.end(); it++ ) {
        if (it->first == "M") {
            p_M_value = &(it->second);
        } else if (it->first == "H") {
            p_H_value = &(it->second);
        } else {
            throw std::invalid_argument(ERROR_ENCODE_UNKNOWN_VARIABLE_NAME);
        };
    };
    
    using Word = typename SHA::Word;
    
    ple::Cnf cnf;
    cnf.set_add_max_args(add_max_args);
    cnf.set_xor_max_args(xor_max_args);
    
    ple::Ref<ple::CnfEncoderBit>::allocator().set_cnf(&cnf);
    
    ple::CnfTracer tracer(cnf);
    SHA sha;
    
    ple::RefArray<Word> M(SHA::MESSAGE_BLOCK_SIZE);
    ple::RefArray<Word> H(SHA::HASH_SIZE);
    
    ple::VariablesArray M_array(SHA::MESSAGE_BLOCK_SIZE, SHA::WORD_SIZE);
    M_array.assign_unassigned();
    
    if (p_M_value != nullptr) {
        switch (p_M_value->mode) {
            case vmRandom:
                generate_variable_random("M", *p_M_value, M_array);
                break;
            case vmDefine:
                assert(false);
                break;
            case vmCompute:
                // same as unspecified...
                break;
            case vmValue:
                merge_variable_value("M", *p_M_value, M_array, true, 0);
                break;
        };
    };
    
    cnf.variable_generator().generate_unassigned(M_array.data(), M_array.size());
    assign(M, M_array);
    
    sha.execute(M, H, tracer, rounds);
    
    if (p_H_value != nullptr) {
        ple::VariablesArray H_array(SHA::HASH_SIZE, SHA::WORD_SIZE);
        Words2VariablesArray(H, H_array);
        
        switch (p_H_value->mode) {
            case vmRandom:
                generate_variable_random("H", *p_H_value, H_array);
                break;
            case vmCompute:
                if (p_M_value != nullptr) {
                    if (p_M_value->mode == vmValue &&
                        p_M_value->var_range_first > 0) {
                        // some constant bits were excluded for encoding
                        // evaluate hash with the message again with all bits
                        merge_variable_value("M", *p_M_value, M_array, false, cnf.variables_size());
                        ple::CnfVariableEvaluator(cnf).execute("M", p_M_value->data, "H", H_array);
                        std::cout << "Computed \"H\": " << H_array << std::endl;
                    } else if (p_M_value->mode == vmValue &&
                               p_M_value->var_range_first == 0) {
                        // pre-set constant message
                        // ignore, the hash is already computed and assigned
                    } else if (p_M_value->mode == vmRandom &&
                               p_M_value->random_count == SHA::MESSAGE_BLOCK_SIZE * SHA::WORD_SIZE) {
                        // fully random constant message
                        // ignore, the hash is already computed and assigned
                    } else {
                        throw std::invalid_argument(ERROR_COMPUTE_HASH_NO_MESSAGE);
                    }
                } else {
                    throw std::invalid_argument(ERROR_COMPUTE_HASH_NO_MESSAGE);
                };
                break;
            case vmDefine:
            case vmValue:
                merge_variable_value("H", *p_H_value, H_array, true, cnf.variables_size());
                break;
        };
        ple::CnfVariableAssigner(cnf).execute("H", H_array);
    };
    
    cnf.add_parameter("encoder", "rounds", rounds);
    cnf.add_parameter("encoder", "add_max_args", cnf.get_add_max_args());
    cnf.add_parameter("encoder", "xor_max_args", cnf.get_xor_max_args());
    
    save(cnf, output_file_name);
};

void assign(CnfGenVariablesMap& variables_map,
            const char* const input_file_name, const char* const output_file_name);

#endif /* commands_hpp */
