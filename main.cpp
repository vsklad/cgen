//
//  CNFGen
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include <iostream>
#include "cnfencoderbit.hpp"
#include "sha1.hpp"
#include "sha256.hpp"
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
        CnfGenCommandInfo info;
        CnfGenCommandLineReader command_line_reader(argc, argv);
        command_line_reader.parse(info);
        
        switch(info.command) {
            case cmdNone:
                std::cout << ERROR_COMMAND_NONE << std::endl;
                print_help();
            case cmdEncode:
                switch(info.algorithm) {
                    case algNone:
                        throw std::invalid_argument(ERROR_MISSING_ALGORITHM);
                    case algSHA1:
                        encode<acl::SHA1<ple::CnfEncoderBit>>(info.rounds, info.variables_map,
                            info.add_max_args, info.xor_max_args, info.output_file_name.data());
                        break;
                    case algSHA256:
                        encode<acl::SHA256<ple::CnfEncoderBit>>(info.rounds, info.variables_map,
                            info.add_max_args, info.xor_max_args, info.output_file_name.data());
                        break;
                };
                break;
            case cmdAssign:
            case cmdDefine:
                assign(info.variables_map, info.input_file_name.data(), info.output_file_name.data());
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
