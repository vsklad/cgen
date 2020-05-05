//
//  CGen
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef shared_hpp
#define shared_hpp

#include <map>
#include <vector>
#include "variablesarray.hpp"

#define APP_VERSION "1.2"
#define APP_TITLE "CGen"
#define APP_URL "https://cgen.sophisticatedways.net"
#define APP_DESCRIPTION "CGen is a tool for encoding SHA-1 and SHA-256 hash functions into CNF/DIMACS and ANF/PolyBoRi formats"

#define APP_USAGE_SHORT "\
Usage: \n\
    cgen encode (SHA1|SHA256) [-r <rounds>] [-v <name> <value>]... [<encoder_options>] [<output_options>] [<output_file_name>]\n\
    cgen process [<variable>]... [<output_options>] <input_file_name> [<output_file_name>]\n\
    cgen --help\n\
    cgen --version\n\
";

#define APP_USAGE_LONG "\
Commands:\n\
    encode (SHA1|SHA256) - generate the encoding\n\
    process - read DIMACS CNF from <input file name>, assign variables as specified, pre-process it and save it to <output file name>\n\
Options:\n\
    -f (ANF|CNF) - encoding form, CNF if not specified \n\
    -v <name> <value> - specification of the named variable,\n\
        its mapping to binary variables and/or its constant values\n\
        refer for detailed specifications online\n\
    -r <value> - number of SHA1 rounds to encode\n\
    -m ((unoptimized | u) | (all | a) | (original | o)) - processing mode\n\
    -h | --help\n\
    --version\n\
Further documentation and usage examples available at https://cgen.sophisticatedways.net.\n\
";

#define ERROR_COMMAND_NONE "No command specified"
#define ERROR_UNKNOWN_OPTION "unknown option"
#define ERROR_UNKNOWN_ARGUMENT "unknown argument"
#define ERROR_COMMAND_LINE_PARSE "Command line parsing error"
#define ERROR_INVALID_ARGUMENT "Error"
#define ERROR_OUT_OF_RANGE "Out of range"
#define ERROR_MISSING_ALGORITHM "Algorithm for encoding not specified"
#define ERROR_UNKNOWN_ALGORITHM "Unknown algorithm for encoding"
#define ERROR_INVALID_VARIABLE_NAME "Invalid variable name"
#define ERROR_MISSING_VARIABLE_VALUE "Variable value is missing"
#define ERROR_INVALID_BINARY_VALUE "Binary variable requires a single binary value"
#define ERROR_DUPLICATED_VARIABLE_NAME "Variable may only appear once on the options list"
#define ERROR_NO_VARIABLES "No variables specified"
#define ERROR_EXCEPT_MUST_FOLLOW_ASSIGN_ENCODE "\"except\" variable modifiers are only allowed for\"assign\" or \"encode\" commands"
#define ERROR_EXCEPT_MUST_FOLLOW_DEFINITION "\"except\" variable modifier must be part of variable definition"
#define ERROR_PAD_MUST_FOLLOW_DEFINITION "\"pad\" variable modifier must be part of variable definition"
#define ERROR_REPLACE_MUST_FOLLOW_DEFINITION "\"replace\" variable modifier must be part of variable definition"
#define ERROR_PAD_MUST_FOLLOW_VALUE "\"pad\" variable modifier is incompatible with \"random\" and \"compute\" variable modes"
#define ERROR_EXCEPT_REPLACE_INCOMPATIBLE "\"except\" and \"replace\" variable modifiers are incompatible"
#define ERROR_EXCEPT_ZERO "\"except\" variable modifier must specify a non-zero number of binary variables to skip"
#define ERROR_BINARY_VARIABLE_EXCEPT_INCOMPATIBLE "\"except\" modifier is invalid for a binary variable"
#define ERROR_BINARY_VARIABLE_PAD_INCOMPATIBLE "\"pad\" modifier is invalid for a binary variable"
#define ERROR_BINARY_VARIABLE_REPLACE_INCOMPATIBLE "\"replace\" modifier is invalid for a binary variable"
#define ERROR_RANGE_FIRST_LAST "For binary variables range the first element must be less or equal to the last"
#define ERROR_RANGE_FIRST_ZERO "For binary variables range the first element must be greater than 0"
#define ERROR_PAD_UNKNOWN_VALUE "\"pad\" variable modifier only supports \"SHA1\" and \"SHA256\" values"
#define ERROR_COMPUTE_MODE_CONTEXT "compute mode may be specified for a computed variable only"
#define ERROR_R_MUST_FOLLOW_ENCODE "Rounds option can only be specified after \"encode\""
#define ERROR_ADD_MAX_ARGS_MUST_FOLLOW_ENCODE "add_max_args option can only be specified after \"encode\""
#define ERROR_XOR_MAX_ARGS_MUST_FOLLOW_ENCODE "xor_max_args option can only be specified after \"encode\""
#define ERROR_AAE_MUST_FOLLOW_ENCODE "assign_after_encoding option can only be specified after \"encode\""
#define ERROR_V_MUST_FOLLOW_ENCODE_PROCESS "Variable options can only be specified for \"encode\" and \"process\" commands"
#define ERROR_NORMALIZE_VARIABLES_MUST_FOLLOW_ENCODE_PROCESS \
    "\"normaize variables\" option may only be specified for \"encode\" or \"process\" command"
#define ERROR_MISSING_INPUT_FILE_NAME "Input file name is not specified"
#define ERROR_INPUT_FILE_FORMAT_MISMATCH "Input file extension does not match the specified format"
#define ERROR_OUTPUT_FILE_FORMAT_MISMATCH "Output format is incompatible with the formula"
#define ERROR_MISSING_OUTPUT_FILE_NAME "Output file name is not specified"
#define ERROR_ROUNDS_RANGE "Rounds number is out of range"
#define ERROR_ADD_MAX_ARGS_RANGE "add_max_args should be greater than zero"
#define ERROR_XOR_MAX_ARGS_RANGE "xor_max_args should be greater than zero"
#define ERROR_ENCODE_UNKNOWN_VARIABLE_NAME "Unknown variable name, \"encode\" only expects \"M\" and \"H\" case sensitive"
#define ERROR_UNKNOWN_VARIABLE_NAME "named variable is not defined in the DIMACS file"
#define ERROR_VARIABLE_DEFINITION_MODE "named variable definition can only be its template or constant value"
#define ERROR_VARIABLE_DEFINITION_EXCEPT "named variable definition may not include \"except\" modifier"
#define ERROR_NAMED_VARIABLE_OUT_OF_RANGE " named variable includes a binary variable number greated than than the DIMACS file allows"
#define ERROR_COMPUTE_INVALID_ENCODING "Failed to compute variable values"
#define ERROR_COMPUTE_MESSAGE_NOT_SUPPORTED "Computing of message value is not supported"
#define ERROR_EXCEPT_VARIABLES_RANGE_OUT_OF_BOUNDS "\"except\" variables range is out of bounds"
#define ERROR_EXCEPT_NO_VARIABLES \
    "Specified number of binary variables to except from assignment is higher than the number of assignable non-constant variables present"
#define ERROR_RANDOM_NO_VARIABLES "There are no variables to assign random values to"
#define ERROR_FAILED_OPENING_OUTPUT_FILE "Failed to open the output file"
#define ERROR_UNKNOWN_FORMAT "Unknown output format"
#define ERROR_ANF_UNOPTIMIZED_ONLY "Only unoptimized ANF output is supported"
#define ERROR_PROCESS_UNSUPPORTED_MODE "Unsupported mode for \"process\" command"
#define ERROR_MODE_UNKNOWN_VALUE "Unknown \"mode\" option"
#define ERROR_TRACE_UNKNOWN_VALUE "Unknown \"trace\" option value"
#define ERROR_FORMULA_TYPE_UNDEFINED "Formula type, input or output format cannot be determined"
#define ERROR_OUTPUT_FORMAT_UNSUPPORTED "Output format is not supported for the chosen operation"
#define ERROR_MODE_UNSUPPORTED_COMMAND \
"\"mode\" option may only be specified for \"assign\", \"define\", \"encode\" and \"process\" commands"
#define ERROR_TRACE_UNSUPPORTED_COMMAND \
"\"trace\" option is only possible for \"assign\", \"define\", \"encode\" and \"process\" commands"
#define ERROR_TRACE_NOT_SUPPORTED "The application is built with configuration which does not support tracing"
#define MSG_FORMULA_IS_SATISFIABLE "The formula is SATISFIABLE"

enum CGenCommand {cmdNone, cmdEncode, cmdProcess, cmdHelp, cmdVersion};
enum CGenAlgorithm {algNone, algSHA1, algSHA256};
enum CGenFormulaType {ftCnf, ftAnf};
enum CGenOutputFormat {ofAnfPolybori, ofCnfDimacs, ofCnfVIGGraphML, ofCnfWeightedVIGGraphML, ofCnfVIGGEXF};
enum CGenTraceFormat {tfNone, tfNativeStdOut, tfNativeFile, tfCnfVIGGEXF};

enum CGenVariableMode {vmValue, vmRandom, vmCompute};
enum CGenVariableComputeMode {vcmComplete, vcmDifference, vcmConstant};

typedef struct {
    CGenVariableMode mode;
    CGenVariableComputeMode compute_mode = vcmDifference;
    uint32_t except_count = 0;
    uint32_t except_range_first = 0;
    uint32_t except_range_size = 0;
    bal::VariablesArray data;
    bool replace_existing = false;
} CGenVariableInfo;

inline bool is_binary_variable_name(const std::string& value) {
    return (!value.empty() && std::isdigit(*value.begin()));
}

typedef std::map<std::string, CGenVariableInfo> CGenVariablesMap;

inline const char* const get_formula_type_title(const CGenFormulaType value) {
    switch(value) {
        case ftCnf:
            return "CNF";
        case ftAnf:
            return "ANF";
    };
};

inline const char* const get_output_format_title(const CGenOutputFormat value) {
    switch(value) {
        case ofAnfPolybori:
            return "ANF Polybori";
        case ofCnfDimacs:
            return "DIMACS CNF";
        case ofCnfVIGGraphML:
            return "CNF VIG GraphML";
        case ofCnfWeightedVIGGraphML:
            return "CNF weighted VIG GraphML";
        case ofCnfVIGGEXF:
            return "CNF VIG GEXF";
    };
};

#endif /* shared_hpp */
