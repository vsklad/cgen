//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfclausescontainer_hpp
#define cnfclausescontainer_hpp

#include "assertlevels.hpp"
#include "container.hpp"
#include "binarytreeindex.hpp"
#include "cnfclauses.hpp"

namespace bal {
    
    // macros for addressing clause elements within container
    // can be converted to functions; kept this way for performance reasons
    // see cnfclauses.hpp for clause memory sctructure
#define _clauses_offset_size_memory_size(clause_size) (_clause_size_memory_size(clause_size) + 3)
#define _clauses_offset_item(clauses, offset) ((clauses) + (offset))
#define _clauses_offset_item_clause(p_item) ((p_item) + 3)
#define _clauses_offset_clause(clauses, offset) _clauses_offset_item_clause(_clauses_offset_item(clauses, offset))
#define _clauses_offset_is_included(clauses, offset) _clause_is_included(_clauses_offset_clause(clauses, offset))
#define _clauses_offset_flags(clauses, offset) _clause_flags(_clauses_offset_clause(clauses, offset))
#define _clauses_offset_header(clauses, offset) _clause_header(_clauses_offset_clause(clauses, offset))
#define _clauses_offset_flags(clauses, offset) _clause_flags(_clauses_offset_clause(clauses, offset))
#define _clauses_offset_flags_include(clauses, offset, flags) _clause_flags_include(_clauses_offset_clause(clauses, offset), flags)
#define _clauses_offset_size(clauses, offset) _clause_size(_clauses_offset_clause(clauses, offset))
#define _clauses_offset_literals(clauses, offset) _clause_literals(_clauses_offset_clause(clauses, offset))
#define _clauses_offset_literal(clauses, offset, index) (*(_clause_literals(_clauses_offset_clause(clauses, offset)) + (index)))

    // notes:
    //   1. inclusion/exclusion is only used for processing
    //   2. inclusion is only used for rolling back changes and restoring of original clauses
#define _clauses_offset_exclude(clauses, offset) _clause_exclude(_clauses_offset_clause(clauses, offset))
#define _clauses_offset_is_included(clauses, offset) _clause_is_included(_clauses_offset_clause(clauses, offset))
    
    // must be limited to situations when it is known no clauses changed size
#define _clauses_offset_size_next(offset, clause_size) (offset) += _clauses_offset_size_memory_size(clause_size)
#define _clauses_offset_next(offset, p_clause) _clauses_offset_size_next(offset, _clause_size(p_clause))
    
    template<bool compare_left_right>
    inline container_offset_t clause_index_variable_id(const uint32_t* const p_clause) {
        return literal_t__variable_id(_clause_literal(p_clause, compare_left_right ? 0 : _clause_size(p_clause) - 1));
    };
    
    // clauses are stored within the container
    // with binary tree index data for each clause
    // binary tree index orders clauses according to specified criteria
    // see cnfclauses.hpp for clause memory sctructure
    template<bool compare_left_right>
    class CnfClausesIndexedContainer: public AvlTreesIndex<uint32_t, true, compare_clauses<compare_left_right>, clause_index_variable_id<compare_left_right>> {
    public:
        using base_t = AvlTreesIndex<uint32_t, true, compare_clauses<compare_left_right>, clause_index_variable_id<compare_left_right>>;
        using insertion_point_t = typename base_t::insertion_point_t;
        
        CnfClausesIndexedContainer() = default;
        
    private:
        // returns 0 if the clause is always satisfied; otherwise returns the new size
        inline static clause_size_t normalize_clause_literals(uint32_t* const literals, const clause_size_t literals_size) {
            assert(literals_size > 0);
            
            std::sort(literals, literals + literals_size);
            
            assert(literal_t__is_variable(literals[0]));
            clause_size_t validated_size = 1;
            for (auto i = 1; i < literals_size; i++) {
                assert(literal_t__is_variable(literals[i]));
                if (literals[i] == literals[validated_size - 1]) {
                    //warning(literals_size, literals, "ignoring duplicate literal");
                    continue;
                } else if (literal_t__is_negation_of(literals[i], literals[validated_size - 1])) {
                    //warning(literals_size, literals, "ignoring the clause, always satisfied");
                    return 0;
                } else {
                    if (validated_size < i) {
                        literals[validated_size] = literals[i];
                    };
                    validated_size++;
                };
            };
            
            return validated_size;
        };
        
    protected:
        // simply rebuild the index on rollback, with rebalancing
        // this is because of "replacement" of items which is a modification rather than addition
        // also, rebalancing is needed anyway
        // can use offset increments and _clauses_offset_next because its only immutable clauses
        void rollback(const container_size_t size,
                      const container_size_t instances_size,
                      const container_size_t container_size) override {
            base_t::rollback(0, 0, container_size);
            this->instances_.append(CONTAINER_END, instances_size);
            container_offset_t container_offset = 0;
            while (container_offset < container_size) {
                uint32_t* const p_clause = _clauses_offset_clause(this->p_container_->data_, container_offset);
                _clause_include(p_clause);
                insertion_point_t insertion_point;
                this->find(p_clause, insertion_point);
                _assert_level_1(insertion_point.container_offset == CONTAINER_END);
                base_t::_update(container_offset, insertion_point);
                _clauses_offset_next(container_offset, p_clause);
                this->size_ = container_offset; // "commits" the clause
            };
        };
        
    public:
        CnfClausesIndexedContainer(const Container<uint32_t>& container): base_t(container) {};
        
        // clauses list ordered using clauses_index_
        using clauses_iterable_t = ContainerIterable<CnfClausesIndexedContainer<compare_left_right>, ContainerIndexIterator>;
        clauses_iterable_t clauses() const { return clauses_iterable_t(*this); };
        
        // clause_size - 0 means count clauses of all lengths, otherwise specified length only
        // aggregated - count aggregates if true, otherwise count individual clauses
        template<const clause_size_t clause_size = 0, bool aggregated = false, bool literals = false>
        const clauses_size_t clauses_size() const {
            clauses_size_t result = 0;
            for (auto it: clauses()) {
                const uint32_t* const p_clause = _clauses_offset_item_clause(it);
                const clause_size_t current_clause_size = _clause_size(p_clause);
                if (clause_size == 0 || clause_size == current_clause_size) {
                    if(!aggregated && _clause_size_is_aggregated(current_clause_size)) {
                        result += get_cardinality_uint16(_clause_flags(p_clause)) * (literals ? current_clause_size : 1);
                    } else {
                        result += literals ? current_clause_size : 1;
                    };
                };
            };
            return result;
        };
        
        inline container_offset_t find(const uint32_t* const p_object) const {
            container_offset_t result = base_t::find(p_object);
            
            if (result == CONTAINER_END) __find_clause_unfound++; else __find_clause_found++;
            
            return result;
        };
        
        inline void find(const uint32_t* const p_object, insertion_point_t &insertion_point) const {
            base_t::find(p_object, insertion_point);
            
            if (insertion_point.container_offset == CONTAINER_END) __find_clause_unfound++; else __find_clause_found++;
        };
        
    public:
        // avoid_merging forces to append new clause and prevents merging with an existing one
        template<bool avoid_merging>
        inline void append(const uint32_t* const p_clause, insertion_point_t& insertion_point) {
            __append_clause_++;
            
            _assert_level_0(_clause_is_included(p_clause));
            
            const clause_size_t clause_size = _clause_size(p_clause);
            _assert_level_0(clause_size != 0 && clause_size <= CLAUSE_SIZE_MAX);
            
            // should be sorted at this stage
            // check is sorted; remove later for performance reasons
            for (auto i = 1; i < clause_size; i++) {
                _assert_level_1(_clause_literal(p_clause, i - 1) <= _clause_literal(p_clause, i));
            };
            
            // validity of literals and flags
            if (_clause_size_is_aggregated(clause_size)) {
                _assert_level_1(_clause_flags(p_clause) != 0);
                _assert_level_1(clause_size < 1 || !literal_t__is_negation(_clause_literal(p_clause, 0)));
                _assert_level_1(clause_size < 2 || !literal_t__is_negation(_clause_literal(p_clause, 1)));
                _assert_level_1(clause_size < 3 || !literal_t__is_negation(_clause_literal(p_clause, 2)));
                _assert_level_1(clause_size < 4 || !literal_t__is_negation(_clause_literal(p_clause, 3)));
                _assert_level_1(clause_size != 1 || (_clause_flags(p_clause) & 0xFFFC) == 0);
                _assert_level_1(clause_size != 2 || (_clause_flags(p_clause) & 0xFFF0) == 0);
                _assert_level_1(clause_size != 3 || (_clause_flags(p_clause) & 0xFF00) == 0);
            } else {
                _assert_level_1(_clause_flags(p_clause) == 0);
            };
            
            // check if the clause exists by looking up the first literal index
            // find insertion pont at the same time
            // if avoid_merging is true, this is only used for verification
            if (!this->insertion_point_is_valid(insertion_point)) {
                _assert_level_1(!avoid_merging);
                find(p_clause, insertion_point);
            };
            
            // if a match for an aggregated clause is found, check if should
            // merge existing header from an immutable clause then still insert it
            // immediately in front of the existing clause
            // avoid_merging would ignore duplicate for any clause
            const container_offset_t existing_offset = insertion_point.container_offset;
            
            if (avoid_merging || existing_offset == CONTAINER_END || this->transaction_offset_is_immutable(existing_offset)) {
                // move the clause if necessary
                if (p_clause != _clauses_offset_clause(this->data_, this->size_)) {
                    this->reserve(_clauses_offset_size_memory_size(clause_size));
                    _clause_copy_size(p_clause, _clauses_offset_clause(this->data_, this->size_), clause_size);
                };
                // merge the existing clause into the new one if appropriate
                if (existing_offset != CONTAINER_END && _clause_size_is_aggregated(clause_size)) {
                    if (avoid_merging) {
                        // the existing clause should always be merged into the new one
                        _assert_level_1((_clauses_offset_flags(this->data_, existing_offset) & ~_clause_flags(p_clause)) == 0);
                    } else {
                        _clauses_offset_flags_include(this->data_, this->size_, _clauses_offset_flags(this->data_, existing_offset));
                    };
                };
                // clause not added yet but clauses_size_ is its valid offset
                base_t::_update(this->size_, insertion_point);
                insertion_point.kind = btipkCurrent;
                insertion_point.container_offset = this->size_;
                _clauses_offset_size_next(this->size_, clause_size); // "commits" the clause
            } else if (_clause_size_is_aggregated(clause_size)) {
                // a matching aggregated clause is found; it is enough to merge headers
                _assert_level_1(existing_offset != CONTAINER_END);
                _clauses_offset_flags_include(this->data_, existing_offset, _clause_flags(p_clause));
                // insertion point contains offset of the clause
            } else {
                // a duplicate for an existing non-aggregated clause - ignore
                // insertion point contains offset of the clause
            };
        };
        
        // normalizes and ensures no duplicates by performing the following:
        //   sorts the list of literals
        //   removes any duplicates
        //   if both a variable and its complement are present, ignores the clause
        //   for clauses size 2, 3, 4,
        //     make uncomplemented versions of literals and determine the clause flag
        //   looks for the clause with the same literals
        //     if subsumption determined, ignores the clause
        //   for clauses size 2, 3, 4
        //      merges the clause into existing or adds new if does not exist
        //   for all other clauses
        //      adds the clause unless its a duplicate
        inline void append_clause(const literalid_t* const literals, const clause_size_t literals_size) {
            this->reserve(_clauses_offset_size_memory_size(literals_size));
            uint32_t* const p_clause = _clauses_offset_clause(this->data_, this->size_);
            std::copy(literals, literals + literals_size, _clause_literals(p_clause));
            
            const clause_size_t clause_size = normalize_clause_literals(_clause_literals(p_clause), literals_size);
            clause_flags_t clause_flags = 0;
            
            // if necessary, unnegate the literals and calculate the aggregated flag
            if (_clause_size_is_aggregated(clause_size)) {
                uint16_t clause_bitmap = 0;
                for (auto i = 0; i < clause_size; i++) {
                    if (literal_t__is_unnegated(_clause_literal(p_clause, i))) {
                        clause_bitmap |= 0x1 << i;
                    } else {
                        literal_t__unnegate(_clause_literal(p_clause, i));
                    };
                };
                clause_flags = 0x1 << clause_bitmap;
            };
            _clause_header_set(p_clause, clause_flags, clause_size);
            
            insertion_point_t insertion_point;
            __insertion_point_t_init(insertion_point);
            append<false>(p_clause, insertion_point);
        };
        
        template<typename... Literals>
        inline void append_clause_l(Literals... literals) {
            constexpr auto n = sizeof...(literals);
            const literalid_t values[n] = { literals... };
            append_clause(values, n);
        };
    };
}

#endif /* cnfclausescontainer_hpp */
