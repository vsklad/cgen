//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfgraphml_hpp
#define cnfgraphml_hpp

#include <set>
#include <map>
#include "cnfvig.hpp"

namespace bal {
    
    template<bool WEIGHTED>
    void CnfGraphMLStreamWriter_write_edge_weighted(std::ostream& stream, const edge_weight_data_t<WEIGHTED>& edge) {};
    template<>
    void CnfGraphMLStreamWriter_write_edge_weighted<true>(std::ostream& stream, const edge_weight_data_t<true>& edge) {
        stream << "<data key=\"e_cardinality\">" << edge.cardinality << "</data>" << std::endl;
        stream << "<data key=\"e_weight\">" << edge.weight << "</data>" << std::endl;
    };
    
    template<bool WEIGHTED>
    class CnfGraphMLStreamWriter: public CnfVigStreamWriter<WEIGHTED, false> {
    protected:
        using base = CnfVigStreamWriter<WEIGHTED, false>;
        using edge_data_t = typename base::edge_data_t;
        using base::node_label;
        std::ostream& stream = base::stream;
        
    protected:
        void write_header(const Cnf& value) override {
            stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
            stream << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">" << std::endl;
            stream << "<graph id=\"CNF\" edgedefault=\"undirected\">" << std::endl;
            stream << "<key id=\"n_variable_name\" for=\"node\" attr.name=\"variable_name\" attr.type=\"string\"/>" << std::endl;
            stream << "<key id=\"n_variable_index\" for=\"node\" attr.name=\"variable_index\" attr.type=\"int\"/>" << std::endl;
            stream << "<key id=\"n_variable_id\" for=\"node\" attr.name=\"variable_id\" attr.type=\"int\"/>" << std::endl;
            stream << "<key id=\"n_label\" for=\"node\" attr.name=\"label\" attr.type=\"string\"/>" << std::endl;
            
            if (WEIGHTED) {
                stream << "<key id=\"e_cardinality\" for=\"edge\" attr.name=\"cardinality\" attr.type=\"int\"/>" << std::endl;
                stream << "<key id=\"e_weight\" for=\"edge\" attr.name=\"weight\" attr.type=\"double\"/>" << std::endl;
            };
        };
        
        void write_footer(const Cnf& value) override {
            stream << "</graph>" << std::endl;
            stream << "</graphml>" << std::endl;
        };
        
        void write_node(const variableid_t id, const char* const name,
                        const unsigned index, const unsigned element_size, const timestamp_t end) override {
            stream << std::dec;
            stream << "<node id=\"v"  << literal_t(variable_t__literal_id(id)) << "\">" << std::endl;
            stream << "<data key=\"n_variable_id\">" << literal_t(variable_t__literal_id(id)) << "</data>" << std::endl;
            if (name != nullptr) {
                stream << "<data key=\"n_variable_name\">" << name << "</data>" << std::endl;
                stream << "<data key=\"n_variable_index\">" << index << "</data>" << std::endl;
                stream << "<data key=\"n_label\">" << node_label(name, index, element_size) << "</data>" << std::endl;
            };
            stream << "</node>" << std::endl;
        };
        
        void write_edge(const variableid_t source_id, const variableid_t target_id, const edge_data_t& edge) override {
            if (WEIGHTED) {
                stream << "<edge source=\"v" << literal_t(variable_t__literal_id(source_id)) << "\" target=\"v" << literal_t(variable_t__literal_id(target_id)) << "\">" << std::endl;
                CnfGraphMLStreamWriter_write_edge_weighted<WEIGHTED>(stream, edge);
                stream << "</edge>" << std::endl;
            } else {
                stream << "<edge source=\"v" << literal_t(variable_t__literal_id(source_id)) << "\" target=\"v" << literal_t(variable_t__literal_id(target_id)) << "\"/>" << std::endl;
            };
        };
        
    public:
        CnfGraphMLStreamWriter(std::ostream& stream): base(stream) {};
    };
    
};

#endif /* cnfgraphml_hpp */
