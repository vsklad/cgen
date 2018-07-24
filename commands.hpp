//
//  CGen
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef commands_hpp
#define commands_hpp

#include "shared.hpp"

void encode_anf(const CGenAlgorithm algorithm, const uint32_t rounds,
                const CGenVariablesMap& variables_map, const bool normalize_variables,
                const char* const output_file_name);

void encode_cnf(const CGenAlgorithm algorithm, const uint32_t rounds,
                const CGenVariablesMap& variables_map, const bool normalize_variables,
                const uint32_t add_max_args, const uint32_t xor_max_args,
                const char* const output_file_name);

void assign(CGenVariablesMap& variables_map, const bool normalize_variables,
            const char* const input_file_name, const char* const output_file_name);

#endif /* commands_hpp */
