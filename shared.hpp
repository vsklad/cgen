//
//  CNFGen
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef shared_hpp
#define shared_hpp

#include <map>
#include <vector>
#include "variablesarray.hpp"

#define APP_VERSION "1.0"
#define APP_TITLE "CNFGen"
#define APP_URL "https://cnfgen.sophisticatedways.net"
#define APP_DESCRIPTION "CNFGen is a tool for encoding SHA-1 and SHA-256 hash functions into CNF in DIMACS format"

#define APP_USAGE_SHORT "\
Usage: \n\
    cnfgen encode (SHA1|SHA256) [-r <rounds>] [-v <name> <value>]... [<encoder options>] <output file name>\n\
    cnfgen (assign | define) [-v <name> <value>]... <input file name> <output file name>\n\
    cnfgen --help\n\
    cnfgen --version\n\
";

#define APP_USAGE_LONG "\
Commands:\n\
    encode (SHA1|SHA256) - generate the encoding\n\
    assign - read <input file name>, assign variables as specified and save it to <output file name>\n\
    define - define one or more named variables\n\
Options:\n\
    -v <name> <value> - specification of the named variable,\n\
        its mapping to binary variables and/or its constant values\n\
        refer for detailed specifications online\n\
    -r <value> - number of SHA1 rounds to encode\n\
    -h | --help\n\
    --version\n\
Further documentation and usage examples available at https://cnfgen.sophisticatedways.net.\n\
";

#define ERROR_COMMAND_NONE "No command specified"
#define ERROR_UNKNOWN_OPTION "unknown option"
#define ERROR_UNKNOWN_ARGUMENT "unknown argument"
#define ERROR_COMMAND_LINE_PARSE "Command line parsing error"
#define ERROR_INVALID_ARGUMENT "Error"
#define ERROR_MISSING_ALGORITHM "Algorithm for encoding not specified"
#define ERROR_UNKNOWN_ALGORITHM "Unknown algorithm for encoding"
#define ERROR_INVALID_VARIABLE_NAME "Invalid variable name"
#define ERROR_MISSING_VARIABLE_VALUE "Variable value is missing"
#define ERROR_DUPLICATED_VARIABLE_NAME "Variable may only appear once on the options list"
#define ERROR_NO_VARIABLES "No variables specified"
#define ERROR_EXCEPT_MUST_FOLLOW_ASSIGN_ENCODE "\"except\" variable modifiers are only allowed for\"assign\" or \"encode\" commands"
#define ERROR_EXCEPT_PAD_MUST_FOLLOW_CONSTANT "\"except\" and \"pad\" variable modifiers must follow its constant assignment"
#define ERROR_RANGE_FIRST_LAST "For binary variables range the first element must be less or equal to the last"
#define ERROR_RANGE_FIRST_ZERO "For binary variables range the first element must be greater than 0"
#define ERROR_PAD_UNKNOWN_VALUE "\"pad\" variable modifier only supports \"SHA1\" and \"SHA256\" values"
#define ERROR_R_MUST_FOLLOW_ENCODE "Rounds option can only be specified after \"encode\""
#define ERROR_ADD_MAX_ARGS_MUST_FOLLOW_ENCODE "add_max_args option can only be specified after \"encode\""
#define ERROR_XOR_MAX_ARGS_MUST_FOLLOW_ENCODE "xor_max_args option can only be specified after \"encode\""
#define ERROR_V_MUST_FOLLOW_ASSIGN_ENCODE_DEFINE "Variable options can only be specified after \"assign\", \"encode\" or \"define\""
#define ERROR_MISSING_INPUT_FILE_NAME "Input file name is not specified"
#define ERROR_MISSING_OUTPUT_FILE_NAME "Output file name is not specified"
#define ERROR_ROUNDS_RANGE "Rounds number is out of range"
#define ERROR_ADD_MAX_ARGS_RANGE "add_max_args is out of range, must be between 2 and 6"
#define ERROR_XOR_MAX_ARGS_RANGE "add_max_args is out of range, must be between 2 and 10"
#define ERROR_ENCODE_UNKNOWN_VARIABLE_NAME "Unknown variable name, \"encode\" only expects \"M\" and \"H\" case sensitive"
#define ERROR_UNKNOWN_VARIABLE_NAME "named variable is not defined in the DIMACS file"
#define ERROR_VARIABLE_ALREADY_DEFINED " named variable already defined in the DIMACS file"
#define ERROR_ASSIGN_VARIABLE_VALUE_TOO_LONG "Named variable value is too long for its definiton in the DIMACS file"
#define ERROR_ASSIGN_VARIABLE_CONSTANT_MISMATCH "Variable is already assigned to a conflicting binary constant"
#define ERROR_NAMED_VARIABLE_OUT_OF_RANGE " named variable includes a binary variable number greated than than the DIMACS file allows"
#define ERROR_COMPUTE_HASH_NO_MESSAGE "Computing hash requires full message value specified"
#define ERROR_EXCEPT_VARIABLES_RANGE_OUT_OF_BOUNDS "\"except\" variables range is out of bounds"
#define ERROR_RANDOM_NOT_ENOUGH_VARIABLES "Less variables than the requested number of random values to set"
#define ERROR_FAILED_OPENING_OUTPUT_FILE "Failed to open the output file"
#define WARNING_CNF_IS_EMPTY "The formula is SATISFIED, no clauses in DIMACS file"

enum CnfGenCommand {cmdNone, cmdEncode, cmdAssign, cmdDefine, cmdHelp, cmdVersion};
enum CnfGenAlgorithm {algNone, algSHA1, algSHA256};

enum CnfGenVariableMode {vmValue, vmRandom, vmCompute, vmDefine};

typedef struct {
    CnfGenVariableMode mode;
    uint32_t var_range_first = 0;
    uint32_t var_range_last = 0;
    uint32_t random_count = 0;
    ple::VariablesArray data;
} CnfGenVariableInfo;

typedef std::map<std::string, CnfGenVariableInfo> CnfGenVariablesMap;

#endif /* shared_hpp */
