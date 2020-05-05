//
//  CGen
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
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
    CGenFormulaType formula_type = ftCnf;
    CGenOutputFormat output_format = ofCnfDimacs;
    CGenTraceFormat trace_format = tfNone;
    CGenVariablesMap variables_map;
    uint32_t rounds = 0;
    uint32_t add_max_args = 0; // zero means unassigned
    uint32_t xor_max_args = 0; // zero means unassigned
    std::string input_file_name;
    std::string output_file_name;
    bool b_formula_type_specified = false;
    bool b_assign_after_encoding = false;
    bool b_reindex_variables = true;
    bool b_normalize_variables_specified = false;
    bool b_mode_assigned = false;
    bal::FormulaProcessingMode mode = bal::fpmOriginal;
} CGenCommandInfo;

class CGenCommandLineReader:
    private virtual bal::CommandLineReader,
    private virtual bal::VariableTextReader {
        
    private:
        std::string parse_variable(CGenVariablesMap& variables_map, const CGenCommand command);
        void parse_variable_except(CGenVariablesMap& variables_map, const std::string& variable_name);
        void parse_variable_pad(CGenVariablesMap& variables_map, const std::string& variable_name);
        void parse_variable_replace(CGenVariablesMap& variables_map, const std::string& variable_name);
        void parse_variable_compute_mode(CGenVariablesMap& variables_map, const std::string& variable_name,
                                         const CGenVariableComputeMode compute_mode);
    public:
        CGenCommandLineReader(int argc, const char* const argv[]): bal::CommandLineReader(argc, argv) {};
        
        void parse(CGenCommandInfo& info);
};

#endif /* commandline_hpp */
