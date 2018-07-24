//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef anf_hpp
#define anf_hpp

#include <string>
#include <vector>
#include "variables.hpp"
#include "variablesarray.hpp"
#include "formula.hpp"

namespace ple {

    // storage for ANF equations as a set of strings
    // for simplicity, the burden of encoding the expression is on the encoder bit class

    class Anf: public Formula {
    private:
        VariableGenerator variable_generator_;
        
        // equations_ is an index of its first term
        // number of terms - equations_[i + 1] - equations_[i], same with terms_ and symbols_
        // first term of each equation is the constant, either 0 or 1; always has one constant symbol
        // second term of each equation defines a variable, always has one variable symbol
        //
        std::vector<literalid_t> symbols_;
        std::vector<uint32_t> terms_;
        std::vector<uint32_t> equations_;

    private:
        inline void is_incomplete_last_equation_() const;
        inline void remove_last_equation_();
        
        inline size_t terms_size_(const size_t equation_index) const;
        inline size_t symbols_size_(const size_t term_index) const;
        inline void evaluate_(VariablesArray& variables) const;
        
        // for the given template, for all negated variables
        // add equations that to produce uncomplemented versions and update the template accordingly
        void encode_negations_(VariablesArray& template_);
        
    public:
        const bool is_empty() const override { return equations_.size() == 0; };
        size_t equations_size() const { return equations_.size(); };
        void print_equation(std::ostream& stream, const size_t equation_index) const;
        
        VariableGenerator& variable_generator() { return variable_generator_; };
        const variableid_t variables_size() const override { return variable_generator_.last_variable() + 1; };
        
        void append_equation();
        void append_equation_term(const literalid_t* const symbols, const uint32_t symbols_size);
        void append_equation_term(const literalid_t x);
        void append_equation_term(const literalid_t x, const literalid_t y);

        // if the last equation is a constant,
        //   returns the constant and removes the equation
        // otherwise if it is a variable
        //   returns the variable together with the negation sign and removes the equation
        // otherwise
        //   introduces a new variable, adds it to the equation,
        //   then returns it together with the negation sign
        literalid_t complete_equation(const bool optimize_negation = true);
        
        VariablesArray evaluate(const VariablesArray& value_template, const VariablesArray& value,
                                const VariablesArray& result_template) const;
        
        friend inline const bool evaluate(Anf& anf, const char* parameters_name, const VariablesArray& parameters,
                             const char* result_name, VariablesArray &result_value) {
            result_value = anf.evaluate(anf.get_named_variables().at(parameters_name), parameters,
                                  anf.get_named_variables().at(result_name));
            return true;
        };
        
        // works simlistically by appending necessary number of equations
        friend inline const bool assign(Anf& anf, const char* name, const VariablesArray& value) {
            VariablesArray template_ = anf.get_named_variables().at(name);
            assert(template_.size() == value.size());
            
            const literalid_t* const variables = template_.data();
            const literalid_t* const values = value.data();
            
            for (auto i = 0; i < template_.size(); i++) {
                if (variables[i] != values[i]) {
                    anf.append_equation();
                    anf.append_equation_term(values[i]);
                    literalid_t first_id = anf.symbols_[anf.terms_[anf.equations_[anf.equations_.size() - 1]]];
                    first_id = literal_t__substitute_literal(literal_t__negated(first_id), variables[i]);
                    anf.symbols_[anf.terms_[anf.equations_[anf.equations_.size() - 1]]] = first_id;
                };
            };
            
            anf.add_named_variable(name, value);
            return true;
        };
        
        // negates any negated references within named variables
        // works simplistically by appending equations as necessary
        friend inline const bool normalize_variables(Anf& anf) {
            for (auto vit = anf.get_named_variables_().begin(); vit != anf.get_named_variables_().end(); vit++) {
                anf.encode_negations_(vit->second);
            };
            return true;
        };
    };
}

#endif /* anf_hpp */
