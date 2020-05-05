//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include "cnfsubsumption.hpp"

namespace bal {
    
    // check for subsumption within clause_index_
    inline processor_result_t CnfSubsumptionOptimizer::process_clause_subsumption(uint32_t* const p_clause) {
        const clause_size_t clause_size = _clause_size(p_clause);
        switch(clause_size) {
            case 2:
                return process_clause_subsumption_ca<2>(p_clause);
            case 3:
                return process_clause_subsumption_ca<3>(p_clause);
            case 4:
                return process_clause_subsumption_ca<4>(p_clause);
            default:
                return erUndetermined;
        };
    };
    
    template<clause_size_t size1, clause_size_t size2, clause_size_t l0_index, clause_size_t l1_index, clause_size_t l2_index = size2>
    inline void __process_clause_subsumption_caca(const uint32_t* const p_ca1, uint32_t* const p_ca2) {
        clause_flags_t original_flags = _clause_flags(p_ca2);
        clause_flags_t subsumed_flags = _clause_flags(p_ca1);
        caca_expand_flags<size1, size2, l0_index, l1_index, l2_index>(subsumed_flags);
        if ((original_flags & ~subsumed_flags) == 0) {
            _clause_exclude(p_ca2);
        } else if ((original_flags & subsumed_flags) != 0) {
            _clause_flags_set(p_ca2, original_flags & ~subsumed_flags);
        };
    };
    
    template<clause_size_t ca_size>
    inline processor_result_t CnfSubsumptionOptimizer::process_clause_subsumption_ca(uint32_t* const p_clause) {
        static_assert(_clause_size_is_aggregated(ca_size), "invalid clause size");
        _assert_level_0(ca_size == _clause_size(p_clause));
        
        CnfProcessor::clauses_iterator_t<ca_size> iterator(clauses_index_, *this);
        container_offset_t ca_variables[ca_size];
        for (auto i = 0; i < ca_size; i++) {
            ca_variables[i] = literal_t__variable_id(_clause_literal(p_clause, i));
        };
        container_offset_t ci_offset = iterator.first(ca_variables);
        while (ci_offset != CLAUSES_END) {
            uint32_t* p_ci = _clauses_offset_clause(clauses_data_, ci_offset);
            const clause_size_t ci_size = _clause_size(p_ci);
            if (ca_size == 2) {
                if (iterator.instance_bits == 0b11) {
                    if (ci_size == 3) {
                        if (_clause_literal(p_clause, 0) == _clause_literal(p_ci, 1)) {
                            __process_clause_subsumption_caca<2, 3, 1, 2>(p_clause, p_ci);
                        } else if (_clause_literal(p_clause, 1) == _clause_literal(p_ci, 1)) {
                            __process_clause_subsumption_caca<2, 3, 0, 1>(p_clause, p_ci);
                        } else {
                            __process_clause_subsumption_caca<2, 3, 0, 2>(p_clause, p_ci);
                        };
                    } else if (ci_size == 4) {
                        if (_clause_literal(p_clause, 0) == _clause_literal(p_ci, 2)) {
                            __process_clause_subsumption_caca<2, 4, 2, 3>(p_clause, p_ci);
                        } else if (_clause_literal(p_clause, 1) == _clause_literal(p_ci, 1)) {
                            __process_clause_subsumption_caca<2, 4, 0, 1>(p_clause, p_ci);
                        } else if (_clause_literal(p_clause, 0) == _clause_literal(p_ci, 0)) {
                            if (_clause_literal(p_clause, 1) == _clause_literal(p_ci, 2)) {
                                __process_clause_subsumption_caca<2, 4, 0, 2>(p_clause, p_ci);
                            } else {
                                __process_clause_subsumption_caca<2, 4, 0, 3>(p_clause, p_ci);
                            };
                        } else {
                            if (_clause_literal(p_clause, 1) == _clause_literal(p_ci, 2)) {
                                __process_clause_subsumption_caca<2, 4, 1, 2>(p_clause, p_ci);
                            } else {
                                __process_clause_subsumption_caca<2, 4, 1, 3>(p_clause, p_ci);
                            };
                        };
                    };
                };
            } else if (ca_size == 3) {
                if (ci_size == 2) {
                    switch(iterator.instance_bits) {
                        case 0b0011:
                            __process_clause_subsumption_caca<2, 3, 0, 1>(p_ci, p_clause);
                            break;
                        case 0b0101:
                            __process_clause_subsumption_caca<2, 3, 0, 2>(p_ci, p_clause);
                            break;
                        case 0b0110:
                            __process_clause_subsumption_caca<2, 3, 1, 2>(p_ci, p_clause);
                            break;
                    };
                } else if (ci_size == 4 && iterator.instance_bits == 0b0111) {
                    if (_clause_literal(p_clause, 0) == _clause_literal(p_ci, 1)) {
                        __process_clause_subsumption_caca<3, 4, 1, 2, 3>(p_clause, p_ci);
                    } else if (_clause_literal(p_clause, 2) == _clause_literal(p_ci, 2)) {
                        __process_clause_subsumption_caca<3, 4, 0, 1, 2>(p_clause, p_ci);
                    } else if (_clause_literal(p_clause, 1) == _clause_literal(p_ci, 1)) {
                        __process_clause_subsumption_caca<3, 4, 0, 1, 3>(p_clause, p_ci);
                    } else {
                        _assert_level_3(_clause_literal(p_clause, 1) == _clause_literal(p_ci, 2));
                        __process_clause_subsumption_caca<3, 4, 0, 2, 3>(p_clause, p_ci);
                    };
                };
            } else if (ca_size == 4) {
                if (ci_size == 2) {
                    switch(iterator.instance_bits) {
                        case 0b0011:
                            __process_clause_subsumption_caca<2, 4, 0, 1>(p_ci, p_clause);
                            break;
                        case 0b0101:
                            __process_clause_subsumption_caca<2, 4, 0, 2>(p_ci, p_clause);
                            break;
                        case 0b0110:
                            __process_clause_subsumption_caca<2, 4, 1, 2>(p_ci, p_clause);
                            break;
                        case 0b1001:
                            __process_clause_subsumption_caca<2, 4, 0, 3>(p_ci, p_clause);
                            break;
                        case 0b1010:
                            __process_clause_subsumption_caca<2, 4, 1, 3>(p_ci, p_clause);
                            break;
                        case 0b1100:
                            __process_clause_subsumption_caca<2, 4, 2, 3>(p_ci, p_clause);
                            break;
                    };
                } else if (ci_size == 3) {
                    switch(iterator.instance_bits) {
                        case 0b0111:
                            __process_clause_subsumption_caca<3, 4, 0, 1, 2>(p_ci, p_clause);
                            break;
                        case 0b1011:
                            __process_clause_subsumption_caca<3, 4, 0, 1, 3>(p_ci, p_clause);
                            break;
                        case 0b1101:
                            __process_clause_subsumption_caca<3, 4, 0, 2, 3>(p_ci, p_clause);
                            break;
                        case 0b1110:
                            __process_clause_subsumption_caca<3, 4, 1, 2, 3>(p_ci, p_clause);
                            break;
                    };
                };
            };
            
            if (_clause_is_included(p_clause)) {
                ci_offset = iterator.next();
            } else {
                break;
            };
        };
        
        return erUndetermined;
    };
    
    bool CnfSubsumptionOptimizer::execute() {
        process_clauses<CnfSubsumptionOptimizer, &CnfSubsumptionOptimizer::process_clause_subsumption>(this);
        return true;
    };
    
};
