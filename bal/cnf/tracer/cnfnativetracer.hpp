//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfnativetracer_hpp
#define cnfnativetracer_hpp

#include <vector>
#include "cnfstreamtracer.hpp"

namespace bal {
    
    class CnfNativeTracer: public CnfStreamTracer {
    public:
        typedef uint16_t option_t;
        typedef option_t options_t;
        
        static constexpr option_t TRACE_VARIABLE_ASSIGNMENTS      = 0b00000001;
        static constexpr option_t TRACE_CLAUSE_EVALUATIONS        = 0b00000010;
        static constexpr option_t TRACE_VARIABLE_MAP_FINAL        = 0b00000100;
        static constexpr option_t TRACE_TRANSITIVE_REDUCTION      = 0b00001000;
        
        static constexpr options_t TRACE_EVERYTHING   = 0b11111111;
        static constexpr options_t TRACE_NONE         = 0b00000000;
        static constexpr options_t TRACE_DEFAULT      = TRACE_EVERYTHING;
        
    private:
        const options_t options_;
        const Cnf* p_cnf_ = nullptr;
        
        std::vector<const uint32_t*> clauses_;
        std::vector<uint32_t> offsets_;
        
    private:
        inline void write_action(const char action, const int depth = 0) {
            const int total_depth = int(offsets_.size()) + depth + 1;
            stream << std::setfill('.') << std::setw(total_depth) << action;
        };
        
        inline void write_clause(const uint32_t* p_clause) const {
            stream << ' ';
            print_clause(stream, p_clause, "; ");
        };
        
        inline void write_current_clause_action(const char action, const uint32_t* const p_clause) {
            if (options_ & TRACE_CLAUSE_EVALUATIONS && p_cnf_ != nullptr) {
                _assert_level_0(p_clause != nullptr);
                write_action(action, 1);
                if (!is_current_clause(p_clause, CONTAINER_END)) {
                    write_clause(p_clause);
                };
                stream << std::endl;
            };
        };
        
        // assume clause is always defined and offset may be undefined
        inline void set_current_clause(const uint32_t* const p_clause, const container_offset_t offset) {
            _assert_level_0(!offsets_.empty() && !clauses_.empty());
            _assert_level_0(p_clause != nullptr || offset == CONTAINER_END);
            clauses_.back() = p_clause;
            offsets_.back() = offset;
        };
        
        // match if either offset or the pointer is defined and matches as below
        inline bool is_current_clause(const uint32_t* const p_clause, const container_offset_t offset) {
            _assert_level_0(!offsets_.empty() && !clauses_.empty());
            if (p_clause == nullptr || clauses_.back() == nullptr || p_clause != clauses_.back()) {
                return (offset != CONTAINER_END && offset == offsets_.back());
            } else {
                return (offset == CONTAINER_END || offsets_.back() == CONTAINER_END || offset == offsets_.back());
            };
        };
        
    public:
        void start(const Cnf& cnf) override {
            _assert_level_0(p_cnf_ == nullptr);
            _assert_level_2(offsets_.empty() && clauses_.empty());
            p_cnf_ = &cnf;
            level_next();
        };

        void finish() override {
            level_prev();
            _assert_level_2(offsets_.empty() && clauses_.empty());
            p_cnf_ = nullptr;
        };
        
        void level_next() override {
            offsets_.push_back(CONTAINER_END);
            clauses_.push_back(nullptr);
        };
        
        void level_prev() override {
            offsets_.pop_back();
            clauses_.pop_back();
        };
        
        void assign_variable(const variableid_t variable_id, const literalid_t value) override {
            if (options_ & TRACE_VARIABLE_ASSIGNMENTS && p_cnf_ != nullptr) {
                write_action('v');
                stream << ' ' << literal_t(variable_t__literal_id(variable_id)) << " = " << literal_t(value) << std::endl;
            };
        };
        
        void update_variables(const VariablesArray& variables) override {
            if (options_ & TRACE_VARIABLE_MAP_FINAL) {
                for (variables_size_t i = 0; i < variables.size(); i++) {
                    if (i != literal_t__variable_id(variables.data()[i])) {
                        stream << "v: " << i + 1 << " -> " << literal_t(variables.data()[i]) << std::endl;
                    };
                };
            };
        };
        
        void assign_literal(const literalid_t original_value, const literalid_t assigned_value) override {
            if (options_ & TRACE_CLAUSE_EVALUATIONS && p_cnf_ != nullptr) {
                write_action('+', 1);
                stream << ' ' << literal_t(original_value) << " = " << literal_t(assigned_value) << std::endl;
            };
        };
        
        void process_clause(const uint32_t* const p_clause, const container_offset_t offset, const bool is_resolvent) override {
            if (options_ & TRACE_CLAUSE_EVALUATIONS && p_cnf_ != nullptr) {
                if (!is_current_clause(p_clause, offset)) {
                    set_current_clause(p_clause, offset);
                    const char action = (is_resolvent) ? 'r' :
                        (clauses_.size() > 1 && clauses_[clauses_.size() - 2] != nullptr) ? '+' : 'e';
                    write_action(action);
                    write_clause(p_clause);
                    stream << std::endl;
                } else if (offset == offsets_.back() && p_clause != clauses_.back()) {
                    // changing p_clause with the same offset is fine
                    _assert_level_0(p_clause != nullptr);
                    set_current_clause(p_clause, offset);
                } else {
                    _assert_level_0(clauses_.size() > 0 && clauses_.back() == p_clause);
                    set_current_clause(p_clause, offset);
                    write_action('u', 1);
                    write_clause(p_clause);
                    stream << std::endl;
                };
            };
        };
        
        void append_clause(const uint32_t* const p_clause) override {
            write_current_clause_action('a', p_clause);
        };
        
        void merge_clause(const uint32_t* const p_clause, const container_offset_t offset) override {
            if (options_ & TRACE_CLAUSE_EVALUATIONS && p_cnf_ != nullptr) {
                _assert_level_0(is_current_clause(p_clause, CONTAINER_END));
                _assert_level_0(offset != CONTAINER_END);
                write_action('m', 1);
                write_clause(p_cnf_->get_clause_data(offset));
                stream << std::endl;
            };
        };
        
        void remove_clause(const container_offset_t offset) override {
            if (options_ & TRACE_CLAUSE_EVALUATIONS && p_cnf_ != nullptr) {
                _assert_level_0(offset != CONTAINER_END);
                if (is_current_clause(nullptr, offset)) {
                    write_current_clause_action('x',  clauses_.back());
                    set_current_clause(clauses_.back(), CONTAINER_END);
                } else {
                    write_current_clause_action('x', p_cnf_->get_clause_data(offset));
                };
            };
        };
        
    public:
        CnfNativeTracer(std::ostream& stream, const options_t options = TRACE_DEFAULT):
            CnfStreamTracer(stream), options_(options) {};
    };
    
};

#endif /* cnfnativetracer_hpp */
