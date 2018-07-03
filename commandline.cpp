//
//  CNFGen
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include "commandline.hpp"
#include "sha1.hpp"
#include "sha256.hpp"
#include "cnfencoderbit.hpp"

const constexpr size_t APP_OPTIONS_SIZE = 7;
const constexpr char* APP_OPTIONS[APP_OPTIONS_SIZE] = {
    "v", "r", "add_max_args", "xor_max_args", "h", "help", "version"};

void print_arg_ignore(const char* const message, const char* const arg) {
    std::cout << "Ignoring " << message << ": " << arg << std::endl;
};

ple::VariablesArray CnfGenCommandLineReader::read_remaining_line_bytes() {
    const std::string s_current_line = read_until_eol();
    const uint32_t size_bytes = (uint32_t)s_current_line.size();
    const std::string current_line = s_current_line.c_str();
    
    ple::VariablesArray value(size_bytes << 3);
    
    // encode bit by bit for each byte
    ple::literalid_t* p_literal = value.data();
    for (auto i = 0; i < size_bytes; i++) {
        for (auto j = 0; j < 8; j++) {
            *p_literal = literal_t__constant(current_line[i] & (1 << (7 - j)));
            p_literal++;
        };
    };
    
    return value;
};

CnfGenVariableInfo* CnfGenCommandLineReader::parse_variable_value(CnfGenVariablesMap& variables_map,
                                                                  const CnfGenCommand command) {
    if (!is_token(TextReader::ttLiteral)) {
        parse_error(ERROR_INVALID_VARIABLE_NAME);
    };
    
    std::string variable_name = read_literal();
    
    if (is_symbol('=')) { // value immediately after =
        read_symbol('=');
    } else if (is_eol()) { // value as a next argument
        read_eol();
    } else {
        parse_error(ERROR_MISSING_VARIABLE_VALUE);
    };
    
    CnfGenVariableInfo variable_value;
    
    if (command == cmdDefine) {
        variable_value.mode = vmDefine;
        variable_value.data = read_variable_value();
    } else {
        if (is_token("random")) {
            skip_token();
            variable_value.mode = vmRandom;
            read_symbol(':');
            variable_value.random_count = read_uint32(1, UINT32_MAX);
        } else if (is_token("compute")) {
            skip_token();
            variable_value.mode = vmCompute;
        } else if (is_token("string")) {
            skip_token();
            variable_value.mode = vmValue;
            read_symbol(':');
            // take the rest of the argument as a value
            variable_value.data = read_remaining_line_bytes();
        } else {
            variable_value.mode = vmValue;
            variable_value.data = read_variable_value();
        };
    };
    
    auto r = variables_map.insert(CnfGenVariablesMap::value_type(variable_name, variable_value));
    if (!r.second) {
        parse_error(ERROR_DUPLICATED_VARIABLE_NAME);
    };
    
    return &(r.first->second);
};

void CnfGenCommandLineReader::parse_variable_require_constant(const CnfGenVariableInfo* p_variable_value) {
    if (p_variable_value == nullptr) {
        parse_error(ERROR_EXCEPT_PAD_MUST_FOLLOW_CONSTANT);
    };
    if (p_variable_value->mode != vmValue && p_variable_value->mode != vmDefine) {
        parse_error(ERROR_EXCEPT_PAD_MUST_FOLLOW_CONSTANT);
    };
};

void CnfGenCommandLineReader::parse_variable_except(CnfGenVariableInfo* p_variable_value) {
    skip_token();
    read_symbol(':');
    p_variable_value->var_range_first = read_uint32();
    if (p_variable_value->var_range_first == 0) {
        parse_error(ERROR_RANGE_FIRST_ZERO);
    };
    read_symbol('.');
    read_symbol('.');
    p_variable_value->var_range_last = read_uint32();
    if (p_variable_value->var_range_first > p_variable_value->var_range_last) {
        parse_error(ERROR_RANGE_FIRST_LAST);
    };
};

void CnfGenCommandLineReader::parse_variable_pad(CnfGenVariableInfo* p_variable_value) {
    skip_token();
    read_symbol(':');
    if (is_token("sha1") || is_token("SHA1")) {
        p_variable_value->data =
            acl::SHA1<ple::CnfEncoderBit>::pad_message(p_variable_value->data.data(),
                                                       p_variable_value->data.size());
        skip_token();
    } else if (is_token("sha256") || is_token("SHA256")) {
        acl::SHA256<ple::CnfEncoderBit>::pad_message(p_variable_value->data.data(),
                                                   p_variable_value->data.size());
        skip_token();
    } else {
        parse_error(ERROR_PAD_UNKNOWN_VALUE);
    };
};

void CnfGenCommandLineReader::parse(CnfGenCommandInfo& info) {
    CnfGenVariableInfo* p_last_variable = nullptr;
    
    skip_line(); // the first argument is the file name
    while (!is_eof()) {
        CnfGenVariableInfo* p_current_variable = nullptr;
        if (is_option()) {
            int option_index = read_option(APP_OPTIONS, APP_OPTIONS_SIZE);
            switch(option_index) {
                case 0: { // variable
                    if (info.command != cmdEncode &&
                        info.command != cmdAssign &&
                        info.command != cmdDefine) {
                        parse_error(ERROR_V_MUST_FOLLOW_ASSIGN_ENCODE_DEFINE);
                    };
                    if (is_eol()) {
                        read_eol();
                    };
                    p_current_variable = parse_variable_value(info.variables_map, info.command);
                    break;
                };
                case 1: // rounds
                    if (info.command != cmdEncode) {
                        parse_error(ERROR_R_MUST_FOLLOW_ENCODE);
                    };
                    if (is_eol()) {
                        read_eol();
                    };
                    info.rounds = read_uint32(1, UINT32_MAX);
                    break;
                case 2: // add_max_agrs
                    if (info.command != cmdEncode) {
                        parse_error(ERROR_ADD_MAX_ARGS_MUST_FOLLOW_ENCODE);
                    };
                    read_symbol('=');
                    info.add_max_args = read_uint32();
                    if (info.add_max_args < ple::Cnf::ADD_MAX_ARGS_MIN ||
                        info.add_max_args > ple::Cnf::ADD_MAX_ARGS_MAX) {
                        parse_error(ERROR_ADD_MAX_ARGS_RANGE);
                    };
                    break;
                case 3: // xor_max_args
                    if (info.command != cmdEncode) {
                        parse_error(ERROR_XOR_MAX_ARGS_MUST_FOLLOW_ENCODE);
                    };
                    read_symbol('=');
                    info.xor_max_args = read_uint32();
                    if (info.xor_max_args < ple::Cnf::XOR_MAX_ARGS_MIN ||
                        info.xor_max_args > ple::Cnf::XOR_MAX_ARGS_MAX) {
                        parse_error(ERROR_XOR_MAX_ARGS_RANGE);
                    };
                    break;
                case 4: // help
                case 5:
                    info.command = cmdHelp;
                    break;
                case 6: // version
                    info.command = cmdVersion;
                    break;
                default:
                    print_arg_ignore(ERROR_UNKNOWN_OPTION, get_current_line());
                    read_until_eol();
                    break;
            };
        } else if (is_token("assign")) {
            info.command = cmdAssign;
            skip_token();
        } else if (is_token("define")) {
            info.command = cmdDefine;
            skip_token();
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
        } else if (is_token("except")) {
            if (info.command != cmdEncode && info.command != cmdAssign) {
                parse_error(ERROR_EXCEPT_MUST_FOLLOW_ASSIGN_ENCODE);
            };
            assert(p_current_variable == nullptr);
            parse_variable_require_constant(p_last_variable);
            parse_variable_except(p_last_variable);
            p_current_variable = p_last_variable;
        } else if (is_token("pad")) {
            assert(p_current_variable == nullptr);
            parse_variable_require_constant(p_last_variable);
            parse_variable_pad(p_last_variable);
            p_current_variable = p_last_variable;
        } else if (info.command == cmdEncode && info.output_file_name.empty()) {
            info.output_file_name = read_until_eol();
        } else if ((info.command == cmdAssign || info.command == cmdDefine) &&
                   info.input_file_name.empty()) {
            info.input_file_name = read_until_eol();
        } else if ((info.command == cmdAssign || info.command == cmdDefine) &&
                   info.output_file_name.empty()) {
            info.output_file_name = read_until_eol();
        } else {
            print_arg_ignore(ERROR_UNKNOWN_ARGUMENT, get_current_line());
        };
        read_eol();
        p_last_variable = p_current_variable;
    };
};
