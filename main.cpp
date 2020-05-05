//
//  CGen
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include <stdexcept>
#include <iostream>
#include "shared.hpp"
#include "commandline.hpp"
#include "commands.hpp"

void print_version() {
    std::cout << APP_TITLE << " version " << APP_VERSION << std::endl;
};

void print_help() {
    print_version();
    std::cout << APP_DESCRIPTION << std::endl;
    std::cout << APP_USAGE_SHORT;
    std::cout << APP_USAGE_LONG;
};

void generate_output_file_name_var(CGenCommandInfo& info, const std::string variable_name, std::string& output_file_name) {
    auto map_entry = info.variables_map.find(variable_name);
    if (map_entry != info.variables_map.end()) {
        std::string prefix = variable_name;
        transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);
        output_file_name += prefix;
        
        const CGenVariableInfo* p_value = &(map_entry->second);
        if (p_value->except_count > 0) {
            output_file_name += "v" + std::to_string(p_value->except_count);
        } else if (p_value->except_range_size) {
            output_file_name += "v" + std::to_string(p_value->except_range_size);
        } else {
            output_file_name += "c";
        };
    };
};

void generate_output_file_name(CGenCommandInfo& info) {
    if ((info.command == cmdEncode || info.command == cmdProcess) && info.output_file_name.empty()) {
        std::string output_file_name;
        if (info.command == cmdEncode) {
            switch(info.algorithm) {
                case algSHA1:
                    output_file_name = "sha1";
                    break;
                case algSHA256:
                    output_file_name = "sha256";
                    break;
                default:
                    assert(false);
            };
        
            output_file_name += "r" + std::to_string(info.rounds);
        
            generate_output_file_name_var(info, "M", output_file_name);
            generate_output_file_name_var(info, "H", output_file_name);

            if (info.mode == bal::fpmUnoptimized) {
                output_file_name += "_u";
            };
        } else if (info.command == cmdProcess) {
            // generate only if there is a conversion
            if (info.formula_type == ftCnf && (info.output_format == ofCnfVIGGraphML ||
                                               info.output_format == ofCnfWeightedVIGGraphML ||
                                               info.output_format == ofCnfVIGGEXF)) {
                output_file_name = info.input_file_name;
            };
        };
        if (!output_file_name.empty()) {
            switch(info.output_format) {
                case ofAnfPolybori:
                    output_file_name += ".anf";
                    break;
                case ofCnfDimacs:
                    output_file_name += ".cnf";
                    break;
                case ofCnfVIGGraphML:
                    output_file_name += ".graphml";
                    break;
                case ofCnfWeightedVIGGraphML:
                    output_file_name += "_w.graphml";
                    break;
                case ofCnfVIGGEXF:
                    output_file_name += ".gexf";
                    break;
                default:
                    assert(false);
            };
            info.output_file_name = output_file_name;
        };
    };
};

int main(int argc, const char * argv[]) {
    try {
        CGenCommandInfo info;
        CGenCommandLineReader command_line_reader(argc, argv);
        command_line_reader.parse(info);
        
        switch(info.command) {
            case cmdNone:
                std::cout << ERROR_COMMAND_NONE << std::endl;
                print_help();
            case cmdEncode:
                if (info.algorithm == algNone) {
                    throw std::invalid_argument(ERROR_MISSING_ALGORITHM);
                };
                
                generate_output_file_name(info);
                if (info.output_file_name.empty()) {
                    throw std::invalid_argument(ERROR_MISSING_OUTPUT_FILE_NAME);
                };
                
                _assert_level_1(info.b_formula_type_specified);
                std::cout << "Encoding " << (info.algorithm == algSHA1 ? "SHA-1" : "SHA-256");
                std::cout << " into " << get_formula_type_title(info.formula_type) << std::endl;
                
                if (info.formula_type == ftCnf) {
                    encode_cnf(info.algorithm, info.rounds,
                               info.variables_map,
                               info.add_max_args, info.xor_max_args,
                               info.output_file_name.c_str(),
                               info.output_format, info.trace_format,
                               info.b_reindex_variables,
                               info.b_normalize_variables_specified,
                               info.b_assign_after_encoding,
                               info.mode);
                } else if (info.formula_type == ftAnf) {
                    encode_anf(info.algorithm, info.rounds,
                               info.variables_map,
                               info.add_max_args, info.xor_max_args,
                               info.output_file_name.c_str(),
                               info.output_format, info.trace_format,
                               info.b_reindex_variables,
                               info.b_normalize_variables_specified,
                               info.b_assign_after_encoding,
                               info.mode);
                } else {
                    _assert_level_1(false);
                };
                break;
            case cmdProcess:
                _assert_level_1(info.b_formula_type_specified);
                generate_output_file_name(info);
                std::cout << "Processing " << get_formula_type_title(info.formula_type) << " formula" << std::endl;
                if (info.formula_type == ftCnf) {
                    process_cnf(info.variables_map,
                                info.input_file_name.data(), info.output_file_name.data(),
                                info.output_format, info.trace_format,
                                info.b_reindex_variables, info.b_normalize_variables_specified, info.mode);
                } else if (info.formula_type == ftAnf) {
                    process_anf(info.variables_map,
                                info.input_file_name.data(), info.output_file_name.data(),
                                info.output_format, info.trace_format,
                                info.b_reindex_variables, info.b_normalize_variables_specified, info.mode);
                } else {
                    _assert_level_1(false);
                };
                break;
            case cmdHelp:
                print_help();
                break;
            case cmdVersion:
                print_version();
                break;
        };
    }
    catch (TextReaderException e) {
        std::cerr << ERROR_COMMAND_LINE_PARSE << ": " << e.get_line() << std::endl;
        std::cerr << "Position " << e.get_pos() << ": " << e.get_message() << std::endl;
        std::cerr << "See --help for reference" << std::endl;
        return 1;
    }
    catch (std::invalid_argument e) {
        std::cerr << ERROR_INVALID_ARGUMENT << ": " << e.what() << std::endl;
        return 1;
    }
    catch (std::out_of_range e) {
        std::cerr << ERROR_OUT_OF_RANGE << ": " << e.what() << std::endl;
        return 1;
    };
    
    return 0;
}
