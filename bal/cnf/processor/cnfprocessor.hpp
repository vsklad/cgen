//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfprocessor_hpp
#define cnfprocessor_hpp

#include "cnf.hpp"
#include "linkedlistindex.hpp"

namespace bal {
    
    enum processor_result_t { erUndetermined, erChangedC, erChangedV, erSatisfied, erConflict };
    
    class CnfProcessor {
    protected:
        Cnf& cnf_;
        Cnf::clauses_container_t& clauses_;
        //const Container<uint32_t>* const p_container_;
        uint32_t*& clauses_data_;
        const container_size_t& clauses_size_;
        formula_named_variables_t& named_variables_;
        
        inline void set_variables_size(const variableid_t value) {
            cnf_.__set_variables_size(value);
        };
        
    protected:
        // lists of clauses for each variable
        // order is the same as in the main list, i.e. by offset asc
        // the index is built incrementally during primary causes evaluation cycle
        SimpleLinkedListsIndex<uint32_t> clauses_index_;
        
        // separates clauses that were processed already;
        // when evaluating, processing of subsequent clauses is postponed
        // reducing recursion and improving performance
        // implementation approach relies on this, notably variable assignments
        // where the check prevents unbounded recursion
        container_offset_t processed_offset_ = 0;
        
    protected:
        // DEBUG method
        // outputs processed clauses only
        inline bool __print_clause_offset(const container_offset_t offset) const {
            return __print_clause(_clauses_offset_clause(clauses_data_, offset));
        };
        
        inline void __print_variable_clauses(const variableid_t variable_id) const {
            clauses_index_.iterate_const<CnfProcessor, bool, &CnfProcessor::__print_clause_offset, true>(this, variable_id);
        };
        
    protected:
        // public for variable clauses iterator
        inline bool is_clause_included(const container_offset_t offset) const {
            return _clauses_offset_is_included(clauses_data_, offset);
        };
        
        using nonoptimizing_clauses_iterator_t = LinkedListsIndexInstanceFilteredOffsetIterator<uint32_t, CnfProcessor, &CnfProcessor::is_clause_included>;
        
        template<clause_size_t variables_size>
        using clauses_iterator_t = SimpleLinkedListsIndexMergedFilteredOptimizingInstanceOffsetIterator<uint32_t, variables_size, CnfProcessor, &CnfProcessor::is_clause_included>;
        
        /*
         typedef LinkedListsIndexInstanceFilteredOffsetIterator<uint32_t, CnfOptimizer, &CnfOptimizer::is_clause_included> full_index_iterator_t;
         #define __full_index_iterator() full_index_iterator_t(clauses_index_, *this)
         */
        
    protected:
        // determine if the binary variable is used in at least one clause
        inline bool is_variable_used(const variableid_t variable_id) const {
            return clauses_index_.iterate_const<CnfProcessor, bool, &CnfProcessor::is_clause_included, false>(this, variable_id);
        };
        
        // process each clause, rebuild the data and the index
        // safe mode is slower and memory demanding but does not rely on clauses being unmodified
        // safe mode assumes all clause headers contain the original size so that memory size can be calculated
        template<typename CALLER_T, bool (CALLER_T::*p_update_clause)(uint32_t* const p_clause) const>
        inline void rebuild_clauses(const CALLER_T* const p_caller, const bool b_safe_mode) const {
            _assert_level_1(!cnf_.transaction_is_in());
            if (b_safe_mode) {
                // make a sorted list of all used container offsets
                // then basically d the same thing as unsafe while limiting to these offsets
                std::vector<container_offset_t> offsets;
                for (auto it: cnf_.clauses()) {
                    offsets.push_back((container_offset_t)(it - clauses_data_));
                };
                std::sort(offsets.begin(), offsets.end());
                // rollback after collecting the offsets
                cnf_.rollback(0, 0, 0);
                // add the clauses back
                for (auto i = 0; i < offsets.size(); i++) {
                    uint32_t* const p_clause = _clauses_offset_clause(clauses_data_, offsets[i]);
                    if ((p_caller->*p_update_clause)(p_clause)) {
                        Cnf::insertion_point_t insertion_point;
                        __insertion_point_t_init(insertion_point);
                        clauses_.append<false>(p_clause, insertion_point);
                    };
                };
            } else {
                const container_size_t original_size_ = cnf_.size_;
                cnf_.rollback(0, 0, 0);
                uint32_t offset = 0;
                while (offset < original_size_) {
                    uint32_t* const p_clause = _clauses_offset_clause(clauses_data_, offset);
                    // p_clause size may change
                    const clause_size_t clause_size = _clause_size(p_clause);
                    if ((p_caller->*p_update_clause)(p_clause)) {
                        Cnf::insertion_point_t insertion_point;
                        __insertion_point_t_init(insertion_point);
                        clauses_.append<false>(p_clause, insertion_point);
                    };
                    _clauses_offset_size_next(offset, clause_size);
                };
            };
        };
        
        // process each clause, remove it if the processing method returns false
        template<typename CALLER_T, processor_result_t (CALLER_T::*p_process_clause)(uint32_t* const p_clause)>
        inline processor_result_t process_clauses(CALLER_T* const p_caller) {
            clauses_index_.reset(0, 0);
            processor_result_t result = erUndetermined;
            
            uint32_t offset = 0;
            while (offset < cnf_.size_) {
                uint32_t* p_clause = _clauses_offset_clause(clauses_data_, offset);
                // p_clause size may change
                const clause_size_t clause_size = _clause_size(p_clause);
                
                // the clause may be excluded during processing of the preceding ones
                if (_clause_is_included(p_clause)) {
                    processed_offset_ = offset;
                    if ((p_caller->*p_process_clause)(p_clause) == erConflict) {
                        result = erConflict;
                        break;
                    } else {
                        // the pointer may become invalid
                        p_clause = _clauses_offset_clause(clauses_data_, offset);
                        if (_clause_is_included(p_clause)) {
                            // append processed clause to the index if included
                            for (auto i = 0; i < _clause_size(p_clause); i++) {
                                clauses_index_.append(literal_t__variable_id(_clause_literal(p_clause, i)), offset);
                            };
                        };
                    };
                };
                
                _clauses_offset_size_next(offset, clause_size);
            };
            
            return result;
        };
        
    private:
        inline processor_result_t process_clause_build_index(uint32_t* const p_clause) { return erUndetermined; };
        
    protected:
        void build_clauses_index() {
            process_clauses<CnfProcessor, &CnfProcessor::process_clause_build_index>(this);
        };
        
    public:
        CnfProcessor(Cnf& cnf): cnf_(cnf), clauses_(cnf_), clauses_data_(cnf_.data_), clauses_size_(cnf_.size_), named_variables_(cnf_.get_named_variables_()), clauses_index_(cnf_.p_container_) {};
        
        virtual bool execute() = 0;
    };
    
};

#endif /* cnfprocessor_hpp */
