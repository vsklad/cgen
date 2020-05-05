//
//  CGen
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef commands_hpp
#define commands_hpp

#include "formula.hpp"
#include "shared.hpp"

void encode_anf(const CGenAlgorithm algorithm, const uint32_t rounds,
                CGenVariablesMap& variables_map,
                const uint32_t add_max_args, const uint32_t xor_max_args,
                const char* const output_file_name,
                const CGenOutputFormat output_format, const CGenTraceFormat trace_format,
                const bool b_reindex_variables, const bool b_normalize_variables,
                const bool b_assign_after_encoding, const bal::FormulaProcessingMode mode);

void encode_cnf(const CGenAlgorithm algorithm, const uint32_t rounds,
                CGenVariablesMap& variables_map,
                const uint32_t add_max_args, const uint32_t xor_max_args,
                const char* const output_file_name,
                const CGenOutputFormat output_format, const CGenTraceFormat trace_format,
                const bool b_reindex_variables, const bool b_normalize_variables,
                const bool b_assign_after_encoding, const bal::FormulaProcessingMode mode);

void process_anf(CGenVariablesMap& variables_map,
                 const char* const input_file_name, const char* const output_file_name,
                 const CGenOutputFormat output_format, const CGenTraceFormat trace_format,
                 const bool b_reindex_variables, const bool b_normalize_variables,
                 const bal::FormulaProcessingMode mode);

void process_cnf(CGenVariablesMap& variables_map,
                 const char* const input_file_name, const char* const output_file_name,
                 const CGenOutputFormat output_format, const CGenTraceFormat trace_format,
                 const bool b_reindex_variables, const bool b_normalize_variables,
                 const bal::FormulaProcessingMode mode);

#endif /* commands_hpp */
