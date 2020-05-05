//
//  CGen
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include <stdexcept>
#include <string>
#include <iostream>
#include "commandline.hpp"
#include "sha1.hpp"
#include "sha256.hpp"
#include "cnfencoding.hpp"

constexpr size_t APP_OPTIONS_SIZE = 15;
constexpr const char* const APP_OPTIONS[APP_OPTIONS_SIZE] = {
    "f", "v", "r",
    "add_max_args", "xor_max_args",
    "assign_after_encoding",
    "h", "help", "version",
    "no_variable_reindexing",
    "n", "normalize_variables",
    "m",
    "t", "trace",
};

void print_arg_ignore(const char* const message, const char* const arg) {
    std::cout << "Ignoring " << message << ": " << arg << std::endl;
};

// check if the file_name has the specified extension, case insensitive
bool is_file_extension(const std::string& file_name, const std::string& value) {
    const std::string::size_type idx = file_name.find_last_of(".");
    if (idx != std::string::npos) {
        std::string file_extension = file_name.substr(idx + 1);
        std::transform(file_extension.begin(), file_extension.end(), file_extension.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        std::string value_lc = value;
        std::transform(value_lc.begin(), value_lc.end(), value_lc.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        return file_extension.compare(value_lc) == 0;
    } else {
        return false;
    };
};

void validate_file_extension(const std::string& file_name, const std::string& value, CGenCommandInfo& info,
                             const CGenFormulaType formula_type, const CGenOutputFormat output_format,
                             const char* const error_message) {
    if (is_file_extension(file_name, value)) {
        if (!info.b_formula_type_specified) {
            info.formula_type = formula_type;
            info.output_format = output_format;
            info.b_formula_type_specified = true;
        } else if (info.formula_type != formula_type) {
            throw std::invalid_argument(error_message);
        };
    };
};

std::string CGenCommandLineReader::parse_variable(CGenVariablesMap& variables_map, const CGenCommand command) {
    std::string variable_name;
    bal::variableid_t variable_id = bal::VARIABLEID_UNASSIGNED;
    
    if (is_token(TextReader::ttDec)) {
        // binary variable
        variable_id = bal::variable_t__from_uint(read_uint32(bal::VARIABLEID_MIN + 1, bal::VARIABLEID_MAX + 1));
        variable_name = std::to_string(variable_id + 1);
    } else if (is_token(TextReader::ttLiteral)) {
        // named variable
        variable_name = read_literal();
    } else {
        parse_error(ERROR_INVALID_VARIABLE_NAME);
    };
    
    if (is_symbol('=')) { // value immediately after =
        read_symbol('=');
    } else if (is_eol()) { // value as a next argument
        read_eol();
    } else {
        parse_error(ERROR_MISSING_VARIABLE_VALUE);
    };
    
    CGenVariableInfo variable_value;
    
    if (is_token("random")) {
        skip_token();
        variable_value.mode = vmRandom;
    } else if (is_token("compute")) {
        skip_token();
        variable_value.mode = vmCompute;
    } else if (variable_id == bal::VARIABLEID_UNASSIGNED && is_token("string")) {
        skip_token();
        variable_value.mode = vmValue;
        read_symbol(':');
        // take the rest of the argument as a value
        // needed because the shell removes quotes from arguments
        variable_value.data = read_until_eol().c_str();
    } else {
        variable_value.mode = vmValue;
        variable_value.data = read_variable_value();
        
        if (variable_id != bal::VARIABLEID_UNASSIGNED && variable_value.data.size() != 1) {
            // binary variable requires binary value
            parse_error(ERROR_INVALID_BINARY_VALUE);
        };
    };
    
    auto r = variables_map.insert({variable_name, variable_value});
    if (!r.second) {
        parse_error(ERROR_DUPLICATED_VARIABLE_NAME);
    };
    
    return variable_name;
};

void CGenCommandLineReader::parse_variable_except(CGenVariablesMap& variables_map, const std::string& variable_name) {
    if (variable_name.empty()) {
        parse_error(ERROR_EXCEPT_MUST_FOLLOW_DEFINITION);
    };
    if (is_binary_variable_name(variable_name)) {
        parse_error(ERROR_BINARY_VARIABLE_EXCEPT_INCOMPATIBLE);
    };
    
    CGenVariableInfo& variable_value = variables_map.find(variable_name)->second;
    
    if (variable_value.replace_existing) {
        parse_error(ERROR_EXCEPT_REPLACE_INCOMPATIBLE);
    };
    
    skip_token();
    read_symbol(':');
    const uint32_t param1 = read_uint32();
    if (is_symbol('.')) {
        if (param1 == 0) {
            parse_error(ERROR_RANGE_FIRST_ZERO);
        };
        variable_value.except_range_first = param1 - 1; // becomes zero based
        read_symbol('.');
        read_symbol('.');
        const uint32_t param2 = read_uint32(); // index of the last element, 1 based
        if (param2 < variable_value.except_range_first) {
            parse_error(ERROR_RANGE_FIRST_LAST);
        };
        variable_value.except_range_size = param2 - variable_value.except_range_first;
    } else {
        if (param1 == 0) {
            parse_error(ERROR_EXCEPT_ZERO);
        };
        variable_value.except_count = param1;
    };
};

void CGenCommandLineReader::parse_variable_pad(CGenVariablesMap& variables_map, const std::string& variable_name) {
    if (variable_name.empty()) {
        parse_error(ERROR_PAD_MUST_FOLLOW_DEFINITION);
    };
    if (is_binary_variable_name(variable_name)) {
        parse_error(ERROR_BINARY_VARIABLE_PAD_INCOMPATIBLE);
    };
    
    CGenVariableInfo& variable_value = variables_map.find(variable_name)->second;
    
    if (variable_value.mode != vmValue) {
        parse_error(ERROR_PAD_MUST_FOLLOW_VALUE);
    };
    
    skip_token();
    read_symbol(':');
    if (is_token("sha1") || is_token("SHA1")) {
        variable_value.data = acl::SHA1<bal::Literal<bal::Cnf>>::pad_message(variable_value.data.data(),
                                                                             variable_value.data.size());
        skip_token();
    } else if (is_token("sha256") || is_token("SHA256")) {
        variable_value.data = acl::SHA256<bal::Literal<bal::Cnf>>::pad_message(variable_value.data.data(),
                                                                               variable_value.data.size());
        skip_token();
    } else {
        parse_error(ERROR_PAD_UNKNOWN_VALUE);
    };
};

void CGenCommandLineReader::parse_variable_replace(CGenVariablesMap& variables_map, const std::string& variable_name) {
    if (variable_name.empty()) {
        parse_error(ERROR_REPLACE_MUST_FOLLOW_DEFINITION);
    };
    if (is_binary_variable_name(variable_name)) {
        parse_error(ERROR_BINARY_VARIABLE_REPLACE_INCOMPATIBLE);
    };
    
    CGenVariableInfo& variable_value = variables_map.find(variable_name)->second;
    
    if (variable_value.except_count > 0 || variable_value.except_range_size > 0) {
        parse_error(ERROR_EXCEPT_REPLACE_INCOMPATIBLE);
    };

    variable_value.replace_existing = true;
};

void CGenCommandLineReader::parse_variable_compute_mode(CGenVariablesMap& variables_map,
                                                        const std::string& variable_name,
                                                        const CGenVariableComputeMode compute_mode) {
    CGenVariableInfo& variable_value = variables_map.find(variable_name)->second;
    
    if (variable_value.mode != vmCompute) {
        parse_error(ERROR_COMPUTE_MODE_CONTEXT);
    };

    variable_value.compute_mode = compute_mode;
};

void CGenCommandLineReader::parse(CGenCommandInfo& info) {
    std::string variable_name_last;
    
    skip_line(); // the first argument is the file name
    while (!is_eof()) {
        std::string variable_name_current;
        if (is_option()) {
            int option_index = read_option(APP_OPTIONS, APP_OPTIONS_SIZE);
            switch(option_index) {
                case 0: // format
                    if (is_eol()) {
                        read_eol();
                    };
                    if (is_token("anf") || is_token("ANF")) {
                        skip_token();
                        info.formula_type = ftAnf;
                        info.output_format = ofAnfPolybori;
                        info.b_formula_type_specified = true;
                    } else if (is_token("cnf") || is_token("CNF") ||
                               is_token("dimacs_cnf") || is_token("dimacs_cnf")) {
                        skip_token();
                        info.formula_type = ftCnf;
                        info.output_format = ofCnfDimacs;
                        info.b_formula_type_specified = true;
                    } else if (is_token("vig") || is_token("VIG") ||
                               is_token("vig_graphml") || is_token("VIG_GraphML")) {
                        skip_token();
                        info.formula_type = ftCnf;
                        info.output_format = ofCnfVIGGraphML;
                        info.b_formula_type_specified = true;
                    } else if (is_token("vigw") || is_token("VIGW") ||
                               is_token("vigw_graphml") || is_token("VIGW_GraphML")) {
                        skip_token();
                        info.formula_type = ftCnf;
                        info.output_format = ofCnfWeightedVIGGraphML;
                        info.b_formula_type_specified = true;
                    } else if (is_token("vig_gexf") || is_token("VIG_GEXF")) {
                        skip_token();
                        info.formula_type = ftCnf;
                        info.output_format = ofCnfVIGGEXF;
                        info.b_formula_type_specified = true;
                    } else {
                        parse_error(ERROR_UNKNOWN_FORMAT);
                    };
                    break;
                case 1: // variable
                    if (info.command != cmdEncode &&
                        info.command != cmdProcess) {
                        parse_error(ERROR_V_MUST_FOLLOW_ENCODE_PROCESS);
                    };
                    if (is_eol()) {
                        read_eol();
                    };
                    variable_name_current = parse_variable(info.variables_map, info.command);
                    break;
                case 2: // rounds
                    if (info.command != cmdEncode) {
                        parse_error(ERROR_R_MUST_FOLLOW_ENCODE);
                    };
                    if (is_eol()) {
                        read_eol();
                    };
                    info.rounds = read_uint32(1, UINT32_MAX);
                    break;
                case 3: // add_max_agrs
                    read_symbol('=');
                    info.add_max_args = read_uint32();
                    if (info.add_max_args == 0) {
                        parse_error(ERROR_ADD_MAX_ARGS_RANGE);
                    };
                    break;
                case 4: // xor_max_args
                    read_symbol('=');
                    info.xor_max_args = read_uint32();
                    if (info.xor_max_args == 0) {
                        parse_error(ERROR_XOR_MAX_ARGS_RANGE);
                    };
                    break;
                case 5: // assign_after_encoding
                    info.b_assign_after_encoding = true;
                    break;
                case 6: // help
                case 7:
                    info.command = cmdHelp;
                    break;
                case 8: // version
                    info.command = cmdVersion;
                    break;
                case 9: // no_variable_reindexing
                    info.b_reindex_variables = false;
                    break;
                case 10:
                case 11: // normalize_variables
                    info.b_normalize_variables_specified = true;
                    break;
                case 12: // mode
                    if (is_eol()) {
                        read_eol();
                    };
                    if (is_token("o") || is_token("original")) {
                        skip_token();
                        info.mode = bal::fpmOriginal;
                    } else if (is_token("a") || is_token("all")) {
                        skip_token();
                        info.mode = bal::fpmAll;
                    } else if (is_token("u") || is_token("unoptimized")) {
                        skip_token();
                        info.mode = bal::fpmUnoptimized;
                    } else {
                        parse_error(ERROR_MODE_UNKNOWN_VALUE);
                    };
                    info.b_mode_assigned = true;
                    break;
                case 13: // trace
                case 14:
                    if (is_eol()) {
                        read_eol();
                    };
                    if (is_token("n") || is_token("native") || is_token("N") || is_token("NATIVE")) {
                        skip_token();
                        info.trace_format = tfNativeFile;
                    } else if (is_token("d") || is_token("debug") || is_token("D") || is_token("DEBUG")) {
                        skip_token();
                        info.trace_format = tfNativeStdOut;
                    } else if (is_token("gexf") || is_token("GEXF")) {
                        skip_token();
                        info.trace_format = tfCnfVIGGEXF;
                    } else {
                        parse_error(ERROR_TRACE_UNKNOWN_VALUE);
                    };
                    break;
                default:
                    print_arg_ignore(ERROR_UNKNOWN_OPTION, get_current_line());
                    read_until_eol();
                    break;
            };
        } else if (is_token("encode")) {
            info.command = cmdEncode;
            skip_token();
            read_eol();
            if (is_token("SHA1") || is_token("sha1")) {
                skip_token();
                info.algorithm = algSHA1;
                info.rounds = SHA1_ROUNDS_NUMBER;
            } else if (is_token("SHA256") || is_token("sha256")) {
                skip_token();
                info.algorithm = algSHA256;
                info.rounds = SHA256_ROUNDS_NUMBER;
            } else {
                parse_error(ERROR_UNKNOWN_ALGORITHM);
            };
        } else if (is_token("process")) {
            info.command = cmdProcess;
            skip_token();
        } else if (is_token("except")) {
            assert(variable_name_current.empty());
            parse_variable_except(info.variables_map, variable_name_last);
            variable_name_current = variable_name_last;
        } else if (is_token("pad")) {
            assert(variable_name_current.empty());
            parse_variable_pad(info.variables_map, variable_name_last);
            variable_name_current = variable_name_last;
        } else if (is_token("replace")) {
            assert(variable_name_current.empty());
            parse_variable_replace(info.variables_map, variable_name_last);
            variable_name_current = variable_name_last;
            skip_token();
        } else if (is_token("complete") && !variable_name_last.empty()) {
            assert(variable_name_current.empty());
            parse_variable_compute_mode(info.variables_map, variable_name_last, vcmComplete);
            variable_name_current = variable_name_last;
            skip_token();
        } else if (is_token("constant") && !variable_name_last.empty()) {
            assert(variable_name_current.empty());
            parse_variable_compute_mode(info.variables_map, variable_name_last, vcmConstant);
            variable_name_current = variable_name_last;
            skip_token();
        } else if (is_token("difference") && !variable_name_last.empty()) {
            assert(variable_name_current.empty());
            parse_variable_compute_mode(info.variables_map, variable_name_last, vcmDifference);
            variable_name_current = variable_name_last;
            skip_token();
        } else if (info.command == cmdEncode && info.output_file_name.empty()) {
            info.output_file_name = read_until_eol();
        } else if ((info.command == cmdProcess) &&
                   info.input_file_name.empty()) {
            info.input_file_name = read_until_eol();
        } else if ((info.command == cmdProcess) &&
                   info.output_file_name.empty()) {
            info.output_file_name = read_until_eol();
        } else {
            print_arg_ignore(ERROR_UNKNOWN_ARGUMENT, get_current_line());
        };
        read_eol();
        variable_name_last = variable_name_current;
    };
    
    // validate the result
    // assign default values
    
    if (info.command == cmdProcess) {
        if (info.input_file_name.empty()) {
            parse_error(ERROR_MISSING_INPUT_FILE_NAME);
        };
        validate_file_extension(info.input_file_name, "cnf", info, ftCnf, ofCnfDimacs, ERROR_INPUT_FILE_FORMAT_MISMATCH);
        validate_file_extension(info.input_file_name, "anf", info, ftAnf, ofAnfPolybori, ERROR_INPUT_FILE_FORMAT_MISMATCH);
    };
    
    if (info.command == cmdEncode || info.command == cmdProcess) {
        if (!info.output_file_name.empty()) {
            validate_file_extension(info.output_file_name, "cnf", info, ftCnf, ofCnfDimacs, ERROR_OUTPUT_FILE_FORMAT_MISMATCH);
            validate_file_extension(info.output_file_name, "anf", info, ftAnf, ofAnfPolybori, ERROR_OUTPUT_FILE_FORMAT_MISMATCH);
        };
        
        if (!info.b_formula_type_specified) {
            if (info.command == cmdEncode) {
                info.formula_type = ftCnf;
                info.output_format = ofCnfDimacs;
                info.b_formula_type_specified = true;
            } else {
                parse_error(ERROR_FORMULA_TYPE_UNDEFINED);
            };
        };
    } else {
        if (info.variables_map.size() > 0) {
            parse_error(ERROR_V_MUST_FOLLOW_ENCODE_PROCESS);
        };
    };
    
    if (info.b_normalize_variables_specified) {
        if (info.command != cmdEncode && info.command != cmdProcess) {
            parse_error(ERROR_NORMALIZE_VARIABLES_MUST_FOLLOW_ENCODE_PROCESS);
        };
    };
    
    if (info.b_mode_assigned) {
        if (info.command != cmdEncode && info.command != cmdProcess) {
            parse_error(ERROR_MODE_UNSUPPORTED_COMMAND);
        };
        if (info.formula_type == ftAnf && info.mode != bal::fpmUnoptimized) {
            parse_error(ERROR_ANF_UNOPTIMIZED_ONLY);
        };
    } else if (info.formula_type == ftAnf) {
        info.mode = bal::fpmUnoptimized;
    } else {
        info.mode = bal::fpmOriginal;
    };
    
    if (info.command != cmdEncode) {
        if (info.add_max_args > 0) {
            parse_error(ERROR_ADD_MAX_ARGS_MUST_FOLLOW_ENCODE);
        };
        if (info.xor_max_args > 0) {
            parse_error(ERROR_XOR_MAX_ARGS_MUST_FOLLOW_ENCODE);
        };
        if (info.b_assign_after_encoding) {
            parse_error(ERROR_AAE_MUST_FOLLOW_ENCODE);
        };
    };
    
    if (info.trace_format != tfNone) {
#ifdef CNF_TRACE
        if ((info.formula_type != ftCnf) ||
            (info.command != cmdEncode && info.command != cmdProcess)) {
            parse_error(ERROR_TRACE_UNSUPPORTED_COMMAND);
        };
#else
        parse_error(ERROR_TRACE_NOT_SUPPORTED);
#endif
    };
};
