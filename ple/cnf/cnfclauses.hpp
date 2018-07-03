//
//  Propositional Logic Engine (PLE) Library
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfclauses_hpp
#define cnfclauses_hpp

#include "vector.hpp"
#include "variables.hpp"

namespace ple {
    
    typedef uint16_t clause_size_t;
    typedef uint32_t clauses_size_t;
    typedef variableid_t variables_size_t;
    
    // maximal supported length of the clause (number of literals)
    // due to clause header
    static const clause_size_t constexpr CLAUSE_SIZE_MAX = UINT16_MAX;
    // maximal theoretically supported number of clauses
    // due to clause index
    static const clauses_size_t constexpr CLAUSES_SIZE_MAX = UINT32_MAX - 1;
    static const clauses_size_t constexpr CLAUSES_END = UINT32_MAX;
    static const variables_size_t constexpr VARIABLES_SIZE_MAX = VARIABLEID_MAX;
    
    // uni-directional list of clauses
    typedef struct { uint32_t next_item; uint32_t offset; } clauses_index_item_t;
    
    inline const int compare_clauses(const uint32_t* lhs, const uint32_t* rhs) {
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
    
    // the following two classes implement sorted iterator over clauses lists
    // based on variable clauses index
    // maintains position throughout changes
    
    class clauses_iterator_sorted_t {
    private:
        const vector<uint32_t>& clauses_;
        const vector<uint32_t>& var_clauses_;
        const vector<clauses_index_item_t>& var_clauses_index_;
        clauses_size_t var_index_;
        clauses_size_t var_clauses_index_offset_;
        
    private:
        void first_var_clause() {
            var_clauses_index_offset_ = CLAUSES_END;
            while (var_index_ + 1 < var_clauses_.size_) {
                var_clauses_index_offset_ = var_clauses_.data_[var_index_];
                while ((var_clauses_index_offset_ != CLAUSES_END) && (literal_t__variable_id(*(operator*() + 1)) < var_index_)) {
                    var_clauses_index_offset_ = var_clauses_index_.data_[var_clauses_index_offset_].next_item;
                };
                if (var_clauses_index_offset_ != CLAUSES_END) {
                    break;
                };
                var_index_++;
            };
        };
        
        void next() {
            if (var_clauses_index_offset_ != CLAUSES_END) {
                var_clauses_index_offset_ = var_clauses_index_.data_[var_clauses_index_offset_].next_item;
            };

            if (var_clauses_index_offset_ == CLAUSES_END) {
                var_index_++;
                first_var_clause();
            };
        };
        
    public:
        clauses_iterator_sorted_t(const vector<uint32_t>& clauses, const vector<uint32_t>& var_clauses,
            const vector<clauses_index_item_t>& var_clauses_index): clauses_(clauses),
            var_clauses_(var_clauses), var_clauses_index_(var_clauses_index) {};
        
        void set_begin() {
            var_index_ = 0;
            first_var_clause();
        };
        
        void set_end() {
            var_index_ = var_clauses_.size_ - 1;
            var_clauses_index_offset_ = CLAUSES_END;
        };

        uint32_t* operator *() {
            if (var_clauses_index_offset_ != CLAUSES_END) {
                return clauses_.data_ + var_clauses_index_.data_[var_clauses_index_offset_].offset;
            } else {
                return clauses_.data_ + clauses_.size_;
            };
        };
        
        inline clauses_iterator_sorted_t& operator ++() {
            next();
            return *this;
        };
        
        inline clauses_iterator_sorted_t operator ++(int) {
            clauses_iterator_sorted_t result = *this;
            next();
            return result;
        };
        
        inline bool operator == (const clauses_iterator_sorted_t& rhs) const {
            return var_clauses_index_offset_ == rhs.var_clauses_index_offset_;
        };
        
        inline bool operator != (const clauses_iterator_sorted_t& rhs) const {
            return !(*this == rhs);
        };
    };
    
    class clauses_sequence_sorted_t {
    private:
        clauses_iterator_sorted_t begin_;
        clauses_iterator_sorted_t end_;
        
    public:
        using iterator = clauses_iterator_sorted_t;
        
        clauses_sequence_sorted_t(const vector<uint32_t>& clauses, const vector<uint32_t>& var_clauses,
            const vector<clauses_index_item_t>& var_clauses_index):
            begin_(clauses, var_clauses, var_clauses_index), end_(clauses, var_clauses, var_clauses_index) {
            begin_.set_begin();
            end_.set_end();
        };
        
        iterator begin() const {
            return begin_;
        };
        
        iterator end() const {
            return end_;
        };
    };
};

#endif /* cnfclauses_hpp */
