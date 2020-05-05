//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfstreamtracer_hpp
#define cnfstreamtracer_hpp

#include <sstream>
#include "cnf.hpp"
#include "cnftracer.hpp"

namespace bal {

    class CnfStreamTracer: public CnfTracer {
    protected:
        std::ostream& stream;
    public:
        CnfStreamTracer(std::ostream& _stream): CnfTracer(), stream(_stream) {};
    };

}
    
#endif /* cnfstreamtracer_hpp */
