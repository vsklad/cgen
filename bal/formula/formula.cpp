//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include <iostream>
#include "variablesio.hpp"
#include "formula.hpp"

namespace bal {

    void Formula::initialize() {
        VariableGenerator::reset(0);
        named_variables_.clear();
        parameters_.clear();
        
        add_max_args_ = 0; // 0 means default value
        xor_max_args_ = 0; // 0 means default value
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
    
    void Formula::add_named_variable(const char* const name, const VariablesArray& value, const variables_size_t index) {
        auto it = named_variables_.find(name);
        if (it == named_variables_.end()) {
            VariablesArray variable_value(0, value.size());
            variable_value.expand_append_element(index, value);
            named_variables_.insert({name, variable_value});
        } else {
            it->second.expand_append_element(index, value);
        };
    };
    
    bool Formula::is_variable_named(const variableid_t variable_id) const {
        for (auto vit = named_variables_.begin(); vit != named_variables_.end(); vit++) {
            if (vit->second.contains(variable_id)) {
                return true;
            };
        };
        return false;
    };
    
    void Formula::named_variables_update(const VariablesArray& source) {
        for (auto vit = named_variables_.begin(); vit != named_variables_.end(); vit++) {
            source.assign_template_into(vit->second, vit->second);
        };
    };
    
    variables_size_t Formula::named_variable_update_unassigned(const std::string& name, const VariablesArray& source) {
        auto it = named_variables_.find(name);
        if (it != named_variables_.end()) {
            _assert_level_1(source.size() == it->second.size());
            const bal::literalid_t* const p_src = source.data();
            bal::literalid_t* const p_dst = it->second.data();
            variables_size_t changes_count = 0;
            
            for (auto i = 0; i < source.size(); i++) {
                if (literal_t__is_unassigned(p_dst[i]) && !literal_t__is_unassigned(p_src[i])) {
                    p_dst[i] = p_src[i];
                    changes_count++;
                };
            };
            
            return changes_count;
        } else {
            return VARIABLEID_ERROR;
        };
    };
    
    // does not check for conflicts
    // that is, if the same reference appear both straight and negated
    // the last occurence wins
    void Formula::named_variables_assign_negations(VariablesArray& destination) const {
        literalid_t* data = destination.data();
        // this assigns any negations into variables_
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

    void Formula::set_parameters(const formula_parameters_t& value) {
        parameters_ = value;
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
