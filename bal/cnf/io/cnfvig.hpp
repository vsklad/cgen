//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfvig_hpp
#define cnfvig_hpp

#include <map>
#include <set>
#include <stdint.h>
#include "assertlevels.hpp"
#include "streamable.hpp"
#include "cnf.hpp"

namespace bal {

    typedef uint32_t timestamp_t;
    static constexpr uint32_t TIMESTAMP_MIN = 0;
    static constexpr uint32_t TIMESTAMP_MAX = UINT32_MAX;
    
    typedef enum {eaInsert, eaUpdate, eaRemove} edge_action_t;
    
    struct edge_base_data_t {
        void update(const uint32_t clause_header, const edge_action_t action, const timestamp_t timestamp) {};
    };
    
    template<bool DYNAMIC>
    struct edge_dynamic_data_t: edge_base_data_t {};
    
    template<> struct edge_dynamic_data_t<true> {
        unsigned clauses_count;
        timestamp_t start;
        timestamp_t end;
        
        void update(const uint32_t clause_header, const edge_action_t action, const timestamp_t timestamp) {
            switch(action) {
                case eaInsert:
                    clauses_count = 1;
                    start = timestamp;
                    end = TIMESTAMP_MAX;
                    break;
                case eaUpdate:
                    clauses_count++;
                    break;
                case eaRemove:
                    _assert_level_0(timestamp >= start);
                    _assert_level_0(clauses_count > 0);
                    clauses_count--;
                    if (clauses_count == 0) {
                        end = timestamp;
                    };
                    break;
            };
        };
    };
    
    template<bool WEIGHTED>
    struct edge_weight_data_t: edge_base_data_t {};
    
    template<> struct edge_weight_data_t<true> {
        unsigned cardinality = 0;
        double weight = 0.0;
        
        void update(const uint32_t clause_header, const edge_action_t action, const timestamp_t timestamp) {
            const clause_size_t clause_size = _clause_header_size(clause_header);
            const unsigned clause_cardinality = clause_size < 4 ?
                get_cardinality_uint16(_clause_header_flags(clause_header)) : 1;
            const double clause_weight = 2.0 * clause_cardinality / clause_size / (clause_size - 1);
            
            switch(action) {
                case eaInsert:
                case eaUpdate:
                    cardinality += clause_cardinality;
                    weight += clause_weight;
                    break;
                case eaRemove:
                    _assert_level_0(cardinality >= clause_cardinality);
                    cardinality -= clause_cardinality;
                    weight -= clause_weight;
                    break;
            };
        };
    };
    
    // outputs an undirected graph
    // nodes correspond to variables; node IDs match DIMACS variable numbers
    // there is an edge between two variables if those variables occur in the same clause
    template<bool WEIGHTED, bool DYNAMIC>
    class CnfVigStreamWriter: public StreamWriter<Cnf> {
    protected:
        struct __edge_data_t: edge_weight_data_t<WEIGHTED>, edge_dynamic_data_t<DYNAMIC> {
            unsigned id;
            
            void update(const uint32_t clause_header, const edge_action_t action, const timestamp_t timestamp) {
                edge_weight_data_t<WEIGHTED>::update(clause_header, action, timestamp);
                edge_dynamic_data_t<DYNAMIC>::update(clause_header, action, timestamp);
            };
        };
        
        typedef struct __edge_data_t edge_data_t;
        typedef std::map<uint64_t, edge_data_t> edges_data_t;
        
    protected:
        virtual void write_header(const Cnf& value) = 0;
        virtual void write_footer(const Cnf& value) = 0;
        virtual void write_node(const variableid_t id, const char* const name,
                                const unsigned index, const unsigned element_size, const timestamp_t end) = 0;
        virtual void write_edge(const variableid_t source_id, const variableid_t target_id, const edge_data_t& edge) = 0;
        
    protected:
        // note: elements have big-endian representation,
        //   i.e. the index within element is reversed
        virtual const std::string node_label(const char* const name, const unsigned index, const unsigned element_size) const {
            std::string label;
            if (name != nullptr) {
                label = name;
                if (element_size == 1) {
                    label += "[" + std::to_string(index) + "]";
                } else {
                    label += "[" + std::to_string(index / element_size) + "]";
                    label += "[" + std::to_string(element_size - index % element_size - 1) + "]";
                };
            };
            return label;
        };
        
        // first, write all variables that are part of named ones
        // second, write all other variables
        // link binary variable to the first named variable it occurs in
        // and ignore other ones if any
        virtual void write_nodes(const Cnf& value, const edges_data_t& edges) {
            bool is_processed[value.variables_size()];
            std::fill(is_processed, is_processed + value.variables_size(), false);
            
            const formula_named_variables_t& nv = value.get_named_variables();
            for (formula_named_variables_t::const_iterator it = nv.begin(); it != nv.end(); ++it) {
                const VariablesArray& nv_variables = it->second;
                const std::string& nv_name = it->first;
                for (auto i = 0; i < nv_variables.size(); i++) {
                    if (literal_t__is_variable(nv_variables.data()[i])) {
                        const variableid_t variable_id = literal_t__variable_id(nv_variables.data()[i]);
                        if (!is_processed[variable_id]) {
                            write_node(variable_id, nv_name.c_str(), i, nv_variables.element_size(), TIMESTAMP_MAX);
                            is_processed[variable_id] = true;
                        };
                    };
                };
            };
            
            for (auto i = 0; i < value.variables_size(); i++) {
                if (!is_processed[i]) {
                    write_node(i, nullptr, 0, 1, TIMESTAMP_MAX);
                    is_processed[i] = true;
                };
            };
        };
        
        template<bool REMOVE>
        void compute_edge(const uint32_t* const p_clause, const timestamp_t timestamp,
                          edges_data_t& edges, unsigned& next_edge_id) {
            // iterate literal pairs
            // it is guaranteed that the sequence is sorted and no duplicates exist
            // weight is calculated such that the sum of weights of edges generated from a clause is 1
            for (auto i = 0; i < _clause_size(p_clause); i++) {
                if (_clause_size(p_clause) > 1) {
                    // determine weight and cardinality contribution to each edge
                    for (auto j = i + 1; j < _clause_size(p_clause); j++) {
                        const variableid_t source = literal_t__variable_id(_clause_literal(p_clause, i));
                        const variableid_t target = literal_t__variable_id(_clause_literal(p_clause, j));
                        const uint64_t key = ((uint64_t)target << 32) | source;
                        
                        // check if the adge exists already; ignore if so, add otherwise
                        auto it = edges.find(key);
                        if (it == edges.end()) {
                            _assert_level_1(!REMOVE);
                            edge_data_t edge_data;
                            edge_data.id = next_edge_id++;
                            edge_data.update(*p_clause, eaInsert, timestamp);
                            edges.insert({key, edge_data});
                        } else {
                            it->second.update(*p_clause, (REMOVE ? eaRemove : eaUpdate), timestamp);
                        };
                    };
                };
            };
        };
        
        virtual void compute_edges(const Cnf& value, const timestamp_t timestamp,
                                   edges_data_t& edges, unsigned& next_edge_id) {
            stream << std::dec;
            for (auto it: value.clauses()) {
                compute_edge<false>(_clauses_offset_item_clause(it), timestamp, edges, next_edge_id);
            };
        };
        
        virtual void write_edges(const Cnf& value, const edges_data_t& edges) {
            for (auto edge: edges) {
                write_edge(edge.first & 0xFFFFFFFF, edge.first >> 32, edge.second);
            };
        };
        
    public:
        CnfVigStreamWriter(std::ostream& stream): StreamWriter<Cnf>(stream) {};
        
        void write(const Cnf& value) override {
            edges_data_t edges;
            unsigned next_edge_id = 0;
            compute_edges(value, TIMESTAMP_MIN, edges, next_edge_id);
            
            write_header(value);
            write_nodes(value, edges);
            write_edges(value, edges);
            write_footer(value);
        };
    };
};

#endif /* cnfvig_hpp */
