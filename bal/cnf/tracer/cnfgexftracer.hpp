//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfgexftracer_hpp
#define cnfgexftracer_hpp

#include "cnf.hpp"
#include "cnfgexf.hpp"
#include "cnfstreamtracer.hpp"

namespace bal {
    
    // initial state of the graph is state of the Cnf at the start of processing
    // final state is the state of Cnf at the end of the processing
    // all variables remain the same, they can be assigned to constants or other variables
    // however, new variables may not appear
    // once a clause is removed, it may be added again but not just reappear
    // approach
    //   1) record two timestamps for each clause: start (0 default) and end (max_xxx default);
    //      changes to an aggregated clause do not matter since variables are the same (?)
    //      this may influence edge weights but lets ignore that for now
    //   2) build and write both edges and nodes at the end
    //   3) determine timestamp range for variables from the clauses; also at the end
    //      however, whether the variable is excluded or not, follows from the final map
    //      there map must match clauses
    //      i.e. no clauses must exist for assigned variables
    // Requires quite a lot of memory although is quite simple to implement
    // NODES: NOT IMPLEMENTED
    
    class CnfGexfTracer: public CnfStreamTracer, private CnfGexfStreamWriter<false, true> {
    private:
        
        using writer = CnfGexfStreamWriter<false, true>;
        
    private:
        const Cnf* p_cnf_ = nullptr;
        edges_data_t edges_;
        unsigned next_edge_id_;
        timestamp_t timestamp_;
        
    public:
        void start(const Cnf& cnf) override {
            _assert_level_0(p_cnf_ == nullptr);
            _assert_level_0(edges_.size() == 0);
            next_edge_id_ = 0;
            timestamp_ = TIMESTAMP_MIN;
            p_cnf_ = &cnf;
            writer::write_header(cnf);
            writer::compute_edges(cnf, timestamp_, edges_, next_edge_id_);
        };
        
        void finish() override {
            _assert_level_0(p_cnf_ != nullptr);
            write_nodes(*p_cnf_, edges_);
            write_edges(*p_cnf_, edges_);
            writer::write_footer(*p_cnf_);
            p_cnf_ = nullptr;
            edges_.clear();
        };
        
        void append_clause(const uint32_t* const p_clause) override {
            if (p_cnf_ != nullptr) {
                writer::compute_edge<false>(p_clause, ++timestamp_, edges_, next_edge_id_);
            };
        };
        
        void remove_clause(const container_offset_t offset) override {
            if (p_cnf_ != nullptr) {
                _assert_level_0(offset != CONTAINER_END);
                writer::compute_edge<true>(p_cnf_->get_clause_data(offset), ++timestamp_, edges_, next_edge_id_);
            };
        };
        
    public:
        CnfGexfTracer(std::ostream& stream): CnfStreamTracer(stream), writer(stream) {};
    };
    
};

#endif /* cnfgexftracer_hpp */
