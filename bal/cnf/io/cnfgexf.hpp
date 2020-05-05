//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfgexf_hpp
#define cnfgexf_hpp

#include <set>
#include "cnfvig.hpp"

namespace bal {
    
    template<bool WEIGHTED>
    void CnfGexfStreamWriter_write_edge_weighted(std::ostream& stream, const edge_weight_data_t<WEIGHTED>& edge) {
        stream << "/>" << std::endl;
    };
    template<>
    void CnfGexfStreamWriter_write_edge_weighted<true>(std::ostream& stream, const edge_weight_data_t<true>& edge) {
        stream << " weight=\"" << edge.weight << "\"";
        if (edge.cardinality != 1) {
            stream << ">" << std::endl;
            stream << "  <attvalues>" << std::endl;
            stream << "    <attvalue for=\"0\" value=\"" << edge.cardinality << "\"/>" << std::endl;
            stream << "  </attvalues>" << std::endl;
            stream << "</edge>" << std::endl;
        } else {
            stream << "/>" << std::endl;
        };
    };
    
    template<bool DYNAMIC>
    void CnfGexfStreamWriter_write_edge_dynamic(std::ostream& stream, const edge_dynamic_data_t<DYNAMIC>& edge) {};
    template<>
    void CnfGexfStreamWriter_write_edge_dynamic<true>(std::ostream& stream, const edge_dynamic_data_t<true>& edge) {
        if (edge.start != TIMESTAMP_MIN) {
            stream << " start=\"" << edge.start << "\"";
        };
        if (edge.end != TIMESTAMP_MAX) {
            stream << " end=\"" << edge.end << "\"";
        };
    };
    
    template<bool WEIGHTED, bool DYNAMIC>
    class CnfGexfStreamWriter: public CnfVigStreamWriter<WEIGHTED, DYNAMIC> {
    protected:
        using base = CnfVigStreamWriter<WEIGHTED, DYNAMIC>;
        using edge_data_t = typename base::edge_data_t;
        using edges_data_t = typename base::edges_data_t;
        using base::node_label;
        std::ostream& stream = base::stream;
        
    private:
        std::string current_date_string() {
            std::time_t t = std::time(nullptr);
            char mbstr[100];
            if (std::strftime(mbstr, sizeof(mbstr), "%Y-%m-%d", std::localtime(&t))) {
                return std::string(mbstr);
            } else {
                return "";
            };
        };
        
    protected:
        void write_header(const Cnf& value) override {
            stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
            stream << "<gexf xmlns=\"http://www.gexf.net/1.2draft\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema−instance\" xsi:schemaLocation=\"http://www.gexf.net/1.2draft http://www.gexf.net/1.2draft/gexf.xsd\" version=\"1.2\">" << std::endl;
            stream << "<meta lastmodifieddate=\"" << current_date_string() << "\">" << std::endl;
            stream << "  <creator>https://cgen.sophisticatedways.net</creator>" << std::endl;
            stream << "  <description>CNF formula Variable Incidence Graph (VIG)</description>" << std::endl;
            stream << "  <keywords>CNF, VIG</keywords>" << std::endl;
            stream << "</meta>" << std::endl;
            stream << "<graph defaultedgetype=\"undirected\" idtype=\"integer\"";
            if (DYNAMIC) {
                stream << " mode=\"dynamic\" timeformat=\"integer\"";
            } else {
                stream << " mode=\"static\"";
            };
            stream << ">" << std::endl;
            stream << "  <attributes class=\"node\">" << std::endl;
            stream << "    <attribute id=\"0\" title=\"variable_name\" type=\"string\"/>" << std::endl;
            stream << "    <attribute id=\"1\" title=\"variable_index\" type=\"int\"/>" << std::endl;
            stream << "  </attributes>" << std::endl;
            if (WEIGHTED) {
                stream << "  <attributes class=\"edge\">" << std::endl;
                stream << "    <attribute id=\"0\" title=\"cardinality\" type=\"int\">" << std::endl;
                stream << "      <default>1</default>" << std::endl;
                stream << "    </attribute>" << std::endl;
                stream << "  </attributes>" << std::endl;
            };
        };
        
        void write_footer(const Cnf& value) override {
            stream << "</graph>" << std::endl;
            stream << "</gexf>" << std::endl;
        };
        
        void write_node(const variableid_t id, const char* const name,
                        const unsigned index, const unsigned element_size, const timestamp_t end) override {
            _assert_level_0(DYNAMIC || end == TIMESTAMP_MAX);
            stream << std::dec;
            stream << "<node id=\""  << literal_t(variable_t__literal_id(id)) << "\"";
            if (DYNAMIC && end != TIMESTAMP_MAX) {
                stream << " end=\"" << end << "\"";
            };
            if (name != nullptr) {
                stream << " label=\"" << node_label(name, index, element_size) << "\"";
                stream << ">" << std::endl;
                stream << "  <attvalues>" << std::endl;
                stream << "    <attvalue for=\"0\" value=\"" << name << "\"/>" << std::endl;
                stream << "    <attvalue for=\"1\" value=\"" << index << "\"/>" << std::endl;
                stream << "  </attvalues>" << std::endl;
                stream << "</node>" << std::endl;
            } else {
                stream << "/>" << std::endl;
            };
        };
        
        void write_edge(const variableid_t source_id, const variableid_t target_id, const edge_data_t& edge) override {
            stream << "<edge id=\"" << edge.id << "\"";
            stream << " source=\"" << literal_t(variable_t__literal_id(source_id)) << "\"";
            stream << " target=\"" << literal_t(variable_t__literal_id(target_id)) << "\"";
            CnfGexfStreamWriter_write_edge_dynamic(stream, edge);
            CnfGexfStreamWriter_write_edge_weighted(stream, edge);
        };
        
        void write_nodes(const Cnf& value, const edges_data_t& edges) override {
            stream << "<nodes>" << std::endl;
            base::write_nodes(value, edges);
            stream << "</nodes>" << std::endl;
        };
        
        void write_edges(const Cnf& value, const edges_data_t& edges) override {
            stream << "<edges>" << std::endl;
            base::write_edges(value, edges);
            stream << "</edges>" << std::endl;
        };
        
    public:
        CnfGexfStreamWriter(std::ostream& stream): base(stream) {};
    };
};

#endif /* cnfgexf_hpp */
