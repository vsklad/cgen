//
//  Propositional Logic Engine (PLE) Library
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef reducible_hpp
#define reducible_hpp

namespace ple {

    // number of argumens when multiple passed to a function
    typedef size_t ArgsSize;
    
    //Reducible is an abstract class implementing reduction of binary functions to 2 arguments
    // Parameters:
    //   T - final descendant class
    template <class T>
    class Reducible {
    public:
        typedef T* const (T::*Function1)(const T* const x);
        typedef T* const (T::*Function2)(const T* const x, const T* const y);
        typedef T* const (T::*Function3)(const T* const x, const T* const y, const T* const z);
        typedef T* const (T::*FunctionN)(const T* const args[], const ArgsSize args_size);
        
    protected:
        // reduce function with multiple operands by invoking it with 2 at a time consecutively
        inline static void reduce(T* const instance, Function2 function, const T* const args[], const ArgsSize args_size) {
            assert(args_size >= 2);
            (instance->*function)(args[0], args[1]);
            for (int i=2; i<args_size; i++) {
                assert(instance != args[i]); // because its overwritten already
                (instance->*function)(instance, args[i]);
            };
        };
    };
    
};

#endif /* reducible_hpp */
