//
//  CGen
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <cstring>
#include <random>
#include <vector>
#include "sha1.hpp"
#include "sha256.hpp"
#include "literal.hpp"
#include "cnf.hpp"
#include "cnfencoding.hpp"
#include "cnfoptimizer.hpp"
#include "cnfdimacs.hpp"
#include "cnfgexf.hpp"
#include "cnfgraphml.hpp"
#include "anf.hpp"
#include "anfencoding.hpp"
#include "anfpolybori.hpp"
#include "formulatracer.hpp"
#include "commands.hpp"

#ifdef CNF_TRACE
#include "cnffiletracer.hpp"
#include "cnfgexftracer.hpp"
#include "cnfnativetracer.hpp"
#endif

bal::Ref<acl::SHA1<bal::Literal<bal::Cnf>>::Word> x[2] = { 0x00000000, 0x11111111 };

void print_statistics(const bal::Cnf& cnf) {
    if (cnf.clauses_size() == 0) {
        std::cout << MSG_FORMULA_IS_SATISFIABLE;
        if (cnf.variables_size() > 0) {
            std::cout << " with " << std::dec << cnf.variables_size() << " free variables";
        } else {
            std::cout << " with 1 solution";
        };
        std::cout << std::endl;
    } else {
        std::cout << "CNF: " << std::dec;
        std::cout << cnf.variables_size() << " var";
        std::cout << ", " << cnf.clauses_size() << "/" << cnf.clauses_size<0, true>() << "/" << cnf.clauses_size<2>() << " cls/agr/bin";
        std::cout << ", " << cnf.clauses_size<0, false, true>() << " lit";
        std::cout << ", " << (cnf.memory_size() >> 10) << " Kb" << std::endl;
    };
};

void print_statistics(const bal::Anf& anf) {
    if (anf.equations_size() == 0) {
        std::cout << MSG_FORMULA_IS_SATISFIABLE << std::endl;
    } else {
        std::cout << "ANF: " << std::dec;
        std::cout << anf.variables_size() << " variables, ";
        std::cout << anf.equations_size() << " equations" << std::endl;
    };
};

template<class Formula, class Reader>
void load_impl(Formula& formula, const char* const file_name) {
    std::cout << "Input file: " << file_name << std::endl;
    
    if (std::strlen(file_name) == 0) {
        throw std::invalid_argument(ERROR_MISSING_INPUT_FILE_NAME);
    };
    
    std::ifstream file(file_name);
    if (file.is_open()) {
        try {
            Reader reader(file);
            reader.read(formula);
            file.close();
        }
        catch (TextReaderException e) {
            std::cout << "Parse error: " << e << std::endl;
            throw std::invalid_argument("Failed to load the input file");
        }
    }
    else {
        throw std::invalid_argument("Failed to open the input file");
    };
    print_statistics(formula);
};

void save_append_parameters(bal::Formula& formula) {
    formula.clear_parameters("application");
    formula.add_parameter("application", "name", APP_TITLE);
    formula.add_parameter("application", "version", APP_VERSION);
    formula.add_parameter("application", "url", APP_URL);
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
    
    print_statistics(formula);
};

void save(bal::Cnf& cnf, const char* const file_name, const CGenOutputFormat output_format) {
    save_append_parameters(cnf);
    switch (output_format) {
        case ofCnfDimacs:
            save_impl<bal::Cnf, bal::DimacsStreamWriter>(cnf, file_name);
            break;
        case ofCnfVIGGraphML:
            save_impl<bal::Cnf, bal::CnfGraphMLStreamWriter<false>>(cnf, file_name);
            break;
        case ofCnfWeightedVIGGraphML:
            save_impl<bal::Cnf, bal::CnfGraphMLStreamWriter<true>>(cnf, file_name);
            break;
        case ofCnfVIGGEXF:
            save_impl<bal::Cnf, bal::CnfGexfStreamWriter<false, false>>(cnf, file_name);
            break;
        default:
            throw std::invalid_argument(ERROR_OUTPUT_FORMAT_UNSUPPORTED);
    };
};

void save(bal::Anf& anf, const char* const file_name, const CGenOutputFormat output_format) {
    save_append_parameters(anf);
    switch (output_format) {
        case ofAnfPolybori:
            save_impl<bal::Anf, bal::PolyBoRiStreamWriter>(anf, file_name);
            break;
        default:
            throw std::invalid_argument(ERROR_OUTPUT_FORMAT_UNSUPPORTED);
    };
};

const bal::VariablesArray* const get_formula_named_variable(const bal::Formula& formula, const std::string& name) {
    auto vn_it = formula.get_named_variables().find(name);
    if (vn_it == formula.get_named_variables().end()) {
        throw std::invalid_argument(std::string("\"") + name + std::string("\" ") +
                                    std::string(ERROR_UNKNOWN_VARIABLE_NAME));
    };
    
    return &(vn_it->second);
};

// generate random binary values for all non-constant bits of the definition template
// ignore except options
// store the resulting value in the variable info structure for subsequent use
void variable_define_random(CGenVariableInfo& value, const bal::VariablesArray& definition) {
    _assert_level_0(value.mode == vmRandom);
    
    std::random_device rd;
    std::uniform_int_distribution<unsigned int> value_distribution(0, 1);
    
    value.data = definition;
    bal::literalid_t* const data = value.data.data();
    
    for (auto i = 0; i < value.data.size(); i++) {
        if (!literal_t__is_constant(data[i])) {
            data[i] = literal_t__constant(value_distribution(rd));
        };
    };
    
    // the value has been generated now
    // change the entry type to avoid generating a different value again
    value.mode = vmValue;
};

const bal::VariablesArray variable_get_template(const bal::Formula& formula, const char* const variable_name) {
    if (is_binary_variable_name(variable_name)) {
        const bal::literalid_t literal_id = bal::literal_t__from_cstr(variable_name);
        if (literal_t__variable_id(literal_id) >= formula.variables_size()) {
            throw std::invalid_argument("Variable number out of bounds: " + std::string(variable_name));
        };
        bal::VariablesArray result(1, 1);
        *result.data() = literal_id;
        return result;
    } else {
        return *get_formula_named_variable(formula, variable_name);
    };
};

// define new variables and replace definitions within CNF if necessary
// check validity of the variable definition
// generate random constant values
void variables_define(bal::Formula& formula, CGenVariablesMap& variables_map) {
    for (auto it = variables_map.begin(); it != variables_map.end(); it++ ) {
        if (is_binary_variable_name(it->first)) {
            if (it->second.mode == vmRandom && it->second.data.size() == 0) {
                variable_define_random(it->second, variable_get_template(formula, it->first.c_str()));
            };
        } else {
            auto vn_it = formula.get_named_variables().find(it->first);
            if (vn_it == formula.get_named_variables().end() || it->second.replace_existing) {
                if (it->second.mode != vmValue) {
                    throw std::invalid_argument(std::string("\"") + it->first + std::string("\" ") +
                                                std::string(ERROR_VARIABLE_DEFINITION_MODE));
                };
                if (it->second.except_count > 0 || it->second.except_range_size > 0) {
                    throw std::invalid_argument(std::string("\"") + it->first + std::string("\" ") +
                                                std::string(ERROR_VARIABLE_DEFINITION_EXCEPT));
                };
                formula.add_named_variable(it->first.c_str(), it->second.data);
            } else if (it->second.mode == vmRandom && it->second.data.size() == 0) {
                variable_define_random(it->second, vn_it->second);
            };
        };
    };
};

// check constant assignments against the formula
// if variable is assigned within the formula already
// then ignore the new assignment and reset the value accordingly
void variables_reset_assigned_values(const bal::Formula& formula,
                                     const bal::VariablesArray& variable_template,
                                     bal::VariablesArray& variable_value) {
    const bal::literalid_t* const template_data = variable_template.data();
    bal::literalid_t* const value_data = variable_value.data();
    
    for (auto i = 0; i < variable_value.size(); i++) {
        if (literal_t__is_variable(template_data[i]) && literal_t__is_constant(value_data[i])) {
            const bal::literalid_t value = formula.get_variable_value(literal_t__variable_id(template_data[i]));
            if (value == literal_t__substitute_literal(template_data[i], value_data[i])) {
                // reset binary variable value since it is already assigned within the formula
                value_data[i] = template_data[i];
            };
        };
    };
};

// merge the supplied value with the named variable template (dst)
//   assigning binary constant to a non-constant literal is fine
//   assigning UNASSIGNED to a literal does not change it
//   otherwise src and dst elements must match
// remaining dst variables are made UNASSIGNED
//   as it is expected the value will only be used in conjunction with its template
// dst structure (element_size) is disregarded
// src size must be less or equal to dst
// apply src "except" parameters - this should only be done once because of randomisation
// as non-constant literals are never assigned from src, there is no need to check them
bal::VariablesArray variable_generate_value(const bal::Formula& formula,
                             const std::string name,
                             const CGenVariableInfo& src,
                             const bal::VariablesArray& dst_template,
                             const bool apply_except,
                             const bool reassign_in_formula) {
    bal::VariablesArray dst(dst_template.size() / dst_template.element_size(), dst_template.element_size());
    dst.assign_unassigned();
    
    // ensure that the value is provided/generated/computed
    if (src.data.size() != dst.size()) {
        std::stringstream error_message;
        error_message << "Invalid value for " << name;
        error_message << std::dec << "[" << dst.size() / dst.element_size() << "][" << dst.element_size() << "]";
        error_message << " due to wrong dimensions ";
        error_message << std::dec << "[" << src.data.size() / src.data.element_size() << "][" << src.data.element_size() << "]";
        throw std::invalid_argument(error_message.str());
    };
    
    _assert_level_0(src.data.size() > 0);
    bal::variables_size_t dst_changes_count = 0;
    
    const bal::literalid_t* p_src = src.data.data();
    const bal::literalid_t* p_tmp = dst_template.data();
    bal::literalid_t* p_dst = dst.data();
    
    if ((src.except_count > 0 || src.except_range_size > 0) && apply_except) {
        // generate list of element indexes to assign
        std::vector<unsigned int> assignable_indexes;

        // assignable are non-constant dst elements with corresponding constants in src
        // except for the exclusion range
        for (auto i = 0; i < src.data.size(); i++) {
            if ((src.except_range_size == 0 ||
                 i < src.except_range_first || i >= src.except_range_first + src.except_range_size)) {
                if ((literal_t__is_constant(p_src[i]) && literal_t__is_variable(p_tmp[i]) && reassign_in_formula) ||
                    (!literal_t__is_unassigned(p_src[i]) && literal_t__is_unassigned(p_tmp[i]))) {
                    assignable_indexes.push_back(i);
                } else if (literal_t__is_constant(p_src[i]) && literal_t__is_variable(p_tmp[i])) {
                    const bal::literalid_t formula_value = formula.get_variable_value(literal_t__variable_id(p_tmp[i]));
                    if (p_src[i] != literal_t__substitute_literal(p_tmp[i], formula_value)) {
                        assignable_indexes.push_back(i);
                    };
                } else if (!literal_t__is_unassigned(p_src[i]) && p_tmp[i] != p_src[i]) {
                    assignable_indexes.push_back(i);
                };
            };
        };
        
        if (src.except_count > 0) {
            if (assignable_indexes.size() > src.except_count) {
                // pick except_count elements randomly and remove them
                std::random_device rd;
                for (auto i = 0; i < src.except_count; i++) {
                    std::uniform_int_distribution<size_t> index_distribution(0, assignable_indexes.size() - i - 1);
                    size_t random_index = index_distribution(rd);
                    assignable_indexes[random_index] = assignable_indexes[assignable_indexes.size() - i - 1];
                };
                assignable_indexes.resize(assignable_indexes.size() - src.except_count);
            } else if (assignable_indexes.size() == src.except_count) {
                assignable_indexes.clear();
            } else {
                throw std::invalid_argument(ERROR_EXCEPT_NO_VARIABLES);
            };
        };
        
        if (assignable_indexes.size() > 0) {
            for (auto index: assignable_indexes) {
                _assert_level_0(literal_t__is_constant(p_src[index]) && !literal_t__is_constant(p_tmp[index]));
                p_dst[index] = p_src[index];
            };
            dst_changes_count = (bal::variables_size_t)assignable_indexes.size();
        };
    } else {
        // no except, simply copy the values, ignore unassigned src
        for (auto i = 0; i < src.data.size(); i++) {
            if ((literal_t__is_constant(p_src[i]) && literal_t__is_variable(p_tmp[i]) && reassign_in_formula) ||
                (!literal_t__is_unassigned(p_src[i]) && literal_t__is_unassigned(p_tmp[i]))) {
                p_dst[i] = p_src[i];
                dst_changes_count++;
            } else if (literal_t__is_constant(p_src[i]) && literal_t__is_variable(p_tmp[i])) {
                const bal::literalid_t formula_value = formula.get_variable_value(literal_t__variable_id(p_tmp[i]));
                if (p_src[i] != literal_t__substitute_literal(p_tmp[i], formula_value)) {
                    p_dst[i] = p_src[i];
                    dst_changes_count++;
                };
            } else if (!literal_t__is_unassigned(p_src[i]) && p_tmp[i] != p_src[i]) {
                p_dst[i] = p_src[i];
                dst_changes_count++;
            };
        };
    };
    
    if (dst_changes_count > 0) {
        std::cout << "Assigned " << std::dec << dst_changes_count << " bit(s) in \"" << name << "\"" << std::endl;
        std::cout << name << " = " << dst << std::endl;
    };
    
    return dst;
};

bool variables_require_computing(const CGenVariablesMap& variables_map) {
    for (auto it = variables_map.begin(); it != variables_map.end(); it++ ) {
        if (it->second.mode == vmCompute) {
            return true;
        };
    };
    return false;
};

// merge named variable values into a single array for computing
// ignore except options and computed values
// returns number of changed binary variables
bal::variables_size_t variables_merge(bal::VariablesArray& variables,
                                      bal::Formula& formula,
                                      const CGenVariablesMap& variables_map,
                                      const bool apply_except,
                                      const bool reassign_in_formula) {
    assert(formula.variables_size() == variables.size());
    bal::variables_size_t changes_count = 0;
    for (auto it = variables_map.begin(); it != variables_map.end(); it++ ) {
        // other types must have been resolved into values at this point
        _assert_level_1(it->second.mode == vmValue || it->second.mode == vmCompute);
        if (it->second.mode == vmValue) {
            const bal::VariablesArray variable_template = variable_get_template(formula, it->first.c_str());
            bal::VariablesArray variable_value = variable_generate_value(formula, it->first, it->second,
                                                                         variable_template, apply_except,
                                                                         reassign_in_formula);
            bal::variables_size_t assignment_result = variables.assign_template_from(variable_template, variable_value);
            if (assignment_result != bal::VARIABLEID_ERROR) {
                changes_count += assignment_result;
            } else {
                throw std::invalid_argument("Conflicting binary variable assignment");
            };
            
            if (apply_except && !is_binary_variable_name(it->first)) {
                // update any unassigned variables within template
                // with values that has been specified or generated
                formula.named_variable_update_unassigned(it->first, variable_value);
            };
        };
    };
    return changes_count;
};

// store computed values into variables_map for subsequent use
// ignore except options
// store constants and changed variable values only
void variables_store_computed(const bal::Formula& formula,
                              const bal::VariablesArray& variables,
                              CGenVariablesMap& variables_map) {
    assert(formula.variables_size() == variables.size());
    for (auto it = variables_map.begin(); it != variables_map.end(); it++ ) {
        if (it->second.mode == vmCompute) {
            const bal::VariablesArray template_ = variable_get_template(formula, it->first.c_str());
            it->second.data.assign_from_template(variables, template_,
                                                 it->second.compute_mode == vcmComplete ? bal::VariablesArray::aftmComplete :
                                                 it->second.compute_mode == vcmConstant ? bal::VariablesArray::aftmConstant :
                                                 bal::VariablesArray::aftmDifference);
            
            std::cout << it->first << " = " << it->second.data << std::endl;
            it->second.mode = vmValue; // the value has now been computed
        };
    };
};

// variables_map - contains variable values without except options applied
template<class FORMULA, bool ONLY_IF_CHANGED = false>
bool process_impl(FORMULA& formula, CGenVariablesMap& variables_map,
                  const bool b_reindex_variables, const bal::FormulaProcessingMode mode) {
    static_assert(std::is_base_of<bal::Formula, FORMULA>::value, "FORMULA must be a descendant of bal::Formula");
    
    // all variable values will be mapped to a single array
    // conflicts of the constant values if any will be found on the way
    bal::VariablesArray variables(formula.variables_size(), 1);
    
    // a flag to determine the need for the compute route
    if (variables_require_computing(variables_map)) {
        std::cout << "Computing variables" << std::endl;
        variables.assign_sequence();
        variables_merge(variables, formula, variables_map, false, true);
        if (!evaluate(formula, variables)) {
            throw std::invalid_argument(ERROR_COMPUTE_INVALID_ENCODING);
        };
        variables_store_computed(formula, variables, variables_map);
    };
    
    // rebuild the variables with computed values if exist
    // all random values generated and all except options applied
    variables.assign_sequence();
    const bal::variables_size_t changes_count = variables_merge(variables, formula, variables_map,
                                                                true, mode != bal::fpmUnoptimized);
    if (!ONLY_IF_CHANGED || changes_count > 0) {
        if (changes_count > 0) {
            std::cout << "Assigning " << std::dec << changes_count << " variable(s) in the formula" << std::endl;
        };
        return process(formula, variables, b_reindex_variables, mode);
    } else {
        return true;
    };
};

template<class SHA>
void encode_impl(typename SHA::Bit::Formula& formula, const uint32_t rounds,
                 CGenVariablesMap& variables_map,
                 const uint32_t add_max_args, const uint32_t xor_max_args,
                 const char* const output_file_name, const CGenOutputFormat output_format,
                 const bool b_reindex_variables, const bool b_normalize_variables,
                 const bool b_assign_after_encoding, const bal::FormulaProcessingMode mode) {

    if (rounds == 0 || rounds > SHA::ROUNDS_NUMBER) {
        throw std::invalid_argument(ERROR_ROUNDS_RANGE);
    };
    
    if (add_max_args > 0) {
        formula.set_add_max_args(add_max_args);
    };
    if (xor_max_args > 0) {
        formula.set_xor_max_args(xor_max_args);
    };
    formula.add_parameter("encoder", "add_max_args", formula.get_add_max_args());
    formula.add_parameter("encoder", "xor_max_args", formula.get_xor_max_args());
    
    formula.add_parameter("encoder", "xor_args_structure", "chain"); // "pyramid"
#if defined(XOR_ARGS_ORDER_ASCENDING)
    formula.add_parameter("encoder", "xor_args_order", "ascending");
#elif defined(XOR_ARGS_ORDER_DESCENDING)
    formula.add_parameter("encoder", "xor_args_order", "descending");
#else
    formula.add_parameter("encoder", "xor_args_order", "none");
#endif
    
    formula.add_parameter("encoder", "algorithm", SHA::NAME);
    formula.add_parameter("encoder", "rounds", rounds);
 
    bal::VariablesArray M_array(SHA::MESSAGE_BLOCK_SIZE, SHA::WORD_SIZE);
    M_array.assign_unassigned();
    
    // handling of message M is special because it is used to optimise the encoding itself
    auto M_it = variables_map.find("M");
    if (!b_assign_after_encoding && M_it != variables_map.end()) {
        if (M_it->second.mode == vmCompute) {
            throw std::invalid_argument(ERROR_COMPUTE_MESSAGE_NOT_SUPPORTED);
        } else if (M_it->second.mode == vmRandom) {
            variable_define_random(M_it->second, M_array);
        };
        M_array = variable_generate_value(formula, "M", M_it->second, M_array, true, true);
    };
    
    SHA sha;
    
    bal::Ref<typename SHA::Word> M[SHA::MESSAGE_BLOCK_SIZE];
    bal::Ref<typename SHA::Word> H[SHA::HASH_SIZE];
    
    formula.generate_unassigned_variable_literals(M_array.data(), M_array.size());
    assign(M, formula, M_array);
    
    bal::FormulaTracer<SHA::WORD_SIZE, typename SHA::Bit> tracer(formula);
    sha.execute(M, H, tracer, rounds);
    
    bool is_valid = true;
    
    if (variables_map.size() > 0) {
        // take the same route as process function
        print_statistics(formula);
        variables_define(formula, variables_map);
        is_valid = process_impl<typename SHA::Bit::Formula, true>(formula, variables_map, b_reindex_variables, mode);
    };
    
    if (is_valid && b_normalize_variables) {
        normalize_variables(formula, b_reindex_variables);
    };
    
    if (is_valid) {
        save(formula, output_file_name, output_format);
    } else {
        throw std::invalid_argument("Encoding failed");
    };
};

void encode_anf(const CGenAlgorithm algorithm, const uint32_t rounds,
                CGenVariablesMap& variables_map,
                const uint32_t add_max_args, const uint32_t xor_max_args,
                const char* const output_file_name,
                const CGenOutputFormat output_format, const CGenTraceFormat trace_format,
                const bool b_reindex_variables, const bool b_normalize_variables,
                const bool b_assign_after_encoding, const bal::FormulaProcessingMode mode) {    
    bal::Anf anf;
    
    switch(algorithm) {
        case algSHA1:
            encode_impl<acl::SHA1<bal::Literal<bal::Anf>>>(anf, rounds, variables_map, add_max_args, xor_max_args,
                                                       output_file_name, output_format,
                                                       b_reindex_variables, b_normalize_variables,
                                                       b_assign_after_encoding, mode);
            break;
        case algSHA256:
            encode_impl<acl::SHA256<bal::Literal<bal::Anf>>>(anf, rounds, variables_map, add_max_args, xor_max_args,
                                                         output_file_name, output_format,
                                                         b_reindex_variables, b_normalize_variables,
                                                         b_assign_after_encoding, mode);
            break;
        default:
            assert(false);
    };
};

#ifdef CNF_TRACE
#define __CNF_TRACE_INITIALIZE(trace_format, output_file_name) trace_setup(trace_format, output_file_name)
#define __CNF_TRACE_FINIALIZE bal::set_cnf_tracer(nullptr)

void trace_setup(const CGenTraceFormat trace_format, const char* const output_file_name) {
    switch(trace_format) {
        case tfNone:
            bal::set_cnf_tracer(nullptr);
            break;
        case tfNativeStdOut:
            std::cout << "Trace output: std::cout" << std::endl;
            bal::set_cnf_tracer(new bal::CnfNativeTracer(std::cout));
            break;
        case tfNativeFile: {
            const std::string trace_file_name = std::string(output_file_name) + ".trace";
            std::cout << "Trace file: " << trace_file_name << std::endl;
            bal::set_cnf_tracer(new bal::CnfFileTracer<bal::CnfNativeTracer>(trace_file_name.c_str()));
            break;
        };
        case tfCnfVIGGEXF: {
            const std::string trace_file_name = std::string(output_file_name) + ".trace.gexf";
            std::cout << "Trace file: " << trace_file_name << std::endl;
            bal::set_cnf_tracer(new bal::CnfFileTracer<bal::CnfGexfTracer>(trace_file_name.c_str()));
            break;
        };
    };
};

#else
#define __CNF_TRACE_INITIALIZE(trace_format, output_file_name)
#define __CNF_TRACE_FINIALIZE
#endif

void encode_cnf(const CGenAlgorithm algorithm, const uint32_t rounds,
                CGenVariablesMap& variables_map,
                const uint32_t add_max_args, const uint32_t xor_max_args,
                const char* const output_file_name,
                const CGenOutputFormat output_format, const CGenTraceFormat trace_format,
                const bool b_reindex_variables, const bool b_normalize_variables,
                const bool b_assign_after_encoding, const bal::FormulaProcessingMode mode) {
    bal::Cnf cnf;
    
    cnf.add_parameter("encoder", "add_args_structure", "chain");
    cnf.add_parameter("encoder", "add_args_order", "none");
    
    __CNF_TRACE_INITIALIZE(trace_format, output_file_name);
    switch(algorithm) {
        case algSHA1:
            encode_impl<acl::SHA1<bal::Literal<bal::Cnf>>>(cnf, rounds, variables_map, add_max_args, xor_max_args,
                                                       output_file_name, output_format,
                                                       b_reindex_variables, b_normalize_variables,
                                                       b_assign_after_encoding, mode);
            break;
        case algSHA256:
            encode_impl<acl::SHA256<bal::Literal<bal::Cnf>>>(cnf, rounds, variables_map, add_max_args, xor_max_args,
                                                         output_file_name, output_format,
                                                         b_reindex_variables, b_normalize_variables,
                                                         b_assign_after_encoding, mode);
            break;
        default:
            assert(false);
    };
    __CNF_TRACE_FINIALIZE;
};

void process_anf(CGenVariablesMap& variables_map,
             const char* const input_file_name, const char* const output_file_name,
             const CGenOutputFormat output_format, const CGenTraceFormat trace_format,
             const bool b_reindex_variables, const bool b_normalize_variables,
             const bal::FormulaProcessingMode mode) {
    bal::Anf anf;
    load_impl<bal::Anf, bal::PolyBoRiStreamReader>(anf, input_file_name);
    variables_define(anf, variables_map);
    
    bool is_valid = true;
    
    is_valid = process_impl(anf, variables_map, b_reindex_variables, mode);
    
    if (is_valid && b_normalize_variables) {
        is_valid = normalize_variables(anf, b_reindex_variables);
    };
    
    if (is_valid) {
        if (std::strlen(output_file_name) != 0) {
            save(anf, output_file_name, output_format);
        };
    } else {
        throw std::invalid_argument("Processing failed");
    };
};

void process_cnf(CGenVariablesMap& variables_map,
            const char* const input_file_name, const char* const output_file_name,
            const CGenOutputFormat output_format, const CGenTraceFormat trace_format,
            const bool b_reindex_variables, const bool b_normalize_variables,
            const bal::FormulaProcessingMode mode) {
    bal::Cnf cnf;
    load_impl<bal::Cnf, bal::DimacsStreamReader>(cnf, input_file_name);
    variables_define(cnf, variables_map);
    
    bool is_valid = true;
    
    __CNF_TRACE_INITIALIZE(trace_format, output_file_name);
    is_valid = process_impl(cnf, variables_map, b_reindex_variables, mode);
    __CNF_TRACE_FINIALIZE;
    
    if (is_valid && b_normalize_variables) {
        is_valid = normalize_variables(cnf, b_reindex_variables);
    };
    
    if (is_valid) {
        if (std::strlen(output_file_name) != 0) {
            save(cnf, output_file_name, output_format);
        };
    } else {
        throw std::invalid_argument("Processing failed");
    };
};
