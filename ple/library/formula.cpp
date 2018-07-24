//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include "formula.hpp"

namespace ple {

    void Formula::initialize() {
        named_variables_.clear();
        parameters_.clear();
    };
    
    // Named Variables
    
    const formula_named_variables_t& Formula::get_named_variables() const {
        return named_variables_;
    };
    
    void Formula::add_named_variable(const char* const name, const VariablesArray& value) {
        // a copy of value is made here; overwrites any previous value
        auto it = named_variables_.find(name);
        if (it == named_variables_.end()) {
            named_variables_.insert({name, value});
        } else {
            it->second = value;
        };
    };
    
    void Formula::add_named_variable(const char* const name, const VariablesArray& value, const variableid_t index) {
        auto it = named_variables_.find(name);
        if (it == named_variables_.end()) {
            named_variables_.insert({name, value});
        } else {
            it->second.expand_elements(index + 1);
            it->second.assign_element(value, index);
        };
    };
    
    void Formula::named_variables_update(const VariablesArray& source) {
        for (auto vit = named_variables_.begin(); vit != named_variables_.end(); vit++) {
            source.assign_template_into(vit->second, vit->second);
        };
    };
    
    // does not check for conflicts
    // that is, if the same reference appear both straight and negated
    // the last occurence wins
    void Formula::named_variables_assign_negations(VariablesArray& destination) const {
        literalid_t* data = destination.data();
        // this assign any negations into variables_
        for (auto vit = named_variables_.begin(); vit != named_variables_.end(); vit++) {
            const literalid_t* const template_ = vit->second.data();
            for (auto j = 0; j < vit->second.size(); j++) {
                if (literal_t__is_variable(template_[j])) {
                    const variableid_t variable_id = literal_t__variable_id(template_[j]);
                    assert(variable_id < destination.size());
                    data[variable_id] = template_[j];
                };
            };
        };
    };
    
    // Parameters
    
    const formula_parameters_t& Formula::get_parameters() const {
        return parameters_;
    };
    
    void Formula::add_parameter(const std::string& key, const std::string& name,
                       const std::string& value, const bool b_quote) {
        std::string item = (b_quote) ?
        std::string(name).append(": \"").append(value).append("\"") :
        std::string(name).append(": ").append(value);
        
        auto it = parameters_.find(key);
        if (it != parameters_.end()) {
            if (it->second.size() > 0) {
                it->second.append(", ");
            };
            it->second.append(item);
        } else {
            parameters_.insert({key, item});
        };
    };
    
    void Formula::add_parameter(const std::string& key, const std::string& name, const uint32_t value) {
        add_parameter(key, name, std::to_string(value), false);
    };
    
    void Formula::clear_parameters(const std::string& key) {
        auto it = parameters_.find(key);
        if (it != parameters_.end()) {
            parameters_.erase(it);
        };
    };
    
}
