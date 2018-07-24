//
//  CGen
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include <fstream>
#include <cstring>
#include "commands.hpp"
#include "sha1.hpp"
#include "sha256.hpp"
#include "cnf.hpp"
#include "cnfencoderbit.hpp"
#include "cnfword.hpp"
#include "cnfwordadd.hpp"
#include "cnfoptimizer.hpp"
#include "dimacs.hpp"
#include "anf.hpp"
#include "anfencoderbit.hpp"
#include "anfword.hpp"
#include "anfwordadd.hpp"
#include "polybori.hpp"
#include "variablesrandom.hpp"
#include "formulatracer.hpp"

void load(ple::Cnf& cnf, const char* const file_name) {
    std::cout << "Input file: " << file_name << std::endl;
    
    std::ifstream file(file_name);
    if (file.is_open()) {
        try {
            ple::DimacsStreamReader reader(file);
            reader.read(cnf);
            file.close();
        }
        catch (TextReaderException e) {
            std::cout << "DIMACS Parse Error: " << e << std::endl;
            throw std::invalid_argument("Failed to load the input file");
        }
    }
    else {
        throw std::invalid_argument("Failed to open the input file");
    };
    
    std::cout << "DIMACS size: " << std::dec;
    std::cout << cnf.variables_size() << " variables, ";
    std::cout << cnf.clauses_size() << " clauses" << std::endl;
};

void save_append_parameters(ple::Formula& formula) {
    formula.clear_parameters("application");
    formula.add_parameter("application", "name", APP_TITLE);
    formula.add_parameter("application", "version", APP_VERSION);
    formula.add_parameter("application", "url", APP_URL);
    formula.clear_parameters("writer");
};

template<class Formula, class Writer>
void save_impl(const Formula& formula, const char* const file_name) {
    std::cout << "Output file: " << file_name << std::endl;
    
    std::ofstream file(file_name);
    if (file.is_open()) {
        Writer writer(file);
        writer.write(formula);
        file.close();
    }
    else {
        throw std::invalid_argument(ERROR_FAILED_OPENING_OUTPUT_FILE);
    };
    
    if (formula.is_empty()) {
        std::cout << WARNING_FORMULA_IS_EMPTY << std::endl;
    };
};

void save(ple::Cnf& cnf, const char* const file_name) {
    save_append_parameters(cnf);
    
    if (!cnf.is_empty()) {
        cnf.add_parameter("writer", "is_sorted", 1);
    };
    
    save_impl<ple::Cnf, ple::DimacsSortedStreamWriter>(cnf, file_name);
    
    std::cout << "DIMACS size: " << std::dec;
    std::cout << cnf.variables_size() << " variables, ";
    std::cout << cnf.clauses_size() << " clauses" << std::endl;
};

void save(ple::Anf& anf, const char* const file_name) {
    save_append_parameters(anf);
    
    save_impl<ple::Anf, ple::PolyBoRiStreamWriter>(anf, file_name);
    
    std::cout << "Output size: " << std::dec;
    std::cout << anf.variables_size() << " variables, ";
    std::cout << anf.equations_size() << " equations" << std::endl;
};

void generate_variable_random(const char* const name,
                              const CGenVariableInfo& src, ple::VariablesArray& dst) {
    
    ple::variables_size_t assigned_count = ple::assign_random(dst.data(), dst.size(), src.random_count);
    if (assigned_count != src.random_count) {
        throw std::invalid_argument(ERROR_RANDOM_NOT_ENOUGH_VARIABLES);
    };
    
    std::cout << "Assigned " << assigned_count << " bit(s) randomly in \"" << name << "\"" << std::endl;
};

// merge the supplied value with the named variable template
// if dst/template is a constant, src must be set to the same value
// dst structure (element_size) is disregarded
// src size must be less or equal to dst
void merge_variable_value(const std::string name,
                          const CGenVariableInfo& src, ple::VariablesArray& dst,
                          bool apply_except, const ple::variables_size_t variables_size) {
    const ple::literalid_t literalid_max = variables_size == 0 ?
    ple::LITERALID_MAX : variable_t__literal_id(variables_size - 1);
    const ple::literalid_t* p_src = src.data.data();
    ple::literalid_t* p_dst = dst.data();
    
    if (src.data.size() > dst.size()) {
        throw std::invalid_argument(ERROR_ASSIGN_VARIABLE_VALUE_TOO_LONG);
    };
    
    uint32_t literal_index = 0;
    while (literal_index < src.data.size()) {
        if (literal_t__is_constant(*p_dst) && *p_dst != *p_src) {
            throw std::invalid_argument(ERROR_ASSIGN_VARIABLE_CONSTANT_MISMATCH);
        };
        
        if (*p_src > literalid_max) {
            throw std::invalid_argument(std::string("\"") + name + std::string("\" ") +
                                        ERROR_NAMED_VARIABLE_OUT_OF_RANGE);
        };
        
        if (!apply_except || src.var_range_first == 0 ||
            literal_index + 1 < src.var_range_first ||
            literal_index + 1 > src.var_range_last) {
            *p_dst = *p_src;
        };
        
        p_src++;
        p_dst++;
        literal_index++;
    };
};

template<class SHA, class Formula>
void encode_impl(Formula& formula, const uint32_t rounds,
            const CGenVariablesMap& variables_map,  const bool do_normalize_variables,
            const char* const output_file_name) {

    if (rounds == 0 || rounds > SHA::ROUNDS_NUMBER) {
        throw std::invalid_argument(ERROR_ROUNDS_RANGE);
    };
    
    formula.add_parameter("encoder", "algorithm", SHA::NAME);
    formula.add_parameter("encoder", "rounds", rounds);
    
    const CGenVariableInfo* p_M_value = nullptr;
    const CGenVariableInfo* p_H_value = nullptr;
    
    // validate variables
    for (CGenVariablesMap::const_iterator it = variables_map.begin(); it != variables_map.end(); it++ ) {
        if (it->first == "M") {
            p_M_value = &(it->second);
        } else if (it->first == "H") {
            p_H_value = &(it->second);
        } else {
            throw std::invalid_argument(ERROR_ENCODE_UNKNOWN_VARIABLE_NAME);
        };
    };
    
    SHA sha;
    
    ple::RefArray<typename SHA::Word> M(SHA::MESSAGE_BLOCK_SIZE);
    ple::RefArray<typename SHA::Word> H(SHA::HASH_SIZE);
    
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
    
    formula.variable_generator().generate_unassigned(M_array.data(), M_array.size());
    assign(M, M_array);
    
    ple::FormulaTracer<SHA::WORD_SIZE, typename SHA::Bit> tracer(formula);
    sha.execute(M, H, tracer, rounds);
    
    bool is_valid = true;
    
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
                        merge_variable_value("M", *p_M_value, M_array, false, formula.variables_size());
                        evaluate(formula, "M", M_array, "H", H_array);
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
                merge_variable_value("H", *p_H_value, H_array, true, formula.variables_size());
                break;
        };
        is_valid = assign(formula, "H", H_array);
    };
    
    if (is_valid && do_normalize_variables) {
        normalize_variables(formula);
    };
    
    if (is_valid) {
        save(formula, output_file_name);
    };
};

void encode_anf(const CGenAlgorithm algorithm, const uint32_t rounds,
                const CGenVariablesMap& variables_map, const bool do_normalize_variables,
                const char* const output_file_name) {
    ple::Anf anf;
    ple::Ref<ple::AnfEncoderBit>::allocator().set_anf(&anf);
    
    switch(algorithm) {
        case algSHA1:
            encode_impl<acl::SHA1<ple::AnfEncoderBit>>(anf, rounds,
                                                       variables_map, do_normalize_variables,
                                                       output_file_name);
            break;
        case algSHA256:
            encode_impl<acl::SHA256<ple::AnfEncoderBit>>(anf, rounds,
                                                         variables_map, do_normalize_variables,
                                                         output_file_name);
            break;
        default:
            assert(false);
    };
};

void encode_cnf(const CGenAlgorithm algorithm, const uint32_t rounds,
                const CGenVariablesMap& variables_map, const bool do_normalize_variables,
                const uint32_t add_max_args, const uint32_t xor_max_args,
                const char* const output_file_name) {
    ple::Cnf cnf;
    ple::Ref<ple::CnfEncoderBit>::allocator().set_cnf(&cnf);
    cnf.set_add_max_args(add_max_args);
    cnf.set_xor_max_args(xor_max_args);
    cnf.add_parameter("encoder", "add_max_args", cnf.get_add_max_args());
    cnf.add_parameter("encoder", "xor_max_args", cnf.get_xor_max_args());
    
    switch(algorithm) {
        case algSHA1:
            encode_impl<acl::SHA1<ple::CnfEncoderBit>>(cnf, rounds,
                                                       variables_map, do_normalize_variables,
                                                       output_file_name);
            break;
        case algSHA256:
            encode_impl<acl::SHA256<ple::CnfEncoderBit>>(cnf, rounds,
                                                         variables_map, do_normalize_variables,
                                                         output_file_name);
            break;
        default:
            assert(false);
    };
};

bool assign_require_compute(const CGenVariablesMap& variables_map) {
    for (CGenVariablesMap::const_iterator it = variables_map.begin(); it != variables_map.end(); it++ ) {
        if (it->second.mode == vmCompute) {
            return true;
        };
    };
    return false;
};

const ple::VariablesArray* const get_cnf_named_variable(const ple::Cnf& cnf, const std::string& name) {
    auto vn_it = cnf.get_named_variables().find(name);
    if (vn_it == cnf.get_named_variables().end()) {
        throw std::invalid_argument(std::string("\"") + name + std::string("\" ") +
                                    std::string(ERROR_UNKNOWN_VARIABLE_NAME));
    };
    
    return &(vn_it->second);
};

void assign_merge_variables(const ple::Cnf& cnf, CGenVariablesMap& variables_map,
                            ple::VariablesArray& variables, bool include_compute, bool apply_except) {
    for (CGenVariablesMap::iterator it = variables_map.begin(); it != variables_map.end(); it++ ) {
        if (it->second.mode != vmCompute || include_compute) {
            const ple::VariablesArray* const p_template = get_cnf_named_variable(cnf, it->first);

            // init with template to keep unchanged values unchanged
            ple::VariablesArray variable_value(*p_template);
            
            if (it->second.mode == vmRandom) {
                if (it->second.data.size() == 0) {
                    generate_variable_random(it->first.c_str(), it->second, variable_value);
                    it->second.data = variable_value;
                } else {
                    variable_value = it->second.data;
                };
            } else {
                merge_variable_value(it->first, it->second, variable_value, apply_except, cnf.variables_size());
            };
            
            variables.assign_template_from(*p_template, variable_value);
        };
    };
};

void assign_define_variables(ple::Cnf& cnf, CGenVariablesMap& variables_map) {
    for (CGenVariablesMap::iterator it = variables_map.begin(); it != variables_map.end(); it++ ) {
        if (it->second.mode == vmDefine) {
            auto vn_it = cnf.get_named_variables().find(it->first);
            if (vn_it != cnf.get_named_variables().end()) {
                throw std::invalid_argument(std::string("\"") + it->first + std::string("\" ") +
                                            std::string(ERROR_VARIABLE_ALREADY_DEFINED));
            };
            cnf.add_named_variable(it->first.c_str(), it->second.data);
        };
    };
};

void assign_validate_computed_variables(const ple::Cnf& cnf, CGenVariablesMap& variables_map,
                            ple::VariablesArray& variables) {
    for (CGenVariablesMap::iterator it = variables_map.begin(); it != variables_map.end(); it++ ) {
        if (it->second.mode == vmCompute) {
            const ple::VariablesArray* const p_template = get_cnf_named_variable(cnf, it->first);
            it->second.data = *p_template;
            variables.assign_template_into(*p_template, it->second.data);
            std::cout << "Computed \"" << it->first << "\": " << it->second.data << std::endl;
        };
    };
};

void assign(CGenVariablesMap& variables_map, const bool do_normalize_variables,
            const char* const input_file_name, const char* const output_file_name) {
    if (variables_map.size() == 0) {
        throw std::invalid_argument(ERROR_NO_VARIABLES);
    };
    if (std::strlen(output_file_name) == 0) {
        throw std::invalid_argument(ERROR_MISSING_INPUT_FILE_NAME);
    };
    if (std::strlen(output_file_name) == 0) {
        throw std::invalid_argument(ERROR_MISSING_OUTPUT_FILE_NAME);
    };
    
    ple::Cnf cnf;
    load(cnf, input_file_name);
    
    assign_define_variables(cnf, variables_map);

    bool is_valid = true;
    
    if (cnf.variables_size() > 0) {
        // all variable values will be mapped to a single array
        // conflicts of the constant values if any will be found on the way
        ple::VariablesArray variables(cnf.variables_size());
        variables.assign_sequence();
        
        // a flag to determine the need for the compute route
        if (assign_require_compute(variables_map)) {
            assign_merge_variables(cnf, variables_map, variables, false, false);
            ple::CnfVariableEvaluator(cnf).execute(variables);
            assign_validate_computed_variables(cnf, variables_map, variables);
            // rebuild the variables with computed values
            variables.assign_sequence();
            assign_merge_variables(cnf, variables_map, variables, true, true);
        } else {
            assign_merge_variables(cnf, variables_map, variables, false, true);
        };
        
        is_valid = ple::CnfVariableAssigner(cnf).execute(variables);
    };
    
    if (is_valid && do_normalize_variables) {
        is_valid = normalize_variables(cnf);
    };
    
    if (is_valid) {
        save(cnf, output_file_name);
    };
};
