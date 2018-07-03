//
//  CNFGen
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include <fstream>
#include <random>
#include "commands.hpp"
#include "dimacs.hpp"

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
    std::cout << cnf.clauses_size() << " clauses" << std::endl;;
};

void save(ple::Cnf& cnf, const char* const file_name) {
    cnf.clear_parameters("application");
    cnf.add_parameter("application", "name", APP_TITLE);
    cnf.add_parameter("application", "version", APP_VERSION);
    cnf.add_parameter("application", "url", APP_URL);
    
    cnf.clear_parameters("writer");
    if (!cnf.is_empty()) {
        cnf.add_parameter("writer", "is_sorted", 1);
    };
    
    std::cout << "Output file: " << file_name << std::endl;
    
    std::ofstream file(file_name);
    if (file.is_open()) {
        ple::DimacsSortedStreamWriter writer(file);
        writer.write(cnf);
        file.close();
    }
    else {
        throw std::invalid_argument(ERROR_FAILED_OPENING_OUTPUT_FILE);
    };
    
    std::cout << "DIMACS size: " << std::dec;
    std::cout << cnf.variables_size() << " variables, ";
    std::cout << cnf.clauses_size() << " clauses" << std::endl;
    
    if (cnf.is_empty()) {
        std::cout << WARNING_CNF_IS_EMPTY << std::endl;
    };
};

// set random_count bits of dst to random binary constant values
// variables and unassigned can be set to random values; constants cannot be changed
// assume dst is already pre-set with the template or by other mens
// disregard any dst bits that are constants already
// i.e. exclude those bits from the random_count
void generate_variable_random(const char* const name,
                              const CnfGenVariableInfo& src, ple::VariablesArray& dst) {
    ple::literalid_t* data = dst.data();
    ple::variableid_t size = dst.size();
    
    std::random_device rd;
    std::uniform_int_distribution<ple::variableid_t> index_distribution(0, dst.size() - 1);
    std::uniform_int_distribution<unsigned int> value_distribution(0, 1);
    
    uint32_t assigned_count = 0;
    while (assigned_count < src.random_count) {
        ple::variableid_t random_index = index_distribution(rd);
        unsigned int random_value = value_distribution(rd);
        
        // determine the index to set since the random_index may be a constant already
        // choose the first index greater than the random_index if so;
        // continue from 0 if reach the end of the array
        
        if (literal_t__is_constant(data[random_index])) {
            ple::variableid_t original_index = random_index - 1;
            while (literal_t__is_constant(data[random_index]) && random_index < size) {
                random_index++;
            };
            
            if (random_index == size) {
                random_index = 0;
                while (literal_t__is_constant(data[random_index]) && random_index < original_index) {
                    random_index++;
                };
                
                if (literal_t__is_constant(data[random_index])) {
                    throw std::invalid_argument(ERROR_RANDOM_NOT_ENOUGH_VARIABLES);
                };
            };
        };
        
        data[random_index] = literal_t__constant(random_value);
        assigned_count++;
    };
    
    std::cout << "Assigned " << assigned_count << " bit(s) randomly in \"" << name << "\"" << std::endl;
};

bool assign_require_compute(const CnfGenVariablesMap& variables_map) {
    for (CnfGenVariablesMap::const_iterator it = variables_map.begin(); it != variables_map.end(); it++ ) {
        if (it->second.mode == vmCompute) {
            return true;
        };
    };
    return false;
};

// merge the supplied value with the named variable template
// if dst/template is a constant, src must be set to the same value
// dst structure (element_size) is disregarded
// src size must be less or equal to dst
void merge_variable_value(const std::string name,
                        const CnfGenVariableInfo& src, ple::VariablesArray& dst,
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

const ple::VariablesArray* const get_cnf_named_variable(const ple::Cnf& cnf, const std::string& name) {
    auto vn_it = cnf.named_variables().find(name);
    if (vn_it == cnf.named_variables().end()) {
        throw std::invalid_argument(std::string("\"") + name + std::string("\" ") +
                                    std::string(ERROR_UNKNOWN_VARIABLE_NAME));
    };
    
    return &(vn_it->second);
};

void assign_merge_variables(const ple::Cnf& cnf, CnfGenVariablesMap& variables_map,
                            ple::VariablesArray& variables, bool include_compute, bool apply_except) {
    for (CnfGenVariablesMap::iterator it = variables_map.begin(); it != variables_map.end(); it++ ) {
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

void assign_define_variables(ple::Cnf& cnf, CnfGenVariablesMap& variables_map) {
    for (CnfGenVariablesMap::iterator it = variables_map.begin(); it != variables_map.end(); it++ ) {
        if (it->second.mode == vmDefine) {
            auto vn_it = cnf.named_variables().find(it->first);
            if (vn_it != cnf.named_variables().end()) {
                throw std::invalid_argument(std::string("\"") + it->first + std::string("\" ") +
                                            std::string(ERROR_VARIABLE_ALREADY_DEFINED));
            };
            cnf.add_named_variable(it->first.c_str(), it->second.data);
        };
    };
};

void assign_validate_computed_variables(const ple::Cnf& cnf, CnfGenVariablesMap& variables_map,
                            ple::VariablesArray& variables) {
    for (CnfGenVariablesMap::iterator it = variables_map.begin(); it != variables_map.end(); it++ ) {
        if (it->second.mode == vmCompute) {
            const ple::VariablesArray* const p_template = get_cnf_named_variable(cnf, it->first);
            it->second.data = *p_template;
            variables.assign_template_into(*p_template, it->second.data);
            std::cout << "Computed \"" << it->first << "\": " << it->second.data << std::endl;
        };
    };
};

void assign(CnfGenVariablesMap& variables_map,
            const char* const input_file_name, const char* const output_file_name) {
    if (variables_map.size() == 0) {
        throw std::invalid_argument(ERROR_NO_VARIABLES);
    };
    if (strlen(output_file_name) == 0) {
        throw std::invalid_argument(ERROR_MISSING_INPUT_FILE_NAME);
    };
    if (strlen(output_file_name) == 0) {
        throw std::invalid_argument(ERROR_MISSING_OUTPUT_FILE_NAME);
    };
    
    ple::Cnf cnf;
    load(cnf, input_file_name);
    
    assign_define_variables(cnf, variables_map);

    if (cnf.variables_size() > 0) {
        // all variable values will be mapped to a single array
        // conflicts of the constant values if any will be found on the way
        ple::VariablesArray variables(cnf.variables_size());
        variables.assign_sequence();
        
        // a flag to detrmine the need for the compute route
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
        
        ple::CnfVariableAssigner(cnf).execute(variables);
    };
    
    save(cnf, output_file_name);
};
