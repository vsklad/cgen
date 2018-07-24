//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include <assert.h>
#include "anf.hpp"
#include "variablesio.hpp"

namespace ple {

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
    
    void Anf::append_equation() {
        assert(terms_.size() < UINT32_MAX);
        equations_.push_back((uint32_t)terms_.size());
        // add the constant term
        terms_.push_back((uint32_t)symbols_.size());
        symbols_.push_back(literal_t__constant(0));
    };

    void Anf::append_equation_term(const literalid_t* const symbols, const uint32_t symbols_size) {
        is_incomplete_last_equation_();
        const size_t equation_terms_size = terms_size_(equations_.size() - 1);
        
        // while 1, the term is valid even if becomes 1
        literalid_t constant = literal_t__constant(1);
        
        // validate the term itself
        // build sorted array of unique variable numbers
        // the rest is optimized out
        literalid_t validated_symbols[symbols_size];
        uint32_t validated_symbols_size = 0;
        uint32_t first_negation_index = symbols_size;
        
        for (auto i = 0; i < symbols_size && literal_t__is_constant_1(constant); i++) {
            if (literal_t__is_constant_0(symbols[i])) {
                constant = literal_t__constant(0);
                validated_symbols_size = 0;
                break;
            } else if (literal_t__is_constant_1(symbols[i])) {
                continue;
            } else {
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
                literal_t__constant_eor_lhs(symbols_[terms_[equations_[equations_.size() - 1]]], constant);
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
    
    // if the queation is a constant,
    //   returns the constant and removes the equation
    // otherwise if it is a variable
    //   returns the variable together with the negation sign and removes the equation
    // otherwise
    //   introduces a new variable, adds it to the equation,
    //   then returns it together with the negation sign
    literalid_t Anf::complete_equation(const bool optimize_negation) {
        is_incomplete_last_equation_();
        
        const size_t equation_terms_size = terms_size_(equations_.size() - 1);
        literalid_t result = symbols_[terms_[equations_[equations_.size() - 1]]];
        
        if (equation_terms_size == 1) {
            remove_last_equation_();
        } else {
            if (optimize_negation && equation_terms_size == 2 &&
                symbols_size_(equations_[equations_.size() - 1] + 1) == 1) {
                // one term one one variable
                // negated because 1 means + 1 which means "1 negated"
                result = literal_t__substitute_literal(literal_t__negated(result),
                                                       symbols_[terms_[equations_[equations_.size() - 1] + 1]]);
                remove_last_equation_();
            } else {
                // multiple terms
                result = literal_t__substitute_variable(literal_t__negated(result),
                                                        variable_generator_.new_variable());
                if (optimize_negation) {
                    // negation is within the result, the equation should include the same unnegated
                    symbols_[terms_[equations_[equations_.size() - 1]]] = literal_t__unnegated(result);
                } else {
                    symbols_[terms_[equations_[equations_.size() - 1]]] = result;
                    literal_t__unnegate(result);
                }
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
        
        // skip the first constant term
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

    void Anf::evaluate_(VariablesArray& variables) const {
        for (auto i = 0; i < equations_.size(); i++) {
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

    void Anf::encode_negations_(VariablesArray& template_) {
        literalid_t* variables = template_.data();
        for (auto i = 0; i < template_.size(); i++) {
            if (literal_t__is_negation(variables[i])) {
                append_equation();
                append_equation_term(variables[i]);
                variables[i] = complete_equation(false);
            };
        };
    };

    VariablesArray Anf::evaluate(const VariablesArray& value_template, const VariablesArray& value,
                                     const VariablesArray& result_template) const {
        
        assert(variables_size() > 0);
        VariablesArray variables(variables_size(), 1);
        variables.assign_sequence();
        variables.assign_template_from(value_template, value);
        
        evaluate_(variables);
        
        VariablesArray result(result_template);
        variables.assign_template_into(result_template, result);
        return result;
    };
}
