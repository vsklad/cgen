//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef gf2_hpp
#define gf2_hpp

#include "referenceable.hpp"
#include "field.hpp"

namespace bal {

    template <class T>
    class GF2Element:
        public virtual Element<T>,
        public Assignable<T, bool>,
        public Assignable<T, signed>,
        public Assignable<T, unsigned> {

    private:
        static const Ref<T> identity_[2];
            
    public:
        void assign(const signed value) override {
            _assert_level_0(value == 0 || value == 1);
            assign(value == 1);
        };
        
        operator const signed() const override {
            return bool(*this) ? 0b1 : 0b0;
        };
            
        void assign(const unsigned value) override {
            _assert_level_0(value == 0 || value == 1);
            assign(value == 1);
        };
        
        operator const unsigned() const override {
            return bool(*this) ? 0b1 : 0b0;
        };
            
        template<typename value_t, typename std::enable_if<std::is_integral<value_t>::value, int>::type = 0>
        static T* static_assign(T* instance, const value_t& value) {
            _assert_level_0(value == 0 || value == 1);
            return identity_[value].data();
        };
    };

    template <class T>
    const Ref<T> GF2Element<T>::identity_[2] = {new T(0b0), new T(0b1)};

};

#endif /* gf2_hpp */
