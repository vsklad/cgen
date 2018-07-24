//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef formula_hpp
#define formula_hpp

#include <string>
#include <map>
#include "variablesarray.hpp"

namespace ple {
    
    // Implements features common for formula representations
    
    typedef std::map<const std::string, VariablesArray> formula_named_variables_t;
    typedef std::map<const std::string, std::string> formula_parameters_t;
    
    class Formula {
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
        
    protected:
        void initialize();
        inline formula_named_variables_t& get_named_variables_() { return named_variables_; };
    
    public:
        virtual const bool is_empty() const = 0;
        virtual const variables_size_t variables_size() const = 0;
        
    public:
        // Named Variables
        
        const formula_named_variables_t& get_named_variables() const;
        
        // this does not check if the assignment of the variable changes other variables
        // intended use is during encoding; afterwards see assign_named_variable
        void add_named_variable(const char* const name, const VariablesArray& value);
        // this does not check if the assignment of the variable changes other variables
        // intended use is during encoding; afterwards see assign_named_variable
        void add_named_variable(const char* const name, const VariablesArray& value, const variableid_t index);
        // given an array of all variable values
        // update all variable values
        void named_variables_update(const VariablesArray& source);
        
        void named_variables_assign_negations(VariablesArray& destination) const;
        
        // Parameters
        
        const formula_parameters_t& get_parameters() const;
        void add_parameter(const std::string& key, const std::string& name,
                           const std::string& value, const bool b_quote = true) ;
        void add_parameter(const std::string& key, const std::string& name, const uint32_t value);
        void clear_parameters(const std::string& key);
    };
    
}

#endif /* formula_hpp */
