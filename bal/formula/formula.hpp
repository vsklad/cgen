//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef formula_hpp
#define formula_hpp

#include <stdexcept>
#include <string>
#include <map>
#include "variables.hpp"
#include "variablesarray.hpp"

namespace bal {
    
    // Implements features common for formula representations
    
    typedef std::map<std::string, VariablesArray> formula_named_variables_t;
    typedef std::map<std::string, std::string> formula_parameters_t;
    
    enum FormulaProcessingMode {fpmUnoptimized, fpmAll, fpmOriginal};
    
    class Formula: protected VariableGenerator {
    private:
        // parameters are sets of values
        // it is expected that values are stored as comma separated list
        // with items in the format <name>: <value>
        // parameter name is a category name
        // it is expected that the parameters are write-only
        formula_parameters_t parameters_;
    
        // named variables can be defined with string labels
        // it is expected a variable in most cases an array, i.e. a word of bits
        // or perhaps even and array of words (concatenated arrays of bits)
        // named variables are needed for assignment/evaluation
        // certain or all bits may be constant, i.e. already encoded/optimized within the formula
        formula_named_variables_t named_variables_;
        
        // encoding parameters
        uint32_t add_max_args_;
        uint32_t xor_max_args_;
        
    protected:
        // ADD_MAX_ARGS determines maximal number of arguments for an add expression
        // longer expressions are split into batches of the given lenth, last one can be shorter
        // ADD_MAX_ARGS is applied at bit level; it determines max number of variables
        // including any carry in bits, while any constants are optimized out
        // 6 arguments max supported, see cnfwordadd.hpp
        virtual uint32_t ADD_MAX_ARGS_DEFAULT() const { return 3; };
        virtual uint32_t ADD_MAX_ARGS_MIN() const { return 2; };
        virtual uint32_t ADD_MAX_ARGS_MAX() const { return 6; };
        
        // XOR_MAX_ARGS determines maximal number of arguments for a xor expression
        // longer expressions are split into batches of the given lenth max, last one can be shorter
        // default value of 3 is chosen to minimize both variables and clauses
        // 10 means 2^10 = 1K clauses per expression - seems enough
        virtual uint32_t XOR_MAX_ARGS_DEFAULT() const { return 3; };
        virtual uint32_t XOR_MAX_ARGS_MIN() const { return 2; };
        virtual uint32_t XOR_MAX_ARGS_MAX() const { return 10; };
        
        inline formula_named_variables_t& get_named_variables_() { return named_variables_; };
    
    public:
        Formula() { initialize(); };
        
        virtual void initialize();
        virtual bool is_empty() const = 0;
        virtual variables_size_t variables_size() const { return VariableGenerator::last_variable() + 1; };
        
        // checks for variable assignment withitn the formula without pre-processing
        // returns UNASSIGNED if the variable is not assigned
        virtual literalid_t get_variable_value(const variableid_t variableid) const = 0;
        
    public:
        // Variables
        using VariableGenerator::new_variable_literal;
        using VariableGenerator::generate_unassigned_variable_literals;
        
        // Named Variables
        
        const formula_named_variables_t& get_named_variables() const;
        
        // this does not check if the assignment of the variable changes other variables
        // intended use is during encoding; afterwards see assign_named_variable
        void add_named_variable(const char* const name, const VariablesArray& value);
        // this does not check if the assignment of the variable changes other variables
        // intended use is during encoding; afterwards see assign_named_variable
        void add_named_variable(const char* const name, const VariablesArray& value, const variableid_t index);
        // determine if the binary variable is referenced from a named variable
        bool is_variable_named(const variableid_t variable_id) const;
        // given an array of all variable values
        // update all variable values
        void named_variables_update(const VariablesArray& source);
        // given named variable value, using it
        // assign all literals within definition which are unassigned
        // returns number of updated variables or an error if the variable not found
        variables_size_t named_variable_update_unassigned(const std::string& name, const VariablesArray& source);
        void named_variables_assign_negations(VariablesArray& destination) const;
        
        // Parameters
        
        const formula_parameters_t& get_parameters() const;
        void set_parameters(const formula_parameters_t& value);
        void add_parameter(const std::string& key, const std::string& name,
                           const std::string& value, const bool b_quote = true) ;
        void add_parameter(const std::string& key, const std::string& name, const uint32_t value);
        void clear_parameters(const std::string& key);
        
        // Encoding Parameters
        
        inline uint32_t get_add_max_args() const {
            return add_max_args_ == 0 ? ADD_MAX_ARGS_DEFAULT() : add_max_args_;
        };
        
        inline void set_add_max_args(const uint32_t value) {
            if (value < ADD_MAX_ARGS_MIN() || value > ADD_MAX_ARGS_MAX()) {
                throw std::out_of_range(std::string("add_max_args ").append(std::to_string(value)).append(" should be between ").append(std::to_string(ADD_MAX_ARGS_MIN())).append(" and ").append(std::to_string(ADD_MAX_ARGS_MAX())));
            };
            add_max_args_ = value;
        };
        
        inline uint32_t get_xor_max_args() const {
            return xor_max_args_ == 0 ? XOR_MAX_ARGS_DEFAULT() : xor_max_args_;
        };
        
        inline void set_xor_max_args(const uint32_t value) {
            if (value < XOR_MAX_ARGS_MIN() || value > XOR_MAX_ARGS_MAX()) {
                throw std::out_of_range(std::string("xor_max_args ").append(std::to_string(value)).append(" should be between ").append(std::to_string(XOR_MAX_ARGS_MIN())).append(" and ").append(std::to_string(XOR_MAX_ARGS_MAX())));
            };
            xor_max_args_ = value;
        };
    };
    
    // evaluate named variable given parameter
    // both parameters and result must correspond to existing named variables
    template<class FORMULA, typename std::enable_if<std::is_base_of<Formula, FORMULA>::value, int>::type = 0>
    inline bool evaluate(FORMULA& formula,
                         const char* parameter_name, const VariablesArray& parameter_value,
                         const char* result_name, VariablesArray& result_value) {
        VariablesArray variables(formula.variables_size());
        variables.assign_sequence();
        if (variables.assign_template_from(formula.get_named_variables().at(parameter_name), parameter_value) == VARIABLEID_ERROR) {
            throw std::invalid_argument("Conflicting binary variable assignment");
        };
        if (evaluate(formula, variables)) {
            // compose result using the variable template and the result of the evaluation
            variables.assign_template_into(formula.get_named_variables().at(result_name), result_value);
            return true;
        } else {
            return false;
        };
    };
    
    // assign named parameter to the supplied value and then process the formula
    template<class FORMULA, typename std::enable_if<std::is_base_of<Formula, FORMULA>::value, int>::type = 0>
    inline bool process(FORMULA& formula,
                        const char* parameter_name, const VariablesArray& parameter_value,
                        const bool b_reindex_variables, const FormulaProcessingMode mode) {
        VariablesArray variables(formula.variables_size());
        variables.assign_sequence();
        if (variables.assign_template_from(formula.get_named_variables().at(parameter_name), parameter_value) == VARIABLEID_ERROR) {
            throw std::invalid_argument("Conflicting binary variable assignment");
        };
        return process(formula, variables, b_reindex_variables, mode);
    };
    
}

#endif /* formula_hpp */
