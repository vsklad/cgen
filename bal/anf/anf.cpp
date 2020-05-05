//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include "anf.hpp"
#include "variablesio.hpp"

namespace bal {

    void Anf::initialize() {
        Formula::initialize();
        symbols_.clear();
        terms_.clear();
        equations_.clear();
    };
    
    // check that the data structures are valid
    void Anf::is_incomplete_last_equation_() const {
        assert(equations_.size() > 0);
        assert(terms_size_(equations_.size() - 1) > 0);
        assert(symbols_size_(equations_[equations_.size() - 1]) == 1);
        assert(literal_t__is_constant(symbols_[terms_[equations_[equations_.size() - 1]]]));
    };
    
    void Anf::remove_last_equation_() {
        is_incomplete_last_equation_();
        size_t terms_index = equations_[equations_.size() - 1];
        symbols_.erase(symbols_.begin() + terms_[terms_index], symbols_.end());
        terms_.erase(terms_.begin() + terms_index, terms_.end());
        equations_.erase(equations_.end() - 1);
    };
    
    void Anf::last_equation_remove_term_(const size_t term_index) {
        is_incomplete_last_equation_();
        assert(equations_[equations_.size() - 1] <= term_index && term_index < terms_.size());
        const size_t term_symbols_size = symbols_size_(term_index);
        // remove symbols
        symbols_.erase(symbols_.begin() + terms_[term_index],
                       symbols_.begin() + terms_[term_index] + term_symbols_size);
        // remove the term
        terms_.erase(terms_.begin() + term_index);
        // reindex remaining terms if any
        for (auto i = term_index; i < terms_.size(); i++) {
            terms_[i] -= term_symbols_size;
        };
    };
    
    void Anf::append_equation() {
        assert(terms_.size() < UINT32_MAX);
        equations_.push_back((uint32_t)terms_.size());
        // add the constant term
        terms_.push_back((uint32_t)symbols_.size());
        symbols_.push_back(literal_t__constant(0));
    };

    void Anf::append_equation_term(const literalid_t* const symbols, const std::size_t symbols_size) {
        is_incomplete_last_equation_();
        const size_t equation_terms_size = terms_size_(equations_.size() - 1);
        
        // while 1, the term is valid even if becomes 1
        literalid_t constant = literal_t__constant(1);
        
        // validate the term itself
        // build sorted array of unique variable numbers
        // the rest is optimized out
        literalid_t validated_symbols[symbols_size];
        std::size_t validated_symbols_size = 0;
        std::size_t first_negation_index = symbols_size;
        
        for (auto i = 0; i < symbols_size && literal_t__is_constant_1(constant); i++) {
            if (literal_t__is_constant_0(symbols[i])) {
                constant = literal_t__constant(0);
                validated_symbols_size = 0;
                break;
            } else if (literal_t__is_constant_1(symbols[i])) {
                continue;
            } else {
                // adjust number of variables if necessary
                if (VariableGenerator::next() <= literal_t__variable_id(symbols[i])) {
                    VariableGenerator::reset(literal_t__variable_id(symbols[i]) + 1);
                };
                
                unsigned int j = 0;
                while (j < validated_symbols_size) {
                    if (literal_t__is_negation_of(symbols[i], validated_symbols[j])) {
                        // x & ~x == 0
                        constant = literal_t__constant(0);
                        validated_symbols_size = 0;
                        break;
                    } else if (symbols[i] == validated_symbols[j]) {
                        // x & x == x
                        break;
                    } else if (symbols[i] > validated_symbols[j]) {
                        // found the sorted insertion point
                        break;
                    };
                    j++;
                };
                if (j == validated_symbols_size || symbols[i] > validated_symbols[j]) {
                    if (j < validated_symbols_size) {
                        j++;
                        for (auto k = j; k < validated_symbols_size; k++) {
                            validated_symbols[k + 1] = validated_symbols[k];
                        };
                    };
                    validated_symbols[j] = symbols[i];
                    validated_symbols_size++;
                    
                    if (literal_t__is_negation(validated_symbols[j]) && first_negation_index > j) {
                        first_negation_index = j;
                    };
                };
            };
        };
        
        if (literal_t__is_constant_1(constant)) {
            if (validated_symbols_size > 0) {
                // handle negations
                // the first negation should be processed recursively replacing the negated variable with 1
                // then the second time unnegated
                if (first_negation_index < validated_symbols_size) {
                    literalid_t first_negated_literal = validated_symbols[first_negation_index];
                    validated_symbols[first_negation_index] = literal_t__constant(1);
                    append_equation_term(validated_symbols, validated_symbols_size); // recursive call
                    validated_symbols[first_negation_index] = literal_t__unnegated(first_negated_literal);
                    append_equation_term(validated_symbols, validated_symbols_size); // recursive call
                } else {
                    // now there is a sorted list of unique unnegated literals
                    // go through existing terms and check if there is a matching one
                    size_t term_index = terms_.size();
                    if (equation_terms_size > 1) {
                        const auto first_term_index = equations_[equations_.size() - 1] + 1; // skip the constant
                        for (auto i = first_term_index; i < first_term_index + equation_terms_size - 1; i++) {
                            const auto term_symbols_size = symbols_size_(i);
                            if (term_symbols_size == validated_symbols_size) {
                                term_index = i;
                                const auto first_symbol_index = terms_[i];
                                for (auto j = 0; j < validated_symbols_size; j++) {
                                    if (symbols_[first_symbol_index + j] != validated_symbols[j]) {
                                        term_index = terms_.size();
                                        break;
                                    };
                                };
                                if (term_index < terms_.size()) {
                                    break;
                                };
                            };
                        };
                    };
                    
                    if (term_index < terms_.size()) {
                        // term == terms_[term_index], i.e. <term> + <term>, remove the term_[term_index]
                        last_equation_remove_term_(term_index);
                    } else {
                        // append the new term
                        assert(symbols_.size() < UINT32_MAX);
                        terms_.push_back((uint32_t)symbols_.size());
                        for (auto i = 0; i < validated_symbols_size; i++) {
                            symbols_.push_back(validated_symbols[i]);
                        };
                    };
                };
            } else {
                // the term is the constant 1
                // find the first term of size 1 and attach to it
                auto term_index = equations_[equations_.size() - 1];
                const auto term_index_end = term_index + equation_terms_size;
                while (term_index < term_index_end && symbols_size_(term_index) > 1) {
                    term_index++;
                };
                _assert_level_1(term_index < term_index_end);
                literal_t__negate(symbols_[terms_[term_index]]);
            };
        };
    };

    void Anf::append_equation_term(const literalid_t x) {
        append_equation_term(&x, 1);
    };

    void Anf::append_equation_term(const literalid_t x, const literalid_t y) {
        const literalid_t symbols[2] = { x, y };
        append_equation_term(symbols, 2);
    };
    
    // see descriptiption in the header
    literalid_t Anf::complete_equation(const literalid_t r, const bool optimize_negation) {
        is_incomplete_last_equation_();
        
        const size_t equation_terms_size = terms_size_(equations_.size() - 1);
        const size_t const_term_index = equations_[equations_.size() - 1];
        
        literalid_t result = symbols_[terms_[const_term_index]];
        _assert_level_1(literal_t__is_constant(result));
        
        if (equation_terms_size == 1) {
            _assert_level_1(literal_t__is_unassigned(r));
            remove_last_equation_();
        } else if (optimize_negation && equation_terms_size == 2 && symbols_size_(const_term_index + 1) == 1) {
            // one term one one variable
            // negated because 1 means + 1 which means "1 negated"
            _assert_level_1(literal_t__is_unassigned(r));
            result = literal_t__substitute_literal(literal_t__negated(result), symbols_[terms_[const_term_index + 1]]);
            remove_last_equation_();
        } else if (literal_t__is_constant(r) && equation_terms_size == 2 && symbols_size_(const_term_index + 1) == 1) {
            // one term one one variable
            // unit clause
            _assert_level_1(!optimize_negation);
            _assert_level_1(literal_t__is_variable(symbols_[terms_[const_term_index + 1]]));
            result = symbols_[terms_[const_term_index + 1]];
            result = literal_t__substitute_literal(literal_t__negated(r), result);
            result = literal_t__substitute_literal(literal_t__negated(symbols_[terms_[const_term_index]]), result);
            last_equation_remove_term_(const_term_index + 1);
            symbols_[terms_[const_term_index]] = result;
            // variable value, negation because the equation (result == 0)
            result = literal_t__is_negation(result) ? 0b1 : 0b0;
        } else {
            // multiple terms
            literalid_t last_literal;
            if (literal_t__is_variable(r) || literal_t__is_constant(r)) {
                _assert_level_1(!optimize_negation);
                last_literal = r;
                if (literal_t__is_constant(last_literal)) {
                    // try to find a single variable term and merge with it
                    for (auto i = const_term_index + 1; i < const_term_index + equation_terms_size - 1; i++) {
                        if (symbols_size_(i) == 1) {
                            _assert_level_1(literal_t__is_variable(symbols_[terms_[i]]));
                            last_literal = literal_t__substitute_literal(literal_t__negated(r), symbols_[terms_[i]]);
                            last_equation_remove_term_(i);
                            break;
                        };
                    };
                };
            } else {
                _assert_level_1(literal_t__is_unassigned(r));
                last_literal = VariableGenerator::new_variable_literal();
            };
            
            // 1 + x = ~x, therefore apply negated sign to the last_literal
            result = literal_t__substitute_literal(literal_t__negated(result), last_literal);
            
            if (optimize_negation) {
                // negation is within the result, the equation should include the same unnegated
                symbols_[terms_[const_term_index]] = last_literal;
            } else {
                symbols_[terms_[const_term_index]] = result;
                result = last_literal;
            };
        };
        
        return result;
    };

    size_t Anf::terms_size_(const size_t equation_index) const {
        return (equation_index == equations_.size() - 1 ? terms_.size() : equations_[equation_index + 1]) - equations_[equation_index];
    };

    size_t Anf::symbols_size_(const size_t term_index) const {
        return (term_index == terms_.size() - 1 ? symbols_.size() : terms_[term_index + 1]) - terms_[term_index];
    };

    void Anf::print_equation(std::ostream& stream, const size_t equation_index) const {
        const size_t terms_size = terms_size_(equation_index);
        
        for (auto j = equations_[equation_index]; j < equations_[equation_index] + terms_size; j++) {
            if (j > equations_[equation_index]) {
                stream << " + ";
            };
            
            const size_t symbols_size = symbols_size_(j);
            for (auto k = terms_[j]; k < terms_[j] + symbols_size; k++) {
                if (k > terms_[j]) {
                    stream << " * ";
                };
                
                assert(literal_t__is_variable(symbols_[k]));
                // only the only variable in the first term can be negated
                assert(k == terms_[j] || !literal_t__is_negation(symbols_[k]));
                stream << "x" << std::dec << literal_t__variable_id(symbols_[k]) + 1;
            };
        };
        
        // check the first constant term and output it last
        assert(terms_size >= 1);
        assert(symbols_size_(equations_[equation_index]) == 1);
        if (literal_t__is_negation(symbols_[terms_[equations_[equation_index]]])) {
            stream << " + 1";
        };
        
        stream << std::endl;
    };

    void Anf::evaluate(VariablesArray& variables) const {
        assert(variables_size() == variables.size());
        
        // pass 1 - gather values from unit clauses
        for (auto i = 0; i < equations_.size(); i++) {
            if (terms_size_(i) == 1 && symbols_size_(equations_[i]) == 1) {
                const literalid_t value = symbols_[terms_[equations_[i]]];
                _assert_level_1(literal_t__is_variable(value));
                variables.data()[literal_t__variable_id(value)] = literal_t__is_negation(value) ? 0b1 : 0b0;
            };
        };
        
        // pass 2 - try to evaluate all equations
        for (auto i = 0; i < equations_.size(); i++) {
            if (!(terms_size_(i) == 1 && symbols_size_(equations_[i]) == 1)) {
                const size_t terms_size = terms_size_(i);
                assert(terms_size >= 2);
                
                // assume the first term is the variable to assign
                // and the constant combined
                assert(symbols_size_(equations_[i]) == 1);
                assert(literal_t__is_variable(symbols_[terms_[equations_[i]]]));
                
                bool value = literal_t__is_negation(symbols_[terms_[equations_[i]]]);
                variableid_t variable_id = literal_t__variable_id(symbols_[terms_[equations_[i]]]);
                assert(variable_id < variables.size());
                
                // calculate variable value from the rest of equation
                for (auto j = equations_[i] + 1; j < equations_[i] + terms_size; j++) {
                    const size_t symbols_size = symbols_size_(j);
                    bool term_value = true;
                    for (auto k = terms_[j]; (k < terms_[j] + symbols_size) && term_value; k++) {
                        // require that all symbols are constants at this point
                        assert(literal_t__is_variable(symbols_[k]));
                        assert(!literal_t__is_negation(symbols_[k]));
                        assert(literal_t__variable_id(symbols_[k]) < variables.size());
                        const literalid_t symbol_value = variables.data()[literal_t__variable_id(symbols_[k])];
                        assert(literal_t__is_constant(symbol_value));
                        term_value &= literal_t__is_constant_1(symbol_value);
                    };
                    value ^= term_value;
                };
                
                variables.data()[variable_id] = literal_t__constant(value);
            };
        };
    };

    void Anf::encode_negations_(VariablesArray& template_) {
        literalid_t* variables = template_.data();
        for (auto i = 0; i < template_.size(); i++) {
            if (literal_t__is_negation(variables[i])) {
                append_equation();
                append_equation_term(variables[i]);
                variables[i] = complete_equation(LITERALID_UNASSIGNED, false);
            };
        };
    };

    VariablesArray Anf::evaluate(const VariablesArray& value_template, const VariablesArray& value,
                                 const VariablesArray& result_template) const {
        assert(variables_size() > 0);
        VariablesArray variables(variables_size(), 1);
        variables.assign_sequence();
        variables.assign_template_from(value_template, value);
        
        evaluate(variables);
        
        VariablesArray result(result_template);
        variables.assign_template_into(result_template, result);
        return result;
    };
}
