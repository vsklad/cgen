//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef anf_hpp
#define anf_hpp

#include <string>
#include <vector>
#include "variables.hpp"
#include "variablesarray.hpp"
#include "formula.hpp"

namespace bal {

    // storage for ANF equations as a set of strings
    // for simplicity, the burden of encoding the expression is on the encoder bit class

    class Anf: public Formula {
    private:
        // equations_ is an index of its first term
        // number of terms - equations_[i + 1] - equations_[i], same with terms_ and symbols_
        // first term of each equation is the constant, either 0 or 1; always has one constant symbol
        // second term of each equation defines a variable, always has one variable symbol
        std::vector<literalid_t> symbols_;
        std::vector<uint32_t> terms_;
        std::vector<uint32_t> equations_;

    private:
        inline void is_incomplete_last_equation_() const;
        inline void remove_last_equation_();
        inline void last_equation_remove_term_(const size_t term_index);
        
        inline size_t terms_size_(const size_t equation_index) const;
        inline size_t symbols_size_(const size_t term_index) const;
        
        // for the given template, for all negated variables
        // add equations that to produce uncomplemented versions and update the template accordingly
        void encode_negations_(VariablesArray& template_);
        
    protected:
        // add - full adder is supported only
        uint32_t ADD_MAX_ARGS_DEFAULT() const override { return 3; };
        uint32_t ADD_MAX_ARGS_MIN() const override { return 3; };
        uint32_t ADD_MAX_ARGS_MAX() const override { return 3; };
        // xor - virtually unlimited
        uint32_t XOR_MAX_ARGS_DEFAULT() const override { return UINT32_MAX; };
        uint32_t XOR_MAX_ARGS_MIN() const override { return 2; };
        uint32_t XOR_MAX_ARGS_MAX() const override { return UINT32_MAX; };
        
    public:
        void initialize() override;
        
        bool is_empty() const override { return equations_.size() == 0; };
        size_t equations_size() const { return equations_.size(); };
        void print_equation(std::ostream& stream, const std::size_t equation_index) const;
        
        void append_equation();
        void append_equation_term(const literalid_t* const symbols, const std::size_t symbols_size);
        void append_equation_term(const literalid_t x);
        void append_equation_term(const literalid_t x, const literalid_t y);

        // completes the current/last unfinished equation
        //   r is the right hand side
        //     if r is unassigned it means a new variable shoud be introduced
        // if the last equation is a constant,
        //   returns the constant and removes the equation
        //   requires that r is unassigned
        // otherwise if it is a variable
        //   if r is unassigned
        //     returns the variable together with the negation sign and removes the equation
        //   if r is a constant
        //     merges the variable with both the constant term and r, making a unit clause
        //     returns the constant value of the unit clause
        // otherwise
        //   if r is unassigned, introduces a new variable,
        //     adds it to the equation by merging with the first constant term
        //     then returns it together with the negation sign
        //   if r is a variable
        //     inserts it into the equation together with the negation sign
        //     by merging with the first constant term
        //     then returns it as well
        //   if r is a constant
        //     applies the constant to the first constant term
        //     then finds the first term of size 1 and merges it with the constant term
        literalid_t complete_equation(const literalid_t r, const bool optimize_negation);
        
        literalid_t get_variable_value(const variableid_t variableid) const override {
            return LITERALID_UNASSIGNED;
        };
        
        void evaluate(VariablesArray& variables) const;
        VariablesArray evaluate(const VariablesArray& value_template, const VariablesArray& value,
                                const VariablesArray& result_template) const;
        
        friend inline bool evaluate(const Anf& anf, VariablesArray& variables) {
            anf.evaluate(variables);
            return true;
        };
         
        // works simlistically by appending equations
        // limited parameters supported only
        friend inline bool process(Anf& anf, const VariablesArray& variables,
                                   const bool b_reindex_variables, const FormulaProcessingMode mode) {
            _assert_level_0(mode == fpmUnoptimized); // other modes not supported
            _assert_level_1(anf.variables_size() == variables.size());
            
            const literalid_t* const values = variables.data();
            
            for (variableid_t i = 0; i < variables.size(); i++) {
                if (variable_t__literal_id(i) != values[i]) {
                    anf.append_equation();
                    anf.append_equation_term(values[i]);
                    literalid_t first_id = anf.symbols_[anf.terms_[anf.equations_[anf.equations_.size() - 1]]];
                    first_id = literal_t__substitute_literal(literal_t__negated(first_id), variable_t__literal_id(i));
                    anf.symbols_[anf.terms_[anf.equations_[anf.equations_.size() - 1]]] = first_id;
                };
            };
            
            return true;
        };
        
        // negates any negated references within named variables
        // works simplistically by appending equations as necessary
        friend inline bool normalize_variables(Anf& anf, const bool b_reindex_variables) {
            assert(b_reindex_variables);
            for (auto vit = anf.get_named_variables_().begin(); vit != anf.get_named_variables_().end(); vit++) {
                anf.encode_negations_(vit->second);
            };
            return true;
        };
    };
};

#endif /* anf_hpp */
