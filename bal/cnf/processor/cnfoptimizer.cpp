//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include <iomanip>
#include <iostream>
#include "assertlevels.hpp"
#include "cnfoptimizer.hpp"

#include "cnfnativetracer.hpp"

namespace bal {
 
    /////////////////////////////
    //  Utilities
    /////////////////////////////
    
    inline void __reduce_clause_flags(uint16_t& flags, const clause_size_t index, const literalid_t value) {
        switch ((index << 1) + value) {
            case 0: // variable 0, value 0
                flags =
                (flags & 0x0002) >> 1 | (flags & 0x0008) >> 2 |
                (flags & 0x0020) >> 3 | (flags & 0x0080) >> 4 |
                (flags & 0x0200) >> 5 | (flags & 0x0800) >> 6 |
                (flags & 0x2000) >> 7 | (flags & 0x8000) >> 8;
                break;
            case 1: // variable 0, value 1
                flags =
                (flags & 0x0001)      | (flags & 0x0004) >> 1 |
                (flags & 0x0010) >> 2 | (flags & 0x0040) >> 3 |
                (flags & 0x0100) >> 4 | (flags & 0x0400) >> 5 |
                (flags & 0x1000) >> 6 | (flags & 0x4000) >> 7;
                break;
            case 2: // variable 1, value 0
                flags =
                (flags & 0x000C) >> 2 | (flags & 0x00C0) >> 4 |
                (flags & 0x0C00) >> 6 | (flags & 0xC000) >> 8;
                break;
            case 3: // variable 1, value 1
                flags =
                (flags & 0x0003)      | (flags & 0x0030) >> 2 |
                (flags & 0x0300) >> 4 | (flags & 0x3000) >> 6;
                break;
            case 4: // variable 2, value 0
                flags = (flags & 0x00F0) >> 4 | (flags & 0xF000) >> 8;
                break;
            case 5: // variable 2, value 1
                flags = (flags & 0x000F)      | (flags & 0x0F00) >> 4;
                break;
            case 6: // variable 3, value 0
                flags = (flags & 0xFFFF) >> 8;
                break;
            case 7: // variable 3, value 1
                flags = (flags & 0x00FF);
                break;
        };
    };

    /////////////////////////////
    //  Tracer
    /////////////////////////////
    
#if defined(CNF_TRACE)
    
    Ref<CnfTracer> p_tracer_;
    
    void set_cnf_tracer(Ref<CnfTracer> p_tracer) {
        // new tracer can only be assigned if there is no existing one
        _assert_level_1(p_tracer_ == nullptr || p_tracer == nullptr);
        p_tracer_ = p_tracer;
    };
    
    // record variables assigned at the start of the trace
    #define TRACE_START if (p_tracer_ != nullptr) {\
        p_tracer_->start(cnf_);\
        for (auto i = 0; i < variables_.size(); i++) {\
            if (variables_.data()[i] != variable_t__literal_id(i)) {\
                p_tracer_->assign_variable(i, variables_.data()[i]);\
            };\
        };\
    };
    
    #define TRACE_FINISH if (p_tracer_ != nullptr) p_tracer_->finish();
    #define TRACE_LEVEL_NEXT if (p_tracer_ != nullptr) p_tracer_->level_next();
    #define TRACE_LEVEL_PREV if (p_tracer_ != nullptr) p_tracer_->level_prev();
    #define TRACE_CLAUSE(p_clause, offset) if (p_tracer_ != nullptr) p_tracer_->process_clause((p_clause), (offset), false);
    #define TRACE_CLAUSE_RESOLVENT(p_clause) if (p_tracer_ != nullptr) p_tracer_->process_clause((p_clause), CONTAINER_END, true);
    #define TRACE_CLAUSE_APPEND(p_clause) if (p_tracer_ != nullptr) p_tracer_->append_clause((p_clause));
    #define TRACE_CLAUSE_MERGE(p_clause, offset) if (p_tracer_ != nullptr) p_tracer_->merge_clause((p_clause), (offset));
    #define TRACE_CLAUSE_UPDATE(p_clause, offset) if (p_tracer_ != nullptr) p_tracer_->process_clause((p_clause), (offset), true);
    #define TRACE_CLAUSE_REMOVE(offset) if (p_tracer_ != nullptr) p_tracer_->remove_clause((offset));
    #define TRACE_VARIABLE_ASSIGNMENT(variable_id, value) if (p_tracer_ != nullptr) p_tracer_->assign_variable((variable_id), (value));
#else
    #define TRACE_START
    #define TRACE_FINISH
    #define TRACE_LEVEL_NEXT
    #define TRACE_LEVEL_PREV
    #define TRACE_CLAUSE(p_clause, offset)
    #define TRACE_CLAUSE_RESOLVENT(p_clause)
    #define TRACE_CLAUSE_APPEND(p_clause)
    #define TRACE_CLAUSE_MERGE(p_clause, offset)
    #define TRACE_CLAUSE_UPDATE(p_clause, offset)
    #define TRACE_CLAUSE_REMOVE(offset)
    #define TRACE_VARIABLE_ASSIGNMENT(variable_id, value)
#endif
    
#define TR_LOG_CLAUSE(action, p_clause)
    
    /////////////////////////////
    //  CnfOptimizer
    /////////////////////////////
    
    inline void CnfOptimizer::exclude_clause(const container_offset_t offset) {
#ifdef CNF_TRACE
        // it is possible that the clause is excluded for the second time due to recursive evaluation (?)
        if (p_tracer_ != nullptr && _clauses_offset_is_included(clauses_data_, offset)) {
            TRACE_CLAUSE_REMOVE(offset);
        };
#endif
        _clauses_offset_exclude(clauses_data_, offset);
    };
    
    inline processor_result_t CnfOptimizer::process_clause_evaluate(uint32_t* const p_clause) {
        
        if ((evaluations_ & 0x3FFF) == 0) {
             std::cout << "s:" << std::dec << std::setfill(' ') << std::setw(8) << (clauses_size_ >> 10) << " Kb d: " << std::setw(8) << ((clauses_size_ - processed_offset_) >> 10) << "Kb v: " << std::setw(8) << variables_assigned_ << std::flush << "\r";
        };
        
        _assert_level_3(p_clause == _clauses_offset_clause(clauses_data_, processed_offset_));
        const processor_result_t result = evaluate_clause(processed_offset_);
        if (result == erConflict) {
            __print_conflict(variables_.data(), p_clause);
        };
        return result;
    };

    // process clauses one by one
    // the list will grow after each optimization
    // therefore the process will stop when consequences of all optimizations are evaluated
    inline processor_result_t CnfOptimizer::evaluate_clauses() {
        evaluations_ = 0;
        evaluations_aggregated_ = 0;
        variables_assigned_ = 0;
        
        const processor_result_t result = process_clauses<CnfOptimizer, &CnfOptimizer::process_clause_evaluate>(this);
        
        std::cout << "Evaluation: " << std::dec << evaluations_ << "/" << evaluations_aggregated_ << " cls, size: ";
        std::cout << cnf_.clauses_size() << "/" << cnf_.clauses_size<0, true>() << " cls, ";
        std::cout << (cnf_.memory_size() >> 10) << "/";
        std::cout << (clauses_index_.memory_size() >> 10) << " Kb";
        std::cout << std::endl;
        
        return result;
    };
    
    // fix variable values for unary clauses, eliminate them
    // exclude clause before further analysis to avoid recursive loop
    // only update indexes if:
    //   clause changes - a literal change or a partial assignment
    // do not update variable indexes if:
    //   variable becomes or is constant
    inline processor_result_t CnfOptimizer::evaluate_clause(const uint32_t offset) {
        evaluations_++;
        
        processor_result_t result = erUndetermined;
        uint32_t* p_clause = _clauses_offset_clause(clauses_data_, offset);
        uint32_t dst[_clause_memory_size(p_clause)];
        
        // copy literals checking for constants and changes
        // if an aggregate, reduce/ensure literals are uncomplemented
        TRACE_CLAUSE(p_clause, offset);
        if (_clause_is_aggregated(p_clause)) {
            result = normalize_ca(p_clause, dst);
        } else {
            result = normalize_cu(p_clause, dst);
        };
        
        if (result == erUndetermined) {
            if (_clause_is_aggregated(dst)) {
                // recursive update may assign valiables so that subsequent evaluation results into this
                // need to avoid ideally; for that, separate normalization for assign_literal is needed
                _assert_level_1(offset == processed_offset_);
                TRACE_CLAUSE(dst, offset);
                result = evaluate_clause_a(dst, offset, offset);
            };
        } else if (result == erChangedC || result == erChangedV) {
            if (_clause_is_aggregated(dst)) {
                container_offset_t original_offset = CONTAINER_END;
                if (offset == processed_offset_
                    ) {
                    original_offset = offset;
                };
                // no transitive closure if the clause is changed
                // instead, the derived clause is saved and the closure is produced later
                exclude_clause(offset);
                TRACE_CLAUSE_RESOLVENT(dst);
                result = evaluate_clause_a(dst, CONTAINER_END,  original_offset);
            } else {
                Cnf::insertion_point_t insertion_point;
                __insertion_point_t_init(insertion_point);
                if (clauses_.transaction_offset_is_immutable(offset) || (offset < processed_offset_ && result == erChangedV)) {
                    exclude_clause(offset);
                    clauses_.append<false>(dst, insertion_point);
                    TRACE_CLAUSE_APPEND(dst);
                } else {
                    clauses_.find(dst, insertion_point);
                    if (insertion_point.container_offset == CONTAINER_END) {
                        TRACE_CLAUSE_REMOVE(offset);
                        // copying must occur afterwards to indentify old clause
                        clauses_.update(offset, insertion_point);
                        _assert_level_1(_clause_is_included(dst));
                        _clause_copy(dst, _clauses_offset_clause(clauses_data_, offset));
                        TRACE_CLAUSE_APPEND(dst);
                    } else {
                        exclude_clause(offset);
                        TRACE_LEVEL_NEXT;
                        TRACE_CLAUSE_RESOLVENT(dst);
                        TRACE_LEVEL_PREV;
                    };
                };
            };
        } else if (result == erSatisfied) {
            exclude_clause(offset);
        };
        
        return result;
    };
    
#ifdef CNF_TRACE
#define __TRACE_ASSIGN_LITERAL \
    if (p_tracer_ != nullptr) { \
        if (literal_t__is_negation(original_value)) \
            p_tracer_->assign_literal(literal_t__negated(original_value), literal_t__negated(new_value)); \
        else \
            p_tracer_->assign_literal(original_value, new_value); \
    };
#else
    #define __TRACE_ASSIGN_LITERAL
#endif
    
    // accepts normalization of self
    // returns erUndetermined if not changed
    template<bool track_changes>
    inline processor_result_t CnfOptimizer::normalize_ca(const uint32_t* const p_src, uint32_t* const p_dst) const {
        processor_result_t result = erUndetermined;
        const clause_size_t clause_size = _clause_size(p_src);
        const literalid_t* const literals = _clause_literals(p_src);
        uint16_t flags = _clause_flags(p_src);
        const literalid_t* const variable_values = variables_.data();
        
        literalid_t* new_literals = _clause_literals(p_dst);
        clause_size_t new_clause_size = 0;
        
        _assert_level_0(_clause_size_is_aggregated(clause_size));
        
        for (auto i = 0; i < clause_size && flags != 0; i++) {
            const literalid_t original_value = literals[i]; // because p_src may == p_dst
            _assert_level_0(literal_t__is_variable(original_value) && !literal_t__is_negation(original_value));
            literalid_t new_value = literal_t::resolve(variable_values, original_value);
            
            if (literal_t__is_variable(new_value)) {
                if (literal_t__is_negation(new_value)) {
                    __TRACE_ASSIGN_LITERAL;
                    ca_flags_negate(flags, new_clause_size);
                    literal_t__unnegate(new_value);
                    result = erChangedV;
                } else if (new_value != original_value) {
                    __TRACE_ASSIGN_LITERAL;
                    result = erChangedV;
                };
                new_literals[new_clause_size] = new_value;
                ca_insert_last_literal_sorted(flags, new_clause_size, new_literals);
            } else { // is_constant
                __TRACE_ASSIGN_LITERAL;
                new_literals[new_clause_size] = new_value;
                __reduce_clause_flags(flags, new_clause_size, new_value);
                if (result == erUndetermined) {
                    result = erChangedC;
                };
            };
        };
        // save new clause size and flags
        _clause_header_set(p_dst, flags, new_clause_size);
        return (flags != 0) ? result : erSatisfied;
    };
    
    inline processor_result_t CnfOptimizer::normalize_cu(const uint32_t* const p_src, uint32_t* const p_dst) const {
        processor_result_t result = erUndetermined;
        
        const literalid_t* literals = _clause_literals(p_src);
        const clause_size_t clause_size = _clause_size(p_src);
        const literalid_t* const var_values = variables_.data();
            
        // an unaggregated clause indeed
        _assert_level_1(!_clause_size_is_aggregated(clause_size));
        
        literalid_t* new_literals = _clause_literals(p_dst);
        clause_size_t new_clause_size = 0;
        
        for (auto i = 0; i < clause_size; i++) {
            const literalid_t original_value = literals[i]; // because p_src may == p_dst
            const literalid_t new_value = literal_t::resolve(var_values, original_value);
            // count resolved literal if not a constant
            if (literal_t__is_variable(new_value)) {
                if (new_value != original_value) {
                    __TRACE_ASSIGN_LITERAL;
                    if (result == erUndetermined || result == erChangedC) {
                        result = erChangedV;
                    };
                };
                // insert into the sorted sequence
                auto j = new_clause_size;
                while (j > 0) {
                    if (new_value >= new_literals[j - 1]) {
                        new_literals[j] = new_value;
                        break;
                    } else {
                        new_literals[j] = new_literals[j - 1];
                        j--;
                    };
                };
                if (j == 0) {
                    new_literals[0] = new_value;
                    new_clause_size++;
                } else if (new_value == new_literals[j - 1]) {
                    // check duplicate; should be rare
                    for (auto k = j; k < new_clause_size; k++) {
                        new_literals[k] = new_literals[k + 1];
                    };
                } else {
                    // check presence of a variable together with its negation
                    if (literal_t__is_same_variable(new_value, new_literals[j - 1])) {
                        result = erSatisfied;
                        break;
                    };
                    new_clause_size++;
                };
                // check presence of a variable together with its negation
                if (j < new_clause_size - 1 && literal_t__is_same_variable(new_value, new_literals[j + 1])) {
                    result = erSatisfied;
                    break;
                };
            } else if (literal_t__is_constant_1(new_value)) {
                result = erSatisfied;
                break;
            } else {
                // a literal is 0; the clause is shortened
                __TRACE_ASSIGN_LITERAL;
                if (result == erUndetermined) {
                    result = erChangedC;
                };
            };
        };
        
        if (result != erSatisfied) {
            if (_clause_size_is_aggregated(new_clause_size)) {
                // the clause has become aggregated; unnegate literals and build flags
                uint16_t flags = 0x1;
                for (auto i = 0; i < new_clause_size; i++) {
                    if (literal_t__is_negation(new_literals[i])) {
                        literal_t__unnegate(new_literals[i]);
                    } else {
                        flags <<= (0x1 << i);
                    };
                };
                _clause_header_set(p_dst, flags, new_clause_size);
            } else {
                _clause_header_set(p_dst, 0, new_clause_size);
            }
        } else {
            _clause_header_set(p_dst, 0, 0);
        };
    
        return result;
    };

    // the primary method for simplifying the clause and producing resolvents
    // p_clause - a normalized aggregated clause to be evaluated
    // offset - offset of the clause itself or an equivalent clause to be merged with
    // original_offset - offset the modified clause can be stored at; CONTAINER_END means append
    // returns erUndetermined if p_clause is not changed
    // returns erChanged if p_clause is changed in any way
    // returns erSatisfied if p_clause is satisfied
    // Note 1
    //   assume transitive closure does not need updating when update_offset < processed_offset_
    //   because it is only done when a constant is assigned
    //   which means all resolvents would be created from existing clauses anyway
    inline processor_result_t CnfOptimizer::evaluate_clause_a(uint32_t* const p_clause,
                                                               container_offset_t offset,
                                                               const container_offset_t original_offset) {
        // the clause which originated the current one after normalization
        _assert_level_2(original_offset <= processed_offset_ || original_offset == CONTAINER_END);
        
        // offset that the resulting clause can be stored to
        const container_offset_t update_offset = clauses_.transaction_offset_is_immutable(original_offset) ? CONTAINER_END : original_offset;
        
        // the clause may be updated several times
        // the processing stops when the last iteration results in erUndetermined
        // however, the returned value reflects preceding change
        processor_result_t result = erUndetermined;
        
        // repeat processing for as long as the clause is changed, at least once
        while(true) {
            evaluations_aggregated_++;
            
            const clause_size_t clause_size = _clause_size(p_clause);
            if (clause_size > 1) {
                // "processed" clause flags from offset clause
                // 0 for first time processing, actual for processed clauses
                // referring to the baseline for already generated subsuming clauses;
                // it is used for merging and deriving of reduced clauses to only process the changes
                clause_flags_t processed_flags = offset < processed_offset_ ? _clauses_offset_flags(clauses_data_, offset) : 0;
                
                // insertion point stores index position when a clause is searched for
                // it will be used if thenew clause is appended
                Cnf::clauses_container_t::insertion_point_t insertion_point;
                __insertion_point_t_init(insertion_point);
                
                if (offset == CONTAINER_END) {
                    // find existing matching clause before evaluation
                    // note that the result may be beyond processed_offset_ and that
                    // it can be equal to processed_offset_ while evaluating recursively
                    clauses_.find(p_clause, insertion_point);
                    offset = insertion_point.container_offset;
                    if (offset < processed_offset_) {
                        processed_flags = _clauses_offset_flags(clauses_data_, offset);
                    };
                };
                
                // there is offset but not the current clause
                if (offset != CONTAINER_END && (original_offset != processed_offset_ || offset != original_offset)) {
                    const clause_flags_t offset_flags = _clauses_offset_flags(clauses_data_, offset);
                    _assert_level_1(offset_flags);
                    _clause_flags_include(p_clause, offset_flags);
                    if (_clause_flags(p_clause) != offset_flags) {
                        TRACE_CLAUSE_MERGE(p_clause, offset);
                        if (clauses_.transaction_offset_is_immutable(offset)) {
                            // once changed, an immutable clause will be appended afresh
                            // the original clause must be excluded
                            exclude_clause(offset);
                            clauses_.insertion_point_from_container_offset(insertion_point, offset);
                            offset = CONTAINER_END;
                            TRACE_CLAUSE_UPDATE(p_clause, offset);
                        } else if (!is_clause_included(offset)) {
                            _assert_level_1(offset != processed_offset_); // unelaborated and untested
                            if (offset < processed_offset_) {
                                // if the clause is processed, treat is as immutable
                                clauses_.insertion_point_from_container_offset(insertion_point, offset);
                                offset = CONTAINER_END;
                                TRACE_CLAUSE_UPDATE(p_clause, offset);
                            } else if (offset > processed_offset_) {
                                // this essentially re-includes the clause
                                // which is fine assuming the clause is not included into variable clauses index yet
                                _assert_level_1(_clause_is_included(p_clause));
                                _clauses_offset_header(clauses_data_, offset) = _clause_header(p_clause);
                                TRACE_CLAUSE_UPDATE(p_clause, offset);
                                TRACE_CLAUSE_APPEND(p_clause);
                            };
                        } else if (offset > processed_offset_ && update_offset != CONTAINER_END) {
                            // merge the unprocessed clause into the current one
                            exclude_clause(offset);
                            clauses_.insertion_point_from_container_offset(insertion_point, offset);
                            offset = CONTAINER_END;
                            TRACE_CLAUSE_UPDATE(p_clause, offset);
                        } else {
                            // must be updated immediately for cascade updates
                            // and even if excluded, in case it is included again
                            _clauses_offset_flags_include(clauses_data_, offset, _clause_flags(p_clause));
                            TRACE_CLAUSE_UPDATE(p_clause, offset);
                        };
                    } else {
                        // p_clause is fully merged into an existing one
                        break;
                    };
                };
                
                processor_result_t iteration_result = erUndetermined;
                const clause_flags_t flags = _clause_flags(p_clause);
                // there is something to process
                _assert_level_1((flags & ~processed_flags) != 0);
                // ensure processed flags is a subset (merged)
                _assert_level_1((~flags & processed_flags) == 0);
                
                if (clause_size == 2) {
                    
                    if (_c2_is_single_clause_flags(flags)) {
                        // a single clause cannot be reprocessed again since it cannot change
                        // this also means an immutable clause will not be re-insserted
                        // clause is ignored if offset is unprocessed
                        _assert_level_1(offset >= processed_offset_);
                        if ((original_offset == processed_offset_ && offset == original_offset) ||
                            (update_offset == processed_offset_ && offset == CONTAINER_END)) {
                            processor_result_t resolve_result = resolve_c2(p_clause);
                            _assert_level_3(resolve_result == erSatisfied || resolve_result == erConflict || normalize_ca(p_clause, p_clause) == erUndetermined);
                            iteration_result = (resolve_result == erUndetermined) ? iteration_result : resolve_result;
                        };
                    } else {
                        // the result is always resolved fully
                        if (offset != CONTAINER_END) {
                            exclude_clause(offset);
                            offset = CONTAINER_END;
                        };
                        
                        const literalid_t* const variables = variables_.data();
                        const literalid_t* const literals = _clause_literals(p_clause);
                        
                        if (flags == 0b0110 || flags == 0b1001) {
                            // found full equivalence
                            const literalid_t lhs = variables[literal_t__variable_id(literals[1])];
                            const literalid_t rhs = literal_t__negated_onlyif(literals[0], flags == 0b1001);
                            if (literal_t__is_constant(lhs) && literal_t__is_variable(rhs)) {
                                // TODO: why is this possible????
                                iteration_result = assign_literal_value(rhs, lhs);
                            } else if (lhs != rhs) {
                                iteration_result = assign_literal_value(literals[1], rhs);
                            } else {
                                iteration_result = erSatisfied;
                            };
                        } else if (flags == 0b0101 || flags == 0b1010) {
                            // first variable value is inferred
                            iteration_result = assign_literal_value(literals[0], literal_t__constant(flags == 0b1010));
                        } else if (flags == 0b0011 || flags == 0b1100) {
                            // second variable value is inferred
                            iteration_result = assign_literal_value(literals[1], literal_t__constant(flags == 0b1100));
                        } else if (flags == 0b0111 || flags == 0b1011 || flags == 0b1101 || flags == 0b1110) {
                            // both variable values are inferred
                            // note that the second variable may be resolved while assigning the first one
                            // see notes within assign_literal_value()
                            iteration_result = assign_literal_value(literals[0],
                                                                    literal_t__constant((flags & 0b1010) == 0b1010));
                            if (iteration_result != erConflict) {
                                iteration_result = assign_literal_value(literals[1],
                                                                        literal_t__constant((_clause_flags(p_clause) & 0b1100) == 0b1100));
                            };
                        } else {
                            iteration_result = erConflict;
                        };
                    };
                    
                } else if (clause_size == 3) {
                } else if (clause_size == 4) {
                    _assert_level_3(offset == CONTAINER_END || is_clause_included(offset));
                } else {
                    _assert_level_0(false);
                };
                
                // POST-PROCESSING
                result = (iteration_result == erUndetermined) ? result : iteration_result;
                
                if (iteration_result == erUndetermined && offset == CONTAINER_END) {
                    _assert_level_1(_clause_is_included(p_clause));
                    _assert_level_1(update_offset == CONTAINER_END ||
                                    !_clauses_offset_is_included(clauses_data_, update_offset));
                    // something happened while processing recursively
                    if (!clauses_.insertion_point_is_valid(insertion_point)) {
                        clauses_.find(p_clause, insertion_point);
                        offset = insertion_point.container_offset;
                        if (offset != CONTAINER_END) {
                            _assert_level_1(offset != update_offset);
                            uint32_t* const p_found_clause = _clauses_offset_clause(clauses_data_, offset);
                            const clause_flags_t found_flags = _clause_flags(p_found_clause);
                            
                            if (update_offset < offset) {
                                // use update_offset in any case
                                if (found_flags & ~_clause_flags(p_clause)) {
                                    // reprocess the current clause to merge it
                                    continue;
                                } else {
                                    // offset is replaced by p_clause
                                    exclude_clause(offset);
                                };
                            } else if (offset <= processed_offset_) {
                                if ((~found_flags & _clause_flags(p_clause)) == 0) {
                                    // found processed offset includes p_clause
                                    // simply ignore p_clause and keep offset
                                    break;
                                } else {
                                    // offset clause has something p_clause doesn't
                                    // or p_clause has something offset doesn't
                                    // reprocess; immutable and excluded offset will be handled properly
                                    _assert_level_1(offset != processed_offset_);
                                    continue;
                                };
                            } else {
                                if (_clause_flags(p_clause) & ~found_flags) {
                                    // an unprocessed clause and there are new flags to add
                                    if (clauses_.transaction_offset_is_immutable(offset) ||
                                        !_clause_is_included(p_found_clause)) {
                                        _clause_flags_include(p_clause, found_flags);
                                        exclude_clause(offset);
                                    } else {
                                        _clause_flags_include(p_found_clause, _clause_flags(p_clause));
                                        TRACE_CLAUSE_MERGE(p_clause, offset);
                                        break;
                                    };
                                } else {
                                    // an unprocessed clause, no new flags
                                    // ignore the current clause
                                    // if the found clause is excluded that is for a reason
                                    break;
                                };
                            };
                        };
                        _assert_level_2(offset == insertion_point.container_offset);
                    } else {
                        offset = insertion_point.container_offset;
                        if (offset != CONTAINER_END) {
                            if (_clauses_offset_is_included(clauses_data_, offset)) {
                                if ((_clauses_offset_flags(clauses_data_, offset) & ~_clause_flags(p_clause)) != 0) {
                                    // additional clauses were resolved recursively; reprocess
                                    _assert_level_1(offset != processed_offset_);
                                    continue;
                                };
                            };
                        };
                    };
                    
                    if (update_offset != CONTAINER_END) {
                        if (offset != CONTAINER_END) {
                            if (_clauses_offset_is_included(clauses_data_, offset)) {
                                // additional clauses derived while processing recursively
                                _assert_level_2(offset > processed_offset_);
                                _assert_level_3((_clauses_offset_flags(clauses_data_, offset) & ~_clause_flags(p_clause)) == 0);
                                exclude_clause(offset);
                                offset = CONTAINER_END;
                            };
                        };
                        // copying must occur afterwards to indentify the old clause
                        clauses_.update(update_offset, insertion_point);
                        _assert_level_1(_clause_is_included(p_clause));
                        _clause_copy(p_clause, _clauses_offset_clause(clauses_data_, update_offset));
                        // see note 1
                    } else {
                        clauses_.append<true>(p_clause, insertion_point);
                    };
                    
                    TRACE_CLAUSE_APPEND(p_clause);
                    break;
                } else if (iteration_result == erChangedC || iteration_result == erChangedV) {
                    if (offset != CONTAINER_END) {
                        exclude_clause(offset);
                        offset = CONTAINER_END;
                    };
                    TRACE_CLAUSE_RESOLVENT(p_clause);
                    continue;
                } else if (iteration_result == erSatisfied) {
                    if (offset != CONTAINER_END) {
                        exclude_clause(offset);
                    };
                    break;
                } else {
                    break;
                };
                
            } else if (clause_size == 1) {
                const clause_flags_t flags = _clause_flags(p_clause);
                if (flags == 0b11) {
                    result = erConflict;
                } else {
                    result = assign_literal_value(_clause_literal(p_clause, 0), literal_t__constant(flags == 0b10));
                };
                break;
            } else { // clause_size == 0
                result = erConflict;
                break;
            };
        };
        
        return result;
    };
    
    inline processor_result_t CnfOptimizer::assign_literal_value(const literalid_t literal_id, const literalid_t value) {
        variables_assigned_++;
        _assert_level_1(literal_t__is_variable(literal_id) && !literal_t__is_negation(literal_id));
        
        processor_result_t result = erSatisfied;
        const variableid_t variable_id = literal_t__variable_id(literal_id);
        TRACE_LEVEL_NEXT;
        TRACE_VARIABLE_ASSIGNMENT(variable_id, value);
        TRACE_LEVEL_NEXT;
        
        const literalid_t old_value = variables_.data()[variable_id];
        if (literal_t__is_variable(old_value)) {
            // if variable then the new value must be other than self
            _assert_level_2(!literal_t__is_variable(value) || !literal_t__is_same_variable(old_value, value));
            // can reference preceding variables only
            _assert_level_1(!literal_t__is_variable(value) || literal_t__variable_id(value) <= variable_id);
            // the referenced variable must point to self
            // note the same is fine for old_value since eliminated variables are not updated
            // until post-processing
            _assert_level_1(!literal_t__is_variable(value) ||
                            literal_t__unnegated(value) == variables_.data()[literal_t__variable_id(value)]);
            
            if (variable_id != literal_t__variable_id(old_value)) {
                // a variable is pointing to some other variable already
                // can happen during recursion; resolve the pair of values
                // TPDO: the only known reason for thsi situation is when pair of variables resolves to two values
                // and the second varianble is resolved while assigning the first one;
                // a more optimal solution would be to assign both at once and iterate clauses for both at the same time
                if (literal_t__is_variable(value) && value > old_value) {
                    result = assign_literal_value(literal_t__unnegated(value), literal_t__substitute_literal(value, old_value));
                } else {
                    result = assign_literal_value(literal_t__unnegated(old_value), literal_t__substitute_literal(old_value, value));
                    if (result != erConflict) {
                        // the clauses are handled where the variable was assigned from earlier
                        // all that remains is to update the reference value
                        variables_.data()[variable_id] = value;
                    };
                };
            } else {
                // assign the value immediately to prevent recursive evaluation
                variables_.data()[variable_id] = value;
                
                // go through all clauses that the variable is in
                // exclude the original clause
                // if not satisfied:
                //   remove the old clause from the indexes of all variables
                //   prepare a simplified copy
                //     note this may simplify the flags if an aggregate
                //   append afresh with validation check and merging
                //     note this may be an aggregate
                
                // there is no need to optimize the index since it will not be used again
                CnfProcessor::nonoptimizing_clauses_iterator_t iterator(clauses_index_, *this);
                container_offset_t offset = iterator.first(variable_id);
                while (offset != CONTAINER_END) {
                    if (evaluate_clause(offset) == erConflict) {
                        result = erConflict;
                        break;
                    };
                    offset = iterator.next();
                };
            };
        } else {
            // the same constant can be assigned again while updating indexes recursively
            _assert_level_0(old_value == value);
        };
        TRACE_LEVEL_PREV;
        TRACE_LEVEL_PREV;
        return result;
    };
    
    // returns processing status for the master clause i.e. b_ca_master ? p_ca : p_c2
    // returns erUndetermined in any case where the resolvent is not produced
    template<bool b_ca_master, clause_size_t ca_size, clause_size_t ca_index, clause_size_t c2_index>
    inline processor_result_t CnfOptimizer::resolve_ca_c2(uint32_t* const p_ca, uint32_t* const p_c2, const clause_flags_t ca_flags) {
        static_assert(ca_size >= 2 && ca_size <= 4, "ca_size out of bounds; p_ca must be an aggregated clause");
        static_assert(ca_index >= 0 && ca_index < ca_size, "ca_index out of bounds; p_ca must be an aggregated clause");
        static_assert(c2_index >= 0 && c2_index <= 1, "c2_index out of bounds; p_c2 must be a binary clause");
        _assert_level_0(_clause_size(p_ca) == ca_size);
        _assert_level_0(_clause_size(p_c2) == 2);
        _assert_level_0(_clause_literal(p_ca, ca_index) == _clause_literal(p_c2, c2_index));
        _assert_level_0(ca_flags != 0);
        
        // check that the clauses are unchanged
        const literalid_t* const variables = variables_.data();
        if (b_ca_master) {
            _assert_level_3(_clause_is_literal_unchanged(p_c2, c2_index, variables));
            _assert_level_3(ca_size < 1 || ca_index == 0 || _clause_is_literal_unchanged(p_ca, 0, variables));
            _assert_level_3(ca_size < 2 || ca_index == 1 || _clause_is_literal_unchanged(p_ca, 1, variables));
            _assert_level_3(ca_size < 3 || ca_index == 2 || _clause_is_literal_unchanged(p_ca, 2, variables));
            _assert_level_3(ca_size < 4 || ca_index == 3 || _clause_is_literal_unchanged(p_ca, 3, variables));
        } else {
            _assert_level_3(_clause_is_literal_unchanged(p_c2, 0, variables));
            _assert_level_3(_clause_is_literal_unchanged(p_c2, 0, variables));
            _assert_level_3(_clause_is_literal_unchanged(p_ca, ca_index, variables));
        };
        
        const bool is_clause_unchanged = b_ca_master ? (
            _clause_literal(p_c2, 1 - c2_index) == variables[literal_t__variable_id(_clause_literal(p_c2, 1 - c2_index))]
                                                  ) : (
            (ca_size < 1 || ca_index == 0 || _clause_literal(p_ca, 0) == variables[literal_t__variable_id(_clause_literal(p_ca, 0))]) &&
            (ca_size < 2 || ca_index == 1 || _clause_literal(p_ca, 1) == variables[literal_t__variable_id(_clause_literal(p_ca, 1))]) &&
            (ca_size < 3 || ca_index == 2 || _clause_literal(p_ca, 2) == variables[literal_t__variable_id(_clause_literal(p_ca, 2))]) &&
            (ca_size < 4 || ca_index == 3 || _clause_literal(p_ca, 3) == variables[literal_t__variable_id(_clause_literal(p_ca, 3))])
                                                  );
        
        if (is_clause_unchanged) {
            // determine the resolvent flags
            clause_flags_t resolvent_flags = ca_flags;
            resolve_ca_c2_flags(resolvent_flags, ca_index, c2_index, _clause_flags(p_c2));
            
            if (resolvent_flags != 0) {
                // construct the resolvent
                uint32_t resolvent[_clause_size_memory_size(ca_size)];
                clause_size_t resolvent_size = 1;
                _clause_literal(resolvent, 0) = (ca_index == 0) ? _clause_literal(p_c2, 1 - c2_index) : _clause_literal(p_ca, 0);
                for (auto i = 1; i < ca_size; i++) {
                    const literalid_t literal = (ca_index == i) ? _clause_literal(p_c2, 1 - c2_index) : _clause_literal(p_ca, i);
                    _clause_literal(resolvent, resolvent_size) = literal;
                    if (literal <= _clause_literal(resolvent, resolvent_size - 1)) {
                        ca_insert_last_literal_sorted(resolvent_flags, resolvent_size, _clause_literals(resolvent));
                    } else {
                        resolvent_size++;
                    };
                };
                
                if (resolvent_flags != 0) {
                    _clause_header_set(resolvent, resolvent_flags, resolvent_size);
                    
                    TRACE_LEVEL_NEXT;
                    TRACE_CLAUSE(b_ca_master ? p_c2 : p_ca, CONTAINER_END);
                    TRACE_LEVEL_NEXT;
                    TRACE_CLAUSE_RESOLVENT(resolvent);
                    
                    // with c2 resolution, resolvent_size < ca_size is only possible if both c2 literals are in ca
                    // which means resolution should be handled within the aggregation concept
                    _assert_level_2(resolvent_size == ca_size);
                    _assert_level_3(normalize_ca(resolvent, resolvent) == erUndetermined);
                    
                    processor_result_t result = evaluate_clause_a(resolvent);
                    
                    TRACE_LEVEL_PREV;
                    TRACE_LEVEL_PREV;
                    
                    if (result != erConflict) {
                        result = normalize_ca(b_ca_master ? p_ca : p_c2, b_ca_master ? p_ca : p_c2);
                    };
                    
                    return result;
                };
            };
        };
        return erUndetermined;
    };
    
    // subsume ca2 by ca1
    // template parameters are validated within caca_expand_clause_flags
    template<clause_size_t size1, clause_size_t size2, clause_size_t index1, clause_size_t index2, clause_size_t index3>
    inline processor_result_t CnfOptimizer::subsume_ca(uint32_t* const p_ca1, const container_offset_t ca2_offset) {
        uint32_t* const p_ca2 = _clauses_offset_clause(clauses_data_, ca2_offset);
        _assert_level_0(_clause_size(p_ca2) == size2);
        processor_result_t result = erUndetermined;
        
        clause_flags_t caca_flags = _clause_flags(p_ca1);
        caca_expand_flags<size1, size2, index1, index2, index3>(caca_flags);
        
        // update only if the clause flags changed
        const clause_flags_t ca_flags = _clause_flags(p_ca2);
        
        if ((~caca_flags & ca_flags) == 0) {
            // ca is fully subsumed by c2
            // do not update the flags; postpone processing since the clause may be immutable etc
            exclude_clause(ca2_offset);
        } else if ((caca_flags & ~ca_flags) != 0) {
            // check that the non-matching variables are unchanged
            constexpr unsigned indexX = index1 > 0 ? 0 : index2 > 1 ? 1 : index3 == 2 ? 3 : 2;
            const literalid_t* const variables = variables_.data();

            bool is_clause_unchanged = _clause_is_literal_unchanged(p_ca2, indexX, variables);
            if (size1 == 2 && size2 == 4) {
                is_clause_unchanged &= _clause_is_literal_unchanged(p_ca2, 6 - index1 - index2 - indexX, variables);
            };
            
            if (is_clause_unchanged) {
                // evaluate new flags/clauses only
                // this would trigger a merge and transitive closure within
                // p_ca may become invalid during evaluation
                
                uint32_t ca1_expanded[_clause_size_memory_size(size2)];
                _clause_copy_size(p_ca2, ca1_expanded, size2);
                _clause_header(ca1_expanded) = _clause_header_make(caca_flags & ~ca_flags, size2);
                
                TRACE_LEVEL_NEXT;
                TRACE_CLAUSE(p_ca2, CONTAINER_END);
                TRACE_LEVEL_NEXT;
                TRACE_CLAUSE_RESOLVENT(ca1_expanded);
                
                result = evaluate_clause_a(ca1_expanded, ca2_offset);
                
                TRACE_LEVEL_PREV;
                TRACE_LEVEL_PREV;
                
                if (result != erConflict) {
                    result = normalize_ca(p_ca1, p_ca1);
                };
            };
        };

        return result;
    };
    
    template<clause_size_t size1, clause_size_t size2, clause_size_t index1, clause_size_t index2, clause_size_t index3, bool b_lookup>
    inline processor_result_t CnfOptimizer::merge_ca(uint32_t* const p_ca1, container_offset_t& ca1_offset, uint32_t* const p_ca2, clause_flags_t& ca2_flags) {
        
        _assert_level_1(!b_lookup || _clause_flags(p_ca1) == 0);
        clause_flags_t flags = caca_reduced_flags<size1, size2, index1, index2, index3>(ca2_flags);
        if (b_lookup || flags != _clause_flags(p_ca1)) {
            if (flags == 0 || size1 != 2 || _c2_is_single_clause_flags(flags)) {
                if (b_lookup) {
                    ca1_offset = clauses_.find(p_ca1);
                    if (ca1_offset != CONTAINER_END) {
                        const clause_flags_t ca1_flags = _clauses_offset_flags(clauses_data_, ca1_offset);
                        _assert_level_1(ca1_flags != 0);
                        // a processed binary clause must be the only one for its pair of variables
                        _assert_level_3(size1 != 2 || ca1_offset >= processed_offset_ || _c2_is_single_clause_flags(ca1_flags));
                        flags |= ca1_flags;
                        _clause_flags_set(p_ca1, ca1_flags);
                        TRACE_CLAUSE_MERGE(p_ca2, ca1_offset);
                    };
                } else {
                    flags |= _clause_flags(p_ca1);
                };
                if (flags == 0) {
                    return erUndetermined;
                } else if (size1 == 2 && _c2_is_single_clause_flags(flags)) {
                    _clause_flags_include(p_ca1, flags);
                    if (ca1_offset == CONTAINER_END) {
                        // append new binary clause
                        Cnf::insertion_point_t insertion_point;
                        __insertion_point_t_init(insertion_point); // suboptimal, can reuse the above find
                        clauses_.append<false>(p_ca1, insertion_point);
                        TRACE_CLAUSE_APPEND(p_ca1);
                        _assert_level_1(insertion_point.kind == btipkCurrent);
                        ca1_offset = insertion_point.container_offset;
                    } else {
                        caca_expand_flags<size1, size2, index1, index2, index3>(flags);
                        ca2_flags |= flags;
                    };
                    return erUndetermined;
                } else if (size1 != 2) {
                    clause_flags_t ca1_expanded_flags = flags;
                    caca_expand_flags<size1, size2, index1, index2, index3>(ca1_expanded_flags);
                    ca2_flags |= ca1_expanded_flags;
                    
                    if (!b_lookup) {
                        _clause_flags_include(p_ca1, flags);
                        // ca1 flags has been merged into ca2
                        // in the absence of ca1 residual, it can be ignored/excluded
                        // since any derivatives will be produced from ca2 now
                        if (ca_residual_flags<size1>(flags) == 0) {
                            if (ca1_offset != CONTAINER_END) {
                                if (!clauses_.transaction_offset_is_immutable(ca1_offset)) {
                                    _clauses_offset_flags_include(clauses_data_, ca1_offset, _clause_flags(p_ca1));
                                    TRACE_LEVEL_NEXT;
                                    TRACE_CLAUSE_UPDATE(p_ca1, ca1_offset);
                                    TRACE_LEVEL_PREV;
                                };
                                exclude_clause(ca1_offset);
                            };
                            return erUndetermined;
                        };
                    } else {
                        return erUndetermined;
                    };
                } else {
                    // additional exciting flags
                    _clause_flags_include(p_ca1, flags);
                };
            } else {
                // new reduced flags
                _clause_flags_include(p_ca1, flags);
            };
            
            // more than one binary clause with the same variables
            // which means at least one variable will be assigned
            // or an update to a longer clause
            TRACE_LEVEL_NEXT;
            TRACE_CLAUSE_RESOLVENT(p_ca1);
            processor_result_t result = evaluate_clause_a(p_ca1, ca1_offset, CONTAINER_END);
            TRACE_LEVEL_PREV;
            if (result != erConflict) {
                result = normalize_ca(p_ca2, p_ca2);
                _assert_level_1(size1 != 2 || result != erUndetermined);
            };
            return result;
        } else {
            return erUndetermined;
        }
    };
    
    template<clause_size_t size>
    inline processor_result_t CnfOptimizer::merge_ca(uint32_t* const p_clause) {
        static_assert(size == 3 || size == 4, "implemented for c3/c4 only");
        
        clause_flags_t flags = _clause_flags(p_clause);
        processor_result_t result;
        
        if (size >= 3) {
            uint32_t c201[_clause_size_memory_size(2)] = _clause_c2_make(0, _clause_literal(p_clause, 0), _clause_literal(p_clause, 1));
            uint32_t c202[_clause_size_memory_size(2)] = _clause_c2_make(0, _clause_literal(p_clause, 0), _clause_literal(p_clause, 2));
            uint32_t c212[_clause_size_memory_size(2)] = _clause_c2_make(0, _clause_literal(p_clause, 1), _clause_literal(p_clause, 2));
            
            container_offset_t offset201 = CONTAINER_END;
            container_offset_t offset202 = CONTAINER_END;
            container_offset_t offset212 = CONTAINER_END;
            
            result = merge_ca<2, size, 0, 1, size, true>(c201, offset201, p_clause, flags);
            if (result != erUndetermined) return result;
            
            result = merge_ca<2, size, 0, 2, size, true>(c202, offset202, p_clause, flags);
            if (result != erUndetermined) return result;
            
            result = merge_ca<2, size, 1, 2, size, true>(c212, offset212, p_clause, flags);
            if (result != erUndetermined) return result;
            
            if (size == 4) {
                uint32_t c203[_clause_size_memory_size(2)] = _clause_c2_make(0, _clause_literal(p_clause, 0), _clause_literal(p_clause, 3));
                uint32_t c213[_clause_size_memory_size(2)] = _clause_c2_make(0, _clause_literal(p_clause, 1), _clause_literal(p_clause, 3));
                uint32_t c223[_clause_size_memory_size(2)] = _clause_c2_make(0, _clause_literal(p_clause, 2), _clause_literal(p_clause, 3));
                uint32_t c3012[_clause_size_memory_size(3)] = _clause_c3_make(0, _clause_literal(p_clause, 0), _clause_literal(p_clause, 1), _clause_literal(p_clause, 2));
                uint32_t c3013[_clause_size_memory_size(3)] = _clause_c3_make(0, _clause_literal(p_clause, 0), _clause_literal(p_clause, 1), _clause_literal(p_clause, 3));
                uint32_t c3023[_clause_size_memory_size(3)] = _clause_c3_make(0, _clause_literal(p_clause, 0), _clause_literal(p_clause, 2), _clause_literal(p_clause, 3));
                uint32_t c3123[_clause_size_memory_size(3)] = _clause_c3_make(0, _clause_literal(p_clause, 1), _clause_literal(p_clause, 2), _clause_literal(p_clause, 3));
                
                container_offset_t offset203 = CONTAINER_END;
                container_offset_t offset213 = CONTAINER_END;
                container_offset_t offset223 = CONTAINER_END;
                container_offset_t offset3012 = CONTAINER_END;
                container_offset_t offset3013 = CONTAINER_END;
                container_offset_t offset3023 = CONTAINER_END;
                container_offset_t offset3123 = CONTAINER_END;
                
                result = merge_ca<2, 4, 0, 3, 4, true>(c203, offset203, p_clause, flags);
                if (result != erUndetermined) return result;
                result = merge_ca<2, 4, 1, 3, 4, true>(c213, offset213, p_clause, flags);
                if (result != erUndetermined) return result;
                result = merge_ca<2, 4, 2, 3, 4, true>(c223, offset223, p_clause, flags);
                if (result != erUndetermined) return result;
                result = merge_ca<3, 4, 0, 1, 2, true>(c3012, offset3012, p_clause, flags);
                if (result != erUndetermined) return result;
                result = merge_ca<3, 4, 0, 1, 3, true>(c3013, offset3013, p_clause, flags);
                if (result != erUndetermined) return result;
                result = merge_ca<3, 4, 0, 2, 3, true>(c3023, offset3023, p_clause, flags);
                if (result != erUndetermined) return result;
                result = merge_ca<3, 4, 1, 2, 3, true>(c3123, offset3123, p_clause, flags);

                if (result != erUndetermined) return result;
                result = merge_ca<2, 4, 2, 3, 4, false>(c223, offset223, p_clause, flags);
                if (result != erUndetermined) return result;
                result = merge_ca<2, 4, 1, 3, 4, false>(c213, offset213, p_clause, flags);
                if (result != erUndetermined) return result;
                result = merge_ca<2, 4, 0, 3, 4, false>(c203, offset203, p_clause, flags);
                if (result != erUndetermined) return result;
                result = merge_ca<2, 4, 1, 2, 4, false>(c212, offset212, p_clause, flags);
                if (result != erUndetermined) return result;
                result = merge_ca<2, size, 0, 2, size, false>(c202, offset202, p_clause, flags);
                if (result != erUndetermined) return result;
                result = merge_ca<2, size, 0, 1, size, false>(c201, offset201, p_clause, flags);
                if (result != erUndetermined) return result;

                result = merge_ca<3, 4, 0, 2, 3, false>(c3023, offset3023, p_clause, flags);
                if (result != erUndetermined) return result;
                result = merge_ca<3, 4, 0, 1, 3, false>(c3013, offset3013, p_clause, flags);
                if (result != erUndetermined) return result;
                result = merge_ca<3, 4, 0, 1, 2, false>(c3012, offset3012, p_clause, flags);
                if (result != erUndetermined) return result;
                result = merge_ca<3, 4, 1, 2, 3, false>(c3123, offset3123, p_clause, flags);
                if (result != erUndetermined) return result;
            } else {
                result = merge_ca<2, size, 0, 2, size, false>(c202, offset202, p_clause, flags);
                if (result != erUndetermined) return result;
                result = merge_ca<2, size, 0, 1, size, false>(c201, offset201, p_clause, flags);
                if (result != erUndetermined) return result;
            };

            _clause_flags_include(p_clause, flags);
        };
        
        return result;
    };
    
    // build transitive closure incrementally
    // find all first-level implicants and produce all derivative clauses
    // subsequent level implicants will be produced while processing those
    // while potentially requiring more space, this approach reduces combinations because
    // - many derived clauses can be excluded along the way before further resolution
    // - direct resolution rules may be implemented for ternary and longer clauses
    // - some of the resolvents will be aggregated and possibly reduced immediately
    // expect that p_c2 is normalized
    inline processor_result_t CnfOptimizer::resolve_c2(uint32_t* const p_clause) {
        _assert_level_0(_clause_size(p_clause) == 2);
        processor_result_t result = erUndetermined;
        
        CnfProcessor::clauses_iterator_t<2> iterator(clauses_index_, *this);
        const container_offset_t c2_variables[2] = {
            literal_t__variable_id(_clause_literal(p_clause, 0)),
            literal_t__variable_id(_clause_literal(p_clause, 1))
        };
        
        container_offset_t ca_offset = iterator.first(c2_variables);
        while (ca_offset != CLAUSES_END) {
            uint32_t* p_ca = _clauses_offset_clause(clauses_data_, ca_offset);
            const clause_size_t ca_size = _clause_size(p_ca);
            switch(ca_size) {
                case 2:
                    switch(iterator.instance_bits) {
                        case 0b00:
                            _assert_level_0(false);
                            break;
                        case 0b01:
                            if (_clause_literal(p_clause, 0) == _clause_literal(p_ca, 0)) {
                                result = resolve_ca_c2<false, 2, 0, 0>(p_ca, p_clause, _clause_flags(p_ca));
                            } else {
                                result = resolve_ca_c2<false, 2, 1, 0>(p_ca, p_clause, _clause_flags(p_ca));
                            };
                            break;
                        case 0b10:
                            if (_clause_literal(p_clause, 1) == _clause_literal(p_ca, 0)) {
                                result = resolve_ca_c2<false, 2, 0, 1>(p_ca, p_clause, _clause_flags(p_ca));
                            } else {
                                result = resolve_ca_c2<false, 2, 1, 1>(p_ca, p_clause, _clause_flags(p_ca));
                            };
                            break;
                        case 0b11:
                            _assert_level_0(false);
                            break;
                    };
                    break;
            };
            
            if (result == erUndetermined) {
                ca_offset = iterator.next();
            } else {
                break;
            };
        };
    
        // no resolvents changed while processing means no changes to the clause
        _assert_level_3(result != erUndetermined || normalize_ca(p_clause, p_clause) == erUndetermined);
        return result;
    };
    
    // returns true if the clause remains after updates
    // returns false if the clause should be ignored
    inline bool CnfOptimizer::_normalize_clause(uint32_t* const p_clause) const {
        processor_result_t result = erUndetermined;
        const clause_size_t clause_size = _clause_size(p_clause);
        
        if (_clause_size_is_aggregated(clause_size)) {
            result = normalize_ca(p_clause, p_clause);
        } else {
            result = normalize_cu(p_clause, p_clause);
        };
        _assert_level_1(result != erConflict);
        _assert_level_1(result == erSatisfied || _clause_size(p_clause) > 0);
        return (result != erSatisfied);
    };
    
    inline bool CnfOptimizer::_update_clause_variables(uint32_t* const p_clause) const {
        if (_clause_is_included(p_clause)) {
            const uint32_t clause_size = _clause_size(p_clause);
            const literalid_t* var_values = variables_.data();
            
            // assign reindexed variable numbers
            // sort order / normalization must be preserved because the only possible change is reindexing
            // all other changes must have been handled while evaluating
            for (auto i = 0; i < clause_size; i++) {
                const literalid_t old_literal = _clause_literal(p_clause, i);
                const literalid_t new_literal = literal_t__lookup(var_values, old_literal);
                if (old_literal != new_literal) {
                    // constants not possible as those clauses must have been eliminated
                    _assert_level_1(literal_t__is_variable(new_literal));
                    if (literal_t__is_negation(new_literal) && _clause_is_aggregated(p_clause)) {
                        // only unnegated mappings are possible for variables within an aggregated clause
                        _assert_level_3(literal_t__is_same_variable(old_literal, new_literal));
                        clause_flags_t flags = _clause_flags(p_clause);
                        ca_flags_negate(flags, i);
                        _clause_flags_set(p_clause, flags);
                    } else {
                        _clause_literal(p_clause, i) = new_literal;
                    };
                };
            };
            return true;
        } else {
            return false;
        };
    };
     
    // reindex variables such that there are no gaps in variable numbers
    // update CNF variables count
    inline variableid_t CnfOptimizer::update_variables(const bool b_reindex_variables) {
        literalid_t* var_values = variables_.data();
        variableid_t next_variable_id = VARIABLEID_MIN;
        
        // go through the variables sequentially
        for (variables_size_t i = 0; i < variables_.size(); i++) {
            if (literal_t__is_variable(var_values[i])) {
                variableid_t variable_id = literal_t__variable_id(var_values[i]);
                if (variable_id != i) {
                    // assume referencing of variables with lower ids only
                    // and therefore the reference is updated by now
                    assert(i > variable_id);
                    // all clauses with the variable must have been excluded
                    assert(!is_variable_used(i));
                    // UNASSIGNED may not be referenced
                    assert(!literal_t__is_unassigned(var_values[variable_id]));
                    // take the reference of the reference instead of making a new variable
                    var_values[i] = literal_t__substitute_literal(var_values[i], var_values[variable_id]);
                } else if (is_variable_used(variable_id) || cnf_.is_variable_named(variable_id)) {
                    if (b_reindex_variables) {
                        // generate new variable id
                        if (next_variable_id != variable_id) {
                            // only update after next_variable_id falls behind
                            var_values[i] = literal_t__substitute_variable(var_values[i], next_variable_id);
                        };
                        next_variable_id++;
                    };
                } else {
                    var_values[i] = LITERALID_UNASSIGNED;
                };
            } else {
                // unassigned cannot be created earlier than this stage
                assert(literal_t__is_constant(var_values[i]));
            };
        };
#ifdef CNF_TRACE
        if (p_tracer_ != nullptr) p_tracer_->update_variables(variables_);
#endif
        // validate the new variables size
        _assert_level_1(cnf_.variables_size() <= variables_.size());
        _assert_level_1(next_variable_id <= variables_.size());
        return next_variable_id;
    };
        
    // generate clauses for constant and variable values
    // check presence of existant unit clauses and the conflicts
    // return false if there is a conflict
    // fail on attempt to assign "unassigned" - unsupported because unnecessary
    inline bool CnfOptimizer::assign_variable_values_unoptimized() {
        clauses_.transaction_begin();
        
        const literalid_t* const variables = variables_.data();
        for (auto i = 0; i < variables_.size(); i++) {
            if (variable_t__literal_id(i) != variables[i]) {
                if (literal_t__is_constant(variables[i])) {
                    // unit clause
                    uint32_t unit_clause[_clause_size_memory_size(1)] = _clause_c1_make(literal_t__negated_onlyif(variable_t__literal_id(i), literal_t__is_constant_0(variables[i])));
                    Cnf::insertion_point_t insertion_point;
                    __insertion_point_t_init(insertion_point);
                    clauses_.find(unit_clause, insertion_point);
                    if (insertion_point.container_offset == CONTAINER_END) {
                        clauses_.append<true>(unit_clause, insertion_point);
                    } else {
                        // check that the value is the same, otherwise generate conflict
                        if (_clauses_offset_flags(clauses_data_, insertion_point.container_offset) != _clause_flags(unit_clause)) {
                            __print_conflict(variables_.data(), _clauses_offset_clause(clauses_data_, insertion_point.container_offset));
                            clauses_.transaction_rollback();
                            return false;
                        };
                    };
                } else if (literal_t__is_variable(variables[i])) {
                    // equality, 2 clauses
                    clauses_.append_clause_l(variable_t__literal_id(i), literal_t__negated(variables[i]));
                    clauses_.append_clause_l(literal_t__negated(variable_t__literal_id(i)), variables[i]);
                } else {
                    _assert_level_0(false);
                };
            };
        };
        
        clauses_.transaction_commit();
        return true;
    };
    
    inline bool CnfOptimizer::base_execute(const bool b_reindex_variables, const FormulaProcessingMode mode) {
        _assert_level_0(mode == fpmAll || mode == fpmOriginal);
        
        clauses_.transaction_begin();
        __statistics_reset();
        
        TRACE_START;
        bool result = (evaluate_clauses() != erConflict);
        TRACE_FINISH;
        
        if (result) {
            
            __statistics_print();
            
            if (mode == fpmOriginal) {
                // "lazy" suboptimal approach until transitive reduction works efficiently
                // variables are reindexed at this stage
                // keep original clauses after updating variables;
                // except those satisfied while normalizing
                clauses_.transaction_rollback();
                // unit propagate or substitute variable values, rebuild indexes
                rebuild_clauses<CnfOptimizer, &CnfOptimizer::_normalize_clause>(this, false);
                // this may leave excluded clauses; second reinitialization addresses this
                CnfSubsumptionOptimizer::execute();
            } else if (mode == fpmAll) {
                clauses_.transaction_commit();
            };
            
            const variableid_t new_variables_size = update_variables(b_reindex_variables);
            rebuild_clauses<CnfOptimizer, &CnfOptimizer::_update_clause_variables>(this, mode == fpmAll);
            cnf_.named_variables_update(variables_);
            if (b_reindex_variables && new_variables_size != cnf_.variables_size()) {
                set_variables_size(new_variables_size);
            };

        } else {
            clauses_.transaction_rollback();
        };
        
        clauses_index_.reset(0, 0);
        processed_offset_ = 0; // to match state of the indexes
        
        return result;
    };
    
    bool CnfOptimizer::execute() {
        return execute(true, fpmOriginal);
    };
    
    // optimizes the given Cnf without any parameters
    bool CnfOptimizer::execute(const bool b_reindex_variables, const FormulaProcessingMode mode) {
        if (mode == fpmUnoptimized) {
            return assign_variable_values_unoptimized();
        } else {
            auto original_clauses_size = cnf_.clauses_size();
            
            bool result = base_execute(b_reindex_variables, mode);
            if (result) {
                std::cout << "Optimized: " << std::dec;
                std::cout << "(" << variables_.size() << ", " << original_clauses_size << ") -> ";
                std::cout << "(" << (signed long)cnf_.variables_size() - variables_.size() << ", ";
                std::cout << (signed long)cnf_.clauses_size() - original_clauses_size << ") -> ";
                std::cout << "(" << cnf_.variables_size() << ", " << cnf_.clauses_size() << ")";
                std::cout << std::endl;
            };
            return result;
        };
    };
    
    // CnfVariableEvaluator
    
    bool CnfVariableEvaluator::execute() {
        clauses_.transaction_begin();
        bool result = (evaluate_clauses() != erConflict);
        clauses_.transaction_rollback();
        
        if (result) {
            // update variables to resolve any sequenced references
            // lighter version of update_variables() with no reindexing
            literalid_t* var_values = variables_.data();
            
            // go through the variables sequentially
            for (variableid_t i = 0; i < variables_.size(); i++) {
                const literalid_t value = var_values[i];
                if (literal_t__is_variable(value) && i != literal_t__variable_id(value)) {
                    _assert_level_1(i > literal_t__variable_id(value));
                    const literalid_t new_value = var_values[literal_t__variable_id(value)];
                    _assert_level_1(!literal_t__is_unassigned(new_value));
                    var_values[i] = literal_t__substitute_literal(value, new_value);
                };
            };
        };
        return result;
    };
    
    // CnfVariableNormalizer
    
    bool CnfVariableNormalizer::execute(const bool b_reindex_variables) {
        cnf_.named_variables_assign_negations(variables_);
        
        // index is needed for checking if a variable is used
        build_clauses_index();
        const variableid_t new_variables_size = update_variables(b_reindex_variables);
        if (b_reindex_variables && new_variables_size != cnf_.variables_size()) {
            set_variables_size(new_variables_size);
        };
            
        // nothe that rebuild_clauses() invalidates clauses_index_ and processed_offset_
        // since these are not used below, its fine
        rebuild_clauses<CnfOptimizer, &CnfOptimizer::_update_clause_variables>(this, false);
        cnf_.named_variables_update(variables_);
        
        // it is possible that some negated references remain
        // since the same variable may appear straight and complemented
        // across named variables
        // for those, need to introduce new variables
        
        for (auto vit = named_variables_.begin(); vit != named_variables_.end(); vit++) {
            literalid_t* const template_ = vit->second.data();
            for (auto j = 0; j < vit->second.size(); j++) {
                if (literal_t__is_variable(template_[j]) && literal_t__is_negation(template_[j])) {
                    literalid_t value = cnf_.new_variable_literal();
                    clauses_.append_clause_l(value, literal_t__negated(template_[j]));
                    clauses_.append_clause_l(literal_t__negated(value), template_[j]);
                    template_[j] = value;
                };
            };
        };
        
        return true;
    };
};
