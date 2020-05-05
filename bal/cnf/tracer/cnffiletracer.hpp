//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnffiletracer_hpp
#define cnffiletracer_hpp

#include "cnfstreamtracer.hpp"

namespace bal {
    
    template<class T>
    class CnfFileTracer: public T {
        static_assert(std::is_base_of<CnfStreamTracer, T>::value, "T must be a descendant of CnfStreamTracer");
    private:
        std::ofstream file_stream_;
    public:
        CnfFileTracer(const char* const file_name): file_stream_(file_name), T(file_stream_) {};
    };
    
};

#endif /* cnffiletracer_hpp */
