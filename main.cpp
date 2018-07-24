//
//  CGen
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

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
                
                if (info.output_file_name.size() == 0) {
                    throw std::invalid_argument(ERROR_MISSING_OUTPUT_FILE_NAME);
                };
                
                std::cout << "Encoding " << (info.algorithm == algSHA1 ? "SHA-1" : "SHA-256") << " into ";
                std::cout << (info.format == fmtCnfDimacs ? "CNF in DIMACS format" : "ANF in PolyBoRi format") << std::endl;
                
                if (info.format == fmtCnfDimacs) {
                    encode_cnf(info.algorithm, info.rounds,
                               info.variables_map, info.is_normalize_variables_specified,
                               info.add_max_args, info.xor_max_args,
                               info.output_file_name.c_str());
                } else {
                    encode_anf(info.algorithm, info.rounds,
                               info.variables_map, info.is_normalize_variables_specified,
                               info.output_file_name.c_str());
                };
                break;
            case cmdAssign:
            case cmdDefine:
                assign(info.variables_map, info.is_normalize_variables_specified,
                       info.input_file_name.data(), info.output_file_name.data());
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
        std::cout << ERROR_COMMAND_LINE_PARSE << ": " << e.get_line() << std::endl;
        std::cout << "Position " << e.get_pos() << ": " << e.get_message() << std::endl;
        std::cout << "See --help for reference" << std::endl;
        return 1;
    }
    catch (std::invalid_argument e) {
        std::cout << ERROR_INVALID_ARGUMENT << ": " << e.what() << std::endl;
        return 1;
    };
    
    return 0;
}
