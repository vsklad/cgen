//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnf_hpp
#define cnf_hpp

#include <string>
#include <vector>
#include "assertlevels.hpp"
#include "variables.hpp"
#include "variablesarray.hpp"
#include "formula.hpp"
#include "cnfclausescontainer.hpp"

// TODO: review ; aaaaaaa
// 2) consider returning p_clause from iterators instead of offset
// 3) merging with non-processed offset - behaves very wierd
// 4) "triangles" should have special processing since they influence each other: c2 <-> c3 + c3;
//    start wirth c2 resolution; compare offsets to process once when encountered first time;
//    one way - ordered skip list for iterator (list meaning list); singleton for all lists??
// 5) c3 exclusion when subsuming - insertion point linking performs bad
// 6) c3+c2: if subsumable pairs exist for the other 2 variables, include those flags immediately to eliminate future merging?
// 9) review clause exclusion and use of the index remove
//  ) c3->c2 : checkbefore adding?

namespace bal {
    
    class Cnf: public Formula, protected CnfClausesIndexedContainer<COMPARE_CLAUSES_LEFT_RIGHT> {
    public:
        using clauses_container_t = CnfClausesIndexedContainer<COMPARE_CLAUSES_LEFT_RIGHT>;
        using insertion_point_t = typename clauses_container_t::insertion_point_t;
        
        friend class CnfProcessor;
        
    private:
        inline void __set_variables_size(const variableid_t value) {
            VariableGenerator::reset(value);
            // if value is higher, the index will be updated when needed
            // if the value is lower, simply disregard the unnecessary part
            this->instances_.size_ = value;
        };
        
    public:
        void initialize() override {
            Formula::initialize();
            resize(0, 0);
        };
         
        inline void resize(const variables_size_t variables_size, const clauses_size_t clauses_size) {
            clauses_container_t::reset(variables_size, clauses_size << 3); // set initial buffer with 8 words per clause
            VariableGenerator::reset(variables_size);
        };
        
    public:
        const uint32_t* const get_clause_data(const container_offset_t offset) const {
            _assert_level_0(offset < this->size_);
            return _clauses_offset_clause(this->data_, offset);
        };

        bool is_empty() const override { return this->size_ == 0; };
        bool is_compare_left_right() const { return COMPARE_CLAUSES_LEFT_RIGHT; };
        
        literalid_t get_variable_value(const variableid_t variableid) const override {
            uint32_t unit_clause[_clause_size_memory_size(1)] = _clause_c1_make(variable_t__literal_id(variableid));
            insertion_point_t insertion_point;
            __insertion_point_t_init(insertion_point);
            find(unit_clause, insertion_point);
            if (insertion_point.container_offset != CONTAINER_END) {
                return (_clauses_offset_flags(this->data_, insertion_point.container_offset) == 1) ? LITERAL_CONST_0 : LITERAL_CONST_1;
            } else {
                return LITERALID_UNASSIGNED;
            };
        };
        
    public:
        using clauses_container_t::append_clause;
        using clauses_container_t::append_clause_l;
        using clauses_container_t::clauses;
        using clauses_container_t::clauses_size;
        using clauses_container_t::find;
        using clauses_container_t::memory_size;
        
        void record_clauses(const char* const * const map, const std::size_t map_size,
                            literalid_t args[], const std::size_t input_size, const std::size_t output_size) {
            assert(map_size > 0 && input_size > 0 && output_size > 0);
            
            // create output literals
            for (auto i = 0; i < output_size; i++) {
                if (literal_t__is_unassigned(args[input_size + i])) {
                    args[input_size + i] = new_variable_literal();
                };
            };
            
            // buffer for clause literals
            literalid_t literalid_args[input_size + output_size];
            
            // record clauses one by one from the map
            for(auto i = 0; i < map_size; i++) {
                _assert_level_1(std::char_traits<char>::length(map[i]) == input_size + output_size);
                
                clause_size_t clause_size = 0;
                
                // record clause literals
                for (auto j = 0; j < input_size + output_size; j++) {
                    if (map[i][j] == '0' || map[i][j] == '1') {
                        literalid_args[clause_size++] = literal_t__negated_onlyif(args[j], map[i][j] == '0');
                    };
                };
                
                append_clause(literalid_args, clause_size);
            };
        };
    };
};

#endif /* cnf_hpp */
