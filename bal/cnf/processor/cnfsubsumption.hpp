//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfsubsumption_hpp
#define cnfsubsumption_hpp

#include "cnfprocessor.hpp"

namespace bal {

    class CnfSubsumptionOptimizer: public CnfProcessor {
    private:
        inline processor_result_t process_clause_subsumption(uint32_t* const p_clause);
        
        template<clause_size_t ca_size>
        inline processor_result_t process_clause_subsumption_ca(uint32_t* const p_clause);
        
    public:
        CnfSubsumptionOptimizer(Cnf& cnf): CnfProcessor(cnf) {};
        bool execute() override;
    };
    
};

#endif /* cnfsubsumption_hpp */
