//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnf_hpp
#define cnf_hpp

#include <vector>
#include "vector.hpp"
#include "variables.hpp"
#include "variablesarray.hpp"
#include "formula.hpp"
#include "cnfclauses.hpp"

namespace ple {
    class CnfProcessor;
    
    class Cnf: public Formula {
    public:
        class CnfVariableGenerator: public VariableGenerator { friend class Cnf; };
        
        friend class CnfProcessor;

        // ADD_NAIVE enforces encoding of simple binary addition with carry
        static const bool constexpr ADD_NAIVE_DEFAULT = false;
        
        // ADD_MAX_ARGS determines maximal number of arguments for an add expression
        // longer expressions are split into batches of the given lenth, last one can be shorter
        // ADD_MAX_ARGS is applied at bit level; it determines max number of variables
        // including any carry in bits, while any constants are optimized out
        // 6 arguments max supported, see cnfwordadd.hpp
        static const uint32_t constexpr ADD_MAX_ARGS_DEFAULT = 3;
        static const uint32_t constexpr ADD_MAX_ARGS_MIN = 2;
        static const uint32_t constexpr ADD_MAX_ARGS_MAX = 6;
        
        // XOR_MAX_ARGS determines maximal number of arguments for a xor expression
        // longer expressions are split into batches of the given lenth max, last one can be shorter
        // default value of 3 is chosen to minimize both variables and clauses
        // 10 means 2^10 = 1K clauses per expression - seems enough
        static const uint32_t constexpr XOR_MAX_ARGS_DEFAULT = 3;
        static const uint32_t constexpr XOR_MAX_ARGS_MIN = 2;
        static const uint32_t constexpr XOR_MAX_ARGS_MAX = 10;
        
    private:
        // the buffer is a sequence of 32 bit words
        // each clause is a sequence of words
        // first word is a header; it is followed by a sequence of sorted variable/literal IDs
        // lowest 2 bytes of the clause header define clause size (number of literals)
        // higher 2 bytes of the header are clause-dependent flags
        // clauses size 2, 3, 4 (aggregated):
        //   variable ids are used
        //   flags identify which variable combinations are present
        // other clauses: flags not used, composed of literal ids
        vector<uint32_t> clauses_;

        // variable clauses index
        // initial clauses list entry for each variable
        vector<uint32_t> var_clauses_;
        // array with lists of clauses for each variable
        vector<clauses_index_item_t> var_clauses_index_;
   
        // no clauses can be modified below this offset
        // 0 means no baseline
        // new aggregated clauses are produced above this offset only
        vector<uint32_t>::vector_size_t baseline_size_;
        
        CnfVariableGenerator variable_generator_;
        
        // generation options
        uint32_t add_max_args_;
        uint32_t xor_max_args_;
        bool add_naive_;
        
    private:
        inline static int compare_clauses_i(const uint32_t* lhs, const uint32_t* rhs) {
            const clause_size_t lhs_size = *lhs & 0xFFFF;
            const clause_size_t rhs_size = *rhs & 0xFFFF;
            const clause_size_t common_size = lhs_size > rhs_size ? rhs_size : lhs_size;
            
            lhs++;
            rhs++;
            
            for (auto i = 0; i < common_size; i++) {
                if (lhs[i] != rhs[i]) {
                    return lhs[i] < rhs[i] ? -1 : 1;
                };
            };
            // if all the corresponding literals match then the shorter clause is smaller
            return lhs_size < rhs_size ? -1 : (lhs_size == rhs_size ? 0 : 1);
        };

        inline static uint16_t get_cardinality_uint16(const uint16_t value) {
                                    // 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
            const uint16_t map[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
            return map[value & 0xF] + map[value >> 4 & 0xF] +
                map[value >> 8 & 0xF] + map[value >> 12 & 0xF];
        };
        
        // assume the clause is new
        // search for the same clause
        //    if found:
        //      if not the first variable, the index is corrupted
        //      if first variable, merge and skip updating other variables indexes
        //    if not found:
        //      the clause is new, it should not exist for any other variables
        inline void append_clause_i() {
            uint32_t* const p_clause = clauses_.data_ + clauses_.size_;
            literalid_t* const literals = p_clause + 1;
            const clause_size_t literals_size = *p_clause & 0xFFFF;
            
            assert(literals_size != 0);
            
            // should be sorted at this stage
            // check is sorted; remove later for performance reasons
            for (auto i = 1; i < literals_size; i++) {
                assert(*(p_clause + i) <= *(p_clause + i + 1));
            };
            
            // header + uncomplemented literals if needed
            if (literals_size <= 4) {
                if ((*p_clause & 0xFFFF0000) == 0) {
                    uint16_t clause_bitmap = 0;
                    for (auto i = 0; i < literals_size; i++) {
                        if (literals[i] & 0x1) {
                            clause_bitmap |= 0x1 << i;
                        } else {
                            literals[i] |= 0x1;
                        };
                    };
                    *p_clause = literals_size | (0x1 << (16 + clause_bitmap));
                } else {
                    if (literals_size == 1) assert((*p_clause & 0xFFFC0000) == 0);
                    if (literals_size == 2) assert((*p_clause & 0xFFF00000) == 0);
                    if (literals_size == 3) assert((*p_clause & 0xFF000000) == 0);
                };
            };
            
            // ensure proper variable_clauses_ size
            // last literal has the highest variable id
            variableid_t variable_id = literal_t(literals[literals_size - 1]).variable_id();
            if (variable_id >= var_clauses_.size_) {
                var_clauses_.append(CLAUSES_END, variable_id - var_clauses_.size_ + 1);
            };
            
            // append to clauses list for each literal
            for (auto i = 0; i < literals_size; i++) {
                variableid_t variable_id = literal_t(literals[i]).variable_id();
                
                // search for the same clause
                uint32_t* p_index = var_clauses_.data_ + variable_id;
                int compare_result = 1; // in case the list is empty
                while (*p_index != CLAUSES_END) {
                    compare_result = compare_clauses_i(clauses_.data_ + var_clauses_index_.data_[*p_index].offset, clauses_.data_ + clauses_.size_);
                    if (compare_result >= 0) {
                        break;
                    };
                    p_index = &(var_clauses_index_.data_[*p_index].next_item);
                };
                
                if (compare_result == 0) {
                    // a matching clause is found
                    if (literals_size <= 4) {
                        const uint32_t offset = var_clauses_index_.data_[*p_index].offset;
                        if (offset < baseline_size_) {
                            if (i == 0) {
                                *p_clause |= clauses_.data_[offset]; // merge existing header
                            };
                        } else {
                            assert(i == 0);
                            clauses_.data_[offset] |= *p_clause; // it is enough to merge headers
                            // no need to continue since the clause is merged into an existing one
                            return;
                        };
                    } else {
                        // a duplicate for an existing non-aggregated clause, just ignore
                        assert(i == 0); // it should be present for all variables including the first one
                        return;
                    };
                };
                
                // there is no matching clause; insert before *p_index
                // (important to insert before to support baseline/transactions)
                const uint32_t next_item = *p_index;
                *p_index = var_clauses_index_.size_; // the pointer can be invalidated by reserve()
                var_clauses_index_.reserve(1);
                clauses_index_item_t& new_item = var_clauses_index_.data_[var_clauses_index_.size_];
                new_item.offset = clauses_.size_;
                new_item.next_item = next_item;
                var_clauses_index_.size_++; // commits the index entry
            };
            
            clauses_.size_ += literals_size + 1; // "commits" the clause
        };
        
        inline void set_variables_size(const variableid_t value) {
            variable_generator_.reset(value);
            var_clauses_.reset(value);
        };
        
    public:
        Cnf(): Cnf(0, 0) {};
        Cnf(const variables_size_t variables_size, const clauses_size_t clauses_size) {
            initialize(variables_size, clauses_size);
        };
        
        // reset internal structures and resize
        inline void initialize(const variables_size_t variables_size, const clauses_size_t clauses_size) {
            Formula::initialize();
            variable_generator_.reset(variables_size);
            clauses_.reset(clauses_size << 2); // set initial buffer with 4 words per clause
            var_clauses_.reset(variables_size); // assume number of variables is correct
            var_clauses_.append(CLAUSES_END, variables_size); // CLAUSES_END means unassigned
            var_clauses_index_.reset(clauses_size << 2); // again, estimate for number of literals
            baseline_size_ = 0;
            add_max_args_ = ADD_MAX_ARGS_DEFAULT;
            xor_max_args_ = XOR_MAX_ARGS_DEFAULT;
            add_naive_ = ADD_NAIVE_DEFAULT;
        };

        const uint32_t* const data() const { return clauses_.data_; };
        const uint32_t data_size() const { return clauses_.size_; };
        
        virtual const variables_size_t variables_size() const override { return variable_generator_.next(); };
        const uint32_t literals_size() const { return clauses_.size_ - clauses_size(); };

        virtual const bool is_empty() const override { return clauses_.size_ == 0; };
        
        const clauses_size_t clauses_size() const {
            clauses_size_t result = 0;
            uint32_t offset = 0;
            while (offset < clauses_.size_) {
                const clause_size_t literals_size = *(clauses_.data_ + offset) & 0xFFFF;
                if(literals_size <= 4) {
                    result += get_cardinality_uint16(*(clauses_.data_ + offset) >> 16);
                } else {
                    result ++;
                };
                offset += literals_size + 1;
            };
            return result;
        };
        
        inline VariableGenerator& variable_generator() { return variable_generator_; };

        inline bool get_add_naive() const { return add_naive_; };
        inline void set_add_naive(const bool value) { add_naive_ = value; };
        
        inline uint32_t get_add_max_args() const { return add_max_args_; };
        inline void set_add_max_args(const uint32_t value) {
            assert(value >= ADD_MAX_ARGS_MIN && value <= ADD_MAX_ARGS_MAX);
            add_max_args_ = value;
        };
        
        inline uint32_t get_xor_max_args() const { return xor_max_args_; };
        inline void set_xor_max_args(const uint32_t value) {
            assert(value >= XOR_MAX_ARGS_MIN && value <= XOR_MAX_ARGS_MAX);
            xor_max_args_ = value;
        };
        
        // returns offset of the clause if found, otherwise CLAUSES_END
        // returns the first matching clause from the index
        inline const uint32_t find_clause(const uint32_t* const p_clause) const {
            assert((*p_clause & 0xFFFF) > 0);

            // only check the first variable index
            uint32_t* p_index = var_clauses_.data_ + literal_t__variable_id(*(p_clause + 1));
            while (*p_index != CLAUSES_END) {
                const int compare_result = compare_clauses_i(clauses_.data_ + var_clauses_index_.data_[*p_index].offset, p_clause);
                if (compare_result == 0) {
                    return var_clauses_index_.data_[*p_index].offset;;
                } else if (compare_result > 0) {
                    break;
                };
                p_index = &(var_clauses_index_.data_[*p_index].next_item);
            };
            return CLAUSES_END;
        };
        
        // returns 0 if the clause is always satisfied; otherwise returns the new size
        inline static const clause_size_t normalize_clause(uint32_t* const literals, const clause_size_t literals_size) {
            assert(literals_size > 0);
            
            std::sort(literals, literals + literals_size);
            
            assert(literal_t__is_variable(literals[0]));
            clause_size_t validated_size = 1;
            for (auto i = 1; i < literals_size; i++) {
                assert(literal_t__is_variable(literals[i]));
                if (literals[i] == literals[validated_size - 1]) {
                    //warning(literals_size, literals, "ignoring duplicate literal");
                    continue;
                } else if ((literals[i] ^ 0x1) == literals[validated_size - 1]) {
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
            clauses_.reserve(literals_size + 1);
            clauses_.data_[clauses_.size_] = literals_size; // no flags
            literalid_t* const dst_literals = clauses_.data_ + clauses_.size_ + 1;
            std::copy(literals, literals + literals_size, dst_literals);
            clauses_.data_[clauses_.size_] = normalize_clause(dst_literals, literals_size);
            append_clause_i();
        };
        
        // assume the clause is normalized
        //   i.e. literals are sorted, no duplicates etc
        inline void append_clause(const uint32_t* p_clause) {
            if (p_clause != clauses_.data_ + clauses_.size_) {
                const clause_size_t literals_size = *p_clause & 0xFFFF;
                clauses_.reserve(literals_size + 1);
                std::copy(p_clause, p_clause + literals_size + 1, clauses_.data_ + clauses_.size_);
            };
            append_clause_i();
        };
        
        template<typename... Literals>
        inline void append_clause_l(Literals... literals) {
            constexpr auto n = sizeof...(literals);
            const literalid_t values[n] = { literals... };
            append_clause(values, n);
        };
        
        inline static void print_clause(std::ostream& stream, const uint32_t* const p_clause, const char* final_token = nullptr) {
            uint16_t clauses_bitmap = *p_clause >> 16;
            const clause_size_t literals_size = (*p_clause & 0xFFFF);
            
            if (literals_size <= 4 && clauses_bitmap != 0) {
                uint16_t clause_bitmap = 0;
                while (clauses_bitmap > 0) {
                    if (clauses_bitmap & 1) {
                        for (auto i = 0; i < literals_size; i++) {
                            literal_t literal(*(p_clause + i + 1));
                            literal.negate((clause_bitmap & (1 << i)) == 0);
                            stream << ((i > 0) ? " " : "") << literal;
                        };
                        
                        if (final_token != nullptr) {
                            stream << final_token;
                        };
                    };
                    clause_bitmap++;
                    clauses_bitmap >>= 1;
                };
            } else {
                for (auto i = 0; i < literals_size; i++) {
                    stream << ((i > 0) ? " " : "") << literal_t(*(p_clause + i + 1));
                };
                
                if (final_token != nullptr) {
                    stream << final_token;
                };
            };
        };
        
        clauses_sequence_sorted_t sorted_clauses() const { return clauses_sequence_sorted_t(clauses_, var_clauses_, var_clauses_index_); };
        
        // transactions
        // TODO: support nested transactions
        
        inline void begin_transaction() {
            assert(baseline_size_ == 0);
            baseline_size_ = clauses_.size_;
        };
        
        inline void commit_transaction() {
            baseline_size_ = 0;
        };
        
        inline void rollback_transaction() {
            assert(clauses_.size_ == 0 || baseline_size_ > 0);
            // shrink clauses
            clauses_.size_ = baseline_size_;
            // exclude any clause indexes that point beyond the current size
            const variables_size_t var_size = var_clauses_.size_;
            for (variableid_t variable_id = 0; variable_id < var_size; variable_id++) {
                uint32_t* p_next_item =  var_clauses_.data_ + variable_id;
                while (*p_next_item != CLAUSES_END) {
                    const uint32_t offset = var_clauses_index_.data_[*p_next_item].offset;
                    if (offset >= baseline_size_) {
                        // exclude the index entry
                        *p_next_item = var_clauses_index_.data_[*p_next_item].next_item;
                    } else {
                        p_next_item = &(var_clauses_index_.data_[*p_next_item].next_item);
                    };
                };
            };
            baseline_size_ = 0;
        };
    };
    
    class CnfProcessor {
    protected:
        Cnf& cnf_;
        
    protected:
        vector<uint32_t>& clauses_;
        vector<uint32_t>& var_clauses_;
        vector<clauses_index_item_t>& var_clauses_index_;
        formula_named_variables_t& named_variables_;
        
        inline void set_variables_size(const variableid_t value) {
            cnf_.set_variables_size(value);
        };
        
    public:
        CnfProcessor(Cnf& cnf): cnf_(cnf), clauses_(cnf.clauses_),
            var_clauses_(cnf.var_clauses_), var_clauses_index_(cnf.var_clauses_index_),
            named_variables_(cnf_.get_named_variables_()) {};
        
        virtual const bool execute() = 0;
    };
};

#endif /* cnf_hpp */
