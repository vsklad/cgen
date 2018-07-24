//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfprocessor_hpp
#define cnfprocessor_hpp

#include "variablesio.hpp"

//#define LOG_VARIABLE_ASSIGNMENTS
//#define LOG_VARIABLE_MAP_FINAL
//#define LOG_CLAUSE_EVALUATIONS

#ifdef LOG_VARIABLE_ASSIGNMENTS
#define LOG_VARIABLE_ASSIGNMENT(variable_id, value) \
    std::cout << "->: " << variable_id + 1 << " = " << literal_t(value) << std::endl;
#else
#define LOG_VARIABLE_ASSIGNMENT(variable_id, value)
#endif

#ifdef LOG_CLAUSE_EVALUATIONS
#define LOG_CLAUSE(action, offset) \
    std::cout << action << ": "; \
    __print_clause(offset);
#else
#define LOG_CLAUSE(action, offset)
#endif

#define DEBUG_ASSERT(expression) assert(expression)

namespace ple {
    
    // goes through all clauses sequentially
    // each clause is validated and assigned a flag whether its included into result, or not
    // satisfied clauses are excluded
    // changed clauses are appended to the end with originals excluded as well
    // therefore, optimization proceeds like a wave and stops
    // when all clauses (inclusive of newly generated ones) are validated
    class CnfOptimizer: public CnfProcessor {
    protected:
        enum evaluation_result_t { Undetermined, Satisfied, Conflict };

    protected:
        VariablesArray variables_;
        
    private:
        // each bit indicates if the clause
        vector<uint64_t> clauses_bitmap_;
        
        // clauses that are were processed already
        // when evaluating, processing of clauses can be postponed
        // reducing recursion and improving performance
        vector<uint32_t>::vector_size_t processed_offset_;
        
        uint64_t evaluations;
        
    private:
        inline void exclude_clause(const uint32_t offset) {
            const uint32_t bitmap_offset = offset >> 6;
            if (bitmap_offset >= clauses_bitmap_.size_) {
                clauses_bitmap_.append(0xFFFFFFFFFFFFFFFF, bitmap_offset - clauses_bitmap_.size_ + 1);
            };
            clauses_bitmap_.data_[bitmap_offset] &= ~(0x1ull << (offset & 0x3F));
        };
        
        inline const bool is_clause_included(const uint32_t offset) const {
            const uint32_t bitmap_offset = offset >> 6;
            if (bitmap_offset < clauses_bitmap_.size_) {
                return clauses_bitmap_.data_[bitmap_offset] & (0x1ull << (offset & 0x3F));
            } else {
                return true;
            };
        };
        
        inline const bool is_variable_used(const variableid_t variable_id) const {
            uint32_t index = var_clauses_.data_[variable_id];
            while (index != CLAUSES_END) {
                const uint32_t offset = var_clauses_index_.data_[index].offset;
                if (is_clause_included(offset)) {
                    return true;
                };
                index = var_clauses_index_.data_[index].next_item;
            };
            return false;
        };
        
        // DEBUG method
        inline void __print_clause(const uint32_t offset) const {
            std::cout << (is_clause_included(offset) ? "i" : "e") << ": ";
            Cnf::print_clause(std::cout, clauses_.data_ + offset, "; ");
            std::cout << std::endl;
        };
        
        // DEBUG method
        inline void __print_variable_clauses(const variableid_t variable_id) const {
            std::cout << "Variable clauses for " << variable_id + 1 << ":" << std::endl;
            uint32_t* p_index = var_clauses_.data_ + variable_id;
            while (*p_index != CLAUSES_END) {
                __print_clause(var_clauses_index_.data_[*p_index].offset);
                p_index = &(var_clauses_index_.data_[*p_index].next_item);
            };
        };
        
        inline static void reduce_clause_flags(uint16_t& flags, const clause_size_t index, const literalid_t value) {
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
        
        inline static void negate_clause_flags(uint16_t& flags, const clause_size_t index) {
            const constexpr uint16_t map0[4] = { 0x5555, 0x3333, 0x0F0F, 0x00FF };
            const constexpr uint16_t map1[4] = { 0xAAAA, 0xCCCC, 0xF0F0, 0xFF00 };
            const uint16_t shift = 1 << index;
            flags = ((flags & map1[index]) >> shift) | ((flags & map0[index]) << shift);
        };

        // insert into sorted position, merge flags for duplicates
        inline static void insert_last_literal_sorted(uint16_t& flags, clause_size_t& literals_size, literalid_t* const literals) {
            clause_size_t index = 0;
            while (index < literals_size) {
                if (literals[literals_size] <= literals[index]) {
                    break;
                };
                index++;
            };
            if (index == literals_size) {
                // new literal is the last one, nothing else to do
                literals_size++;
            } else if (literals[literals_size] == literals[index]) {
                // nothing to insert, transform flags
                switch((index << 4) | literals_size) {
                    case 0x01: // l0 = l1
                        flags = (flags & 0x0001) | (flags & 0x0018) >> 2 |
                            (flags & 0x0180) >> 4 | (flags & 0x1800) >> 6 | (flags & 0x8000) >> 8;
                        break;
                    case 0x02: // l0 = l2
                        flags = (flags & 0x0005) | (flags & 0x05A0) >> 4 | (flags & 0xA000) >> 8;
                        break;
                    case 0x03: // l0 = l3
                        flags = (flags & 0x0055) | (flags & 0xAA00) >> 8;
                        break;
                    case 0x12: // l1 = l2
                        flags = (flags & 0x0003) | (flags & 0x03C0) >> 4 | (flags & 0xC000) >> 8;
                        break;
                    case 0x13: // l1 = l3
                        flags = (flags & 0x0033) | (flags & 0xCC00) >> 8;
                        break;
                    case 0x23: // l2 = l3
                        flags = (flags & 0x000F) | (flags & 0xF000) >> 8;
                        break;
                };
            } else {
                // insert into index position and update flags
                switch((index << 4) | literals_size) {
                    case 0x01: // l1 = l0
                        flags = (flags & 0x9999) | (flags & 0x2222) << 1 | (flags & 0x4444) >> 1;
                        std::swap(literals[0], literals[1]);
                        break;
                    case 0x02: // l2 = l0
                        flags = (flags & 0x8181) |
                            (flags & 0x0202) << 1 | (flags & 0x0404) << 2 | (flags & 0x0808) << 3 |
                            (flags & 0x1010) >> 3 | (flags & 0x2020) >> 2 | (flags & 0x4040) >> 1;
                        std::swap(literals[1], literals[2]);
                        std::swap(literals[0], literals[1]);
                        break;
                    case 0x03: // l3 = l0
                        flags = (flags & 0x8001) |
                            (flags & 0x0002) << 1 | (flags & 0x0004) << 2 |
                            (flags & 0x0008) << 3 | (flags & 0x0010) << 4 |
                            (flags & 0x0020) << 5 | (flags & 0x0040) << 6 |
                            (flags & 0x0080) << 7 | (flags & 0x0100) >> 7 |
                            (flags & 0x0200) >> 6 | (flags & 0x0400) >> 5 |
                            (flags & 0x0800) >> 4 | (flags & 0x1000) >> 3 |
                            (flags & 0x2000) >> 2 | (flags & 0x4000) >> 1;
                        
                        std::swap(literals[2], literals[3]);
                        std::swap(literals[1], literals[2]);
                        std::swap(literals[0], literals[1]);
                        break;
                    case 0x12: // l2 = l1
                        flags = (flags & 0xC3C3) | (flags & 0x0C0C) << 2 | (flags & 0x3030) >> 2;
                        std::swap(literals[1], literals[2]);
                        break;
                    case 0x13: // l3 = l1
                        flags = (flags & 0xC003) |
                            (flags & 0x000C) << 2 | (flags & 0x0030) << 4 | (flags & 0x00C0) << 6 |
                            (flags & 0x0300) >> 6 | (flags & 0x0C00) >> 4 | (flags & 0x3000) >> 2;
                        std::swap(literals[2], literals[3]);
                        std::swap(literals[1], literals[2]);
                        break;
                    case 0x23: // l3 -> l2
                        flags = (flags & 0xF00F) | (flags & 0x00F0) << 4 | (flags & 0x0F00) >> 4;
                        std::swap(literals[2], literals[3]);
                        break;
                };
                literals_size++;
            };
        };
        
        // fix variable values for unary clauses, eliminate them
        // exclude clause before further analysis to avoid recursive loop
        // only update indexes if:
        //   clause changes - a literal change or a partial assignment
        // do not update variable indexes if:
        //   variable becomes or is constant
        inline evaluation_result_t evaluate_clause(const uint32_t offset) {
            evaluations++;
            
            clauses_.reserve((*(clauses_.data_ + offset) & 0xFFFF) + 1);
            uint32_t* p_new_clause = clauses_.data_ + clauses_.size_;
            literalid_t* new_literals = p_new_clause + 1;
            clause_size_t new_literals_size = 0;
            
            const uint32_t* const p_clause = clauses_.data_ + offset;
            const literalid_t* literals = p_clause + 1;
            const clause_size_t literals_size = *p_clause & 0xFFFF;
            const literalid_t* const var_values = variables_.data();
            
            bool is_clause_changed = false;
            evaluation_result_t result = Undetermined;
            
            // copy literals checking for constants and changes
            // if an aggregate, reduce/ensure literals are uncomplemented
            if (literals_size <= 4) {
                uint16_t flags = *p_clause >> 16;
                
                for (auto i = 0; i < literals_size; i++) {
                    new_literals[new_literals_size] = literal_t::resolve(var_values, literals[i]);
                    
                    if (literal_t__is_variable(new_literals[new_literals_size])) {
                        if (literal_t__is_negation(new_literals[new_literals_size])) {
                            negate_clause_flags(flags, new_literals_size);
                            new_literals[new_literals_size] |= 0x1;
                            is_clause_changed = true;
                        } else {
                            is_clause_changed |= (new_literals[new_literals_size] != literals[i]);
                        };
                        insert_last_literal_sorted(flags, new_literals_size, new_literals);
                    } else { // is_constant
                        reduce_clause_flags(flags, new_literals_size, new_literals[new_literals_size]);
                        is_clause_changed = true;
                    };
                };
                
                // save new clause size and flags
                *p_new_clause = new_literals_size | (flags << 16);
                 
                if (flags == 0) {
                    // all aggregated clauses satisfied
                    result = Satisfied;
                } else {
                    switch(new_literals_size) {
                        case 0:
                            result = Conflict;
                            break;
                        case 1:
                            if (flags == 0b11) {
                                result = Conflict;
                            } else {
                                exclude_clause(offset); // TODO: must exclude here in order to avoid/reduce recursion (?)
                                LOG_CLAUSE("e", offset);
                                result = assign_literal_value(new_literals[0], flags == 0b10);
                            };
                            break;
                        case 2:
                            if (is_clause_changed) {
                                // merge with existing clauses before evaluation
                                uint32_t existing_offset = cnf_.find_clause(p_new_clause);
                                if (existing_offset != CLAUSES_END) {
                                    flags |= clauses_.data_[existing_offset] >> 16;
                                };
                            };
                            
                            switch (flags) {
                                case 0b0110:
                                case 0b1001: {
                                    // found full equivalence
                                    const literalid_t lhs = var_values[literal_t__variable_id(new_literals[1])];
                                    const literalid_t rhs = literal_t__negated_onlyif(new_literals[0], flags == 0b1001);
                                    if (literal_t__is_constant(lhs) && literal_t__is_variable(rhs)) {
                                        // TODO: why is this possible????
                                        exclude_clause(offset); // TODO: must exclude here in order to avoid/reduce recursion (?)
                                        LOG_CLAUSE("e", offset);
                                        result = assign_literal_value(rhs, lhs);
                                    } else if (lhs != rhs) {
                                        exclude_clause(offset); // TODO: must exclude here in order to avoid/reduce recursion (?)
                                        LOG_CLAUSE("e", offset);
                                        result = assign_literal_value(new_literals[1], rhs);
                                    } else {
                                        result = Satisfied;
                                    };
                                    break;
                                };
                                case 0b0101:
                                case 0b1010:
                                    exclude_clause(offset); // TODO: must exclude here in order to avoid/reduce recursion (?)
                                    LOG_CLAUSE("e", offset);
                                    // first variable value is inferred
                                    result = assign_literal_value(new_literals[0], flags == 0b1010);
                                    break;
                                case 0b0011:
                                case 0b1100:
                                    exclude_clause(offset); // TODO: must exclude here in order to avoid/reduce recursion (?)
                                    LOG_CLAUSE("e", offset);
                                    // second variable value is inferred
                                    result = assign_literal_value(new_literals[1], flags == 0b1100);
                                    break;
                                case 0b0001: // !v0 V !v1
                                case 0b0010: // v0 V !v1
                                case 0b0100: // !v0 V v1
                                case 0b1000: // v0 V v1
                                    result = Undetermined;
                                    break;
                                case 0b0111:
                                case 0b1011:
                                case 0b1101:
                                case 0b1110: {
                                    // note new literals will be rewritten during the assignment
                                    // TODO: consider testing and maybe implementing
                                    // simultaneous setting of variables
                                    const literalid_t literal_id_1 = new_literals[1];
                                    exclude_clause(offset); // TODO: must exclude here in order to avoid/reduce recursion (?)
                                    LOG_CLAUSE("e", offset);
                                    result = assign_literal_value(new_literals[0], (flags & 0b1010) == 0b1010);
                                    if (result != Conflict) {
                                        result = assign_literal_value(literal_id_1, (flags & 0b1100) == 0b1100);
                                    };
                                    break;
                                };
                                case 0b1111:
                                    result = Conflict;
                                    break;
                            };
                            break;
                        default: // 3 or 4
                            result = Undetermined;
                            break;
                    };
                };
            } else {
                for (auto i = 0; i < literals_size; i++) {
                    new_literals[new_literals_size] = literal_t::resolve(var_values, literals[i]);
                    is_clause_changed |= new_literals[new_literals_size] != literals[i];
                    // count resolved literal if not a constant
                    if (literal_t__is_variable(new_literals[new_literals_size])) {
                        new_literals_size++;
                    } else if (literal_t__is_constant_1(new_literals[new_literals_size])) {
                        result = Satisfied;
                        break;
                    };
                };
                if (result != Satisfied) {
                    if (new_literals_size == 0) {
                        result = Satisfied;
                    } else if (is_clause_changed) {
                        *p_new_clause = cnf_.normalize_clause(new_literals, new_literals_size);
                        if (*p_new_clause == 0) {
                            result = Satisfied;
                        };
                    };
                };
            };
            
            if (result == Satisfied) {
                exclude_clause(offset);
            } else if (result == Undetermined && is_clause_changed) {
                exclude_clause(offset);
                cnf_.append_clause(p_new_clause);
                LOG_CLAUSE("e", offset);
                LOG_CLAUSE("->", (uint32_t)(p_new_clause - clauses_.data_));
            };
             
            return result;
        };
        
        inline evaluation_result_t assign_literal_value(const literalid_t literal_id, const literalid_t value) {
            return assign_variable_value(literal_t__variable_id(literal_id), value) == Conflict ? Conflict : Satisfied;
        };
        
        inline evaluation_result_t assign_variable_value(const variableid_t variable_id, const literalid_t value) {
            LOG_VARIABLE_ASSIGNMENT(variable_id, value);
            literalid_t* p_var_value = variables_.data() + variable_id;
   
            if (literal_t__is_variable(*p_var_value)) {
                // the new value must be different
                assert(*p_var_value != value);
                // can reference preceding variables only
                assert(!literal_t__is_variable(value) || literal_t__variable_id(value) <= variable_id);

                // assign the value immediately to prevent recursive evaluation
                *p_var_value = value;
                
                // go through all clauses that the variable is in
                // exclude the original clause
                // if not satisfied:
                //   remove the old clause from the indexes of all variables
                //   prepare a simplified copy
                //     note this may simplify the flags if an aggregate
                //   append afresh with validation check and merging
                //     note this may be an aggregate
                uint32_t next_item = var_clauses_.data_[variable_id];
                while (next_item != CLAUSES_END) {
                    const uint32_t offset = var_clauses_index_.data_[next_item].offset;
                    if (offset < processed_offset_ && is_clause_included(offset)) {
                        if (evaluate_clause(offset) == Conflict) {
                            return Conflict;
                        };
                        DEBUG_ASSERT(!is_clause_included(offset));
                    };
                    next_item = var_clauses_index_.data_[next_item].next_item;
                };
            } else {
                // the same constant can be assigned again while updating indexes recursively
                assert(*p_var_value == value);
            };
            
            return Undetermined; // assignment has not resulted in a conflict
        };
        
    protected:
        // reindex variables such that there are no gaps in variable numbers
        // update CNF variables count
        inline void update_variables(bool do_reindex) {
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
                    } else if (is_variable_used(variable_id)) {
                        if (do_reindex) {
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
            
            if (do_reindex) {
                set_variables_size(next_variable_id);
                var_clauses_.reset(next_variable_id);
                var_clauses_.append(CLAUSES_END, next_variable_id);
                var_clauses_index_.reset(0);
            };
            
#ifdef LOG_VARIABLE_MAP_FINAL
            std::cout << variables_ << std::endl;
#endif
        };
        
    private:
        // rebuild clauses
        // variables are reindexed at this stage
        // go through the clauses index and update variable values, then append clauses afresh
        // cannot overwrite because index sequence includes filtered data_ sequence plus new clauses
        void update_clauses() {
            literalid_t* var_values = variables_.data();
            const uint32_t old_clauses_size = clauses_.size_;
            clauses_.size_ = 0;
            
            uint32_t offset = 0;
            while (offset < old_clauses_size) {
                uint32_t* const p_clause = clauses_.data_ + offset;
                const uint32_t literals_size = *p_clause & 0xFFFF;
                
                if (is_clause_included(offset)) {
                    // append and rebuild indexes
                    if (literals_size > 4) {
                        // update variable values
                        for (auto p_literal = p_clause + 1; p_literal < p_clause + literals_size + 1; p_literal++) {
                            assert(literal_t__is_variable(*p_literal));
                            *p_literal = literal_t__lookup(var_values, *p_literal);
                        };
                        // check for duplicates and if always satisfied
                        *p_clause = cnf_.normalize_clause(p_clause + 1, literals_size);
                    } else {
                        uint16_t flags = *p_clause >> 16;
                        uint32_t* literals = p_clause + 1;
                        uint16_t new_literals_size = 0;
                        for (auto i = 0; i < literals_size; i++) {
                            assert(literal_t__is_variable(literals[i]));
                            literals[new_literals_size] = literal_t__lookup(var_values, literals[i]);
                            assert(literal_t__is_variable(literals[new_literals_size]));
                            
                            if (literal_t__is_negation(literals[new_literals_size])) {
                                negate_clause_flags(flags, new_literals_size);
                                literal_t__unnegate(literals[new_literals_size]);
                            };
                            insert_last_literal_sorted(flags, new_literals_size, literals);
                        };
                        
                        // save new clause size and flags
                        *p_clause = new_literals_size | (flags << 16);
                    };
                    
                    // this may be ok actually but need to verify if happens
                    assert(literals_size == (*p_clause & 0xFFFF));
                    
                    if ((*p_clause & 0xFFFF) > 0) {
                        cnf_.append_clause(p_clause);
                    };
                };
                
                offset += literals_size + 1;
            };
            
            clauses_bitmap_.reset(0); // all remaining clauses are included
        };
        
    protected:
        // process clauses one by one
        // the list will grow after each optimization
        // therefore the process will stop when consequences of all optimizations are evaluated
        inline evaluation_result_t evaluate_clauses() {
            evaluations = 0;
            
            uint32_t offset = 0;
            while (offset < clauses_.size_) {
                const clause_size_t literals_size = *(clauses_.data_ + offset) & 0xFFFF;
                // the clause may be excluded during processing of the preceding ones
                if (is_clause_included(offset)) {
                    processed_offset_ = offset;
                    if (evaluate_clause(offset) == Conflict) {
                        std::cout << "CONFLICT" << std::endl;
                        std::cout << "Clause(s):" << std::endl;
                        __print_clause(offset);
                        std::cout << "Variable(s):" << std::endl;
                        for (auto i = 0; i < literals_size; i++) {
                            const literalid_t literal_id = *(clauses_.data_ + offset + i + 1);
                            const variableid_t variable_id = literal_t__variable_id(literal_id);
                            const literalid_t value_id = variables_.data()[variable_id];
                            std::cout << literal_t(variable_t(variable_id)) << " = " << literal_t(value_id) << std::endl;
                        };
                        return Conflict;
                    };
                };
                offset += literals_size + 1;
            };
            
            std::cout << "Clause evaluations: " << std::dec << evaluations << std::endl;
            return Undetermined;
        };
        
        // apply all identified optimizations to the formula
        inline void apply_optimizations() {
            update_variables(true);
            update_clauses();
            cnf_.named_variables_update(variables_);
            clauses_bitmap_.reset(0); // all remaining clauses are included
        };
        
        inline const bool evaluate_optimize() {
            bool result = (evaluate_clauses() != Conflict);
            if (result) {
                apply_optimizations();
            };
            return result;
        };
        
    public:
        CnfOptimizer(Cnf& cnf): CnfProcessor(cnf), variables_(cnf_.variables_size()) {};
        
        // optimizes the given Cnf without any parameters
        virtual const bool execute() override {
            variables_.assign_sequence();
            
            const int64_t original_clauses_size = cnf_.clauses_size();
            
            bool result = evaluate_optimize();
            if (result) {
                std::cout << "Optimized: " << std::dec;
                std::cout << "(" << variables_.size() << ", " << original_clauses_size << ") -> ";
                std::cout << "(" << variables_.size() - cnf_.variables_size() << ",";
                std::cout << original_clauses_size - cnf_.clauses_size() << ") -> ";
                std::cout << "(" << cnf_.variables_size() << ", " << cnf_.clauses_size() << ")";
                std::cout << std::endl;
            };
            return result;
        };
    };
    
    // evaluates result variable given parameters
    // both parameters and result must correspond to existing named variables
    class CnfVariableEvaluator: public CnfOptimizer {
    private:
        inline const bool execute_() {
            cnf_.begin_transaction();

            bool result = (evaluate_clauses() != Conflict);
            if (result) {
                update_variables(false);
            };
            
            cnf_.rollback_transaction();
            return result;
        };
        
    public:
        CnfVariableEvaluator(Cnf& cnf): CnfOptimizer(cnf) {};
        
        virtual const bool execute(const char* parameters_name, const VariablesArray& parameters,
                      const char* result_name, VariablesArray &result_value) {
            variables_.assign_sequence();
            variables_.assign_template_from(cnf_.get_named_variables().at(parameters_name), parameters);
            bool result = execute_();
            if (result) {
                // compose result using the variable template and the result of the evaluation
                variables_.assign_template_into(cnf_.get_named_variables().at(result_name), result_value);
            };
            return result;
        };
        
        virtual const bool execute(VariablesArray& variables) {
            variables_.assign(variables);
            bool result = execute_();
            if (result) {
                variables.assign(variables_);
            };
            return result;
        };
    };
    
    // assigns named variabled after the CNF has been formed
    // removes satisfied clauses via propagation of the supplied variable values
    // updates all named variables to reflect constant assignments
    class CnfVariableAssigner: public CnfOptimizer {
    public:
        CnfVariableAssigner(Cnf& cnf): CnfOptimizer(cnf) {};
        
        virtual const bool execute(const char* name, const VariablesArray& value) {
            variables_.assign_sequence();
            variables_.assign_template_from(cnf_.get_named_variables().at(name), value);
            bool result = evaluate_optimize();
            if (result) {
                std::cout << "\"" << name << "\" assignment eliminated ";
                std::cout << std::dec << variables_.size() - cnf_.variables_size() << " variables" << std::endl;
            };
            return result;
        };
        
        virtual const bool execute(const VariablesArray& variables) {
            variables_.assign(variables);
            bool result = evaluate_optimize();
            if (result) {
                std::cout << "Assignment eliminated ";
                std::cout << std::dec << variables_.size() - cnf_.variables_size() << " variables" << std::endl;
            };
            return result;
        };
    };
    
    class CnfVariableNormalizer: public CnfOptimizer {
    public:
        CnfVariableNormalizer(Cnf& cnf): CnfOptimizer(cnf) {};
        
        virtual const bool execute() override {
            variables_.assign_sequence();
            cnf_.named_variables_assign_negations(variables_);
            apply_optimizations();
            
            // it is possible that some negated references remain
            // since the same variable may appear straight and completented
            // across named variables
            // for those, need to introduce new variables
            
            for (auto vit = named_variables_.begin(); vit != named_variables_.end(); vit++) {
                literalid_t* const template_ = vit->second.data();
                for (auto j = 0; j < vit->second.size(); j++) {
                    if (literal_t__is_variable(template_[j]) && literal_t__is_negation(template_[j])) {
                        literalid_t value = cnf_.variable_generator().new_variable_literal();
                        cnf_.append_clause_l(value, literal_t__negated(template_[j]));
                        cnf_.append_clause_l(literal_t__negated(value), template_[j]);
                        template_[j] = value;
                    };
                };
            };
            
            return true;
        };
    };
    
    inline const bool evaluate(Cnf& cnf, const char* parameters_name, const VariablesArray& parameters,
                         const char* result_name, VariablesArray &result) {
        return CnfVariableEvaluator(cnf).execute(parameters_name, parameters, result_name, result);
    };
    
    inline const bool assign(Cnf& cnf, const char* name, const VariablesArray& value) {
        return CnfVariableAssigner(cnf).execute(name, value);
    };
    
    inline const bool normalize_variables(Cnf& cnf) {
        return CnfVariableNormalizer(cnf).execute();
    };
};

#endif /* cnfprocessor_hpp */
