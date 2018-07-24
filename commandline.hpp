//
//  CGen
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef commandline_hpp
#define commandline_hpp

#include "shared.hpp"
#include "commandlinereader.hpp"
#include "variablesio.hpp"
#include "cnf.hpp"

typedef struct {
    CGenCommand command = cmdNone;
    CGenAlgorithm algorithm = algNone;
    CGenFormat format = fmtCnfDimacs;
    CGenVariablesMap variables_map;
    uint32_t rounds = 0;
    uint32_t add_max_args = ple::Cnf::ADD_MAX_ARGS_DEFAULT;
    uint32_t xor_max_args = ple::Cnf::ADD_MAX_ARGS_DEFAULT;
    std::string input_file_name;
    std::string output_file_name;
    bool is_normalize_variables_specified = false;
} CGenCommandInfo;

class CGenCommandLineReader:
    private virtual ple::CommandLineReader,
    private virtual ple::VariableTextReader {
        
    private:
        // needed because the shell removes quotes from arguments
        ple::VariablesArray read_remaining_line_bytes();
        
        CGenVariableInfo* parse_variable_value(CGenVariablesMap& variables_map, const CGenCommand command);
        void parse_variable_require_constant(const CGenVariableInfo* p_variable_value);
        void parse_variable_except(CGenVariableInfo* p_variable_value);
        void parse_variable_pad(CGenVariableInfo* p_variable_value);
        
    public:
        CGenCommandLineReader(int argc, const char* const argv[]): ple::CommandLineReader(argc, argv) {};
        
        void parse(CGenCommandInfo& info);
};

#endif /* commandline_hpp */
