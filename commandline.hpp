//
//  CNFGen
//  https://cnfgen.sophisticatedways.net
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
    CnfGenCommand command = cmdNone;
    CnfGenAlgorithm algorithm = algNone;
    CnfGenVariablesMap variables_map;
    uint32_t rounds = 0;
    uint32_t add_max_args = ple::Cnf::ADD_MAX_ARGS_DEFAULT;
    uint32_t xor_max_args = ple::Cnf::ADD_MAX_ARGS_DEFAULT;
    std::string input_file_name;
    std::string output_file_name;
} CnfGenCommandInfo;

class CnfGenCommandLineReader:
    private virtual ple::CommandLineReader,
    private virtual ple::VariableTextReader {
        
    private:
        // needed because the shell removes quotes from arguments
        ple::VariablesArray read_remaining_line_bytes();
        
        CnfGenVariableInfo* parse_variable_value(CnfGenVariablesMap& variables_map, const CnfGenCommand command);
        void parse_variable_require_constant(const CnfGenVariableInfo* p_variable_value);
        void parse_variable_except(CnfGenVariableInfo* p_variable_value);
        void parse_variable_pad(CnfGenVariableInfo* p_variable_value);
        
    public:
        CnfGenCommandLineReader(int argc, const char* const argv[]): ple::CommandLineReader(argc, argv) {};
        
        void parse(CnfGenCommandInfo& info);
};

#endif /* commandline_hpp */
