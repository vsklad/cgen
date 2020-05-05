//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef operators_hpp
#define operators_hpp

#include "referenceable.hpp"
#include "field.hpp"

namespace bal {

    // Unary/Binary
    //   n = (Ref&, ...)
    
    template<class T>
    Ref<T> operator ~ (const Ref<T>& value) {
        return inv((T*)nullptr, value.data());
    };
    
    template<class T>
    Ref<T> operator & (const Ref<T>& x, const Ref<T>& y) {
        return con2((T*)nullptr, x.data(), y.data());
    };
    
    template<class T>
    Ref<T> operator | (const Ref<T>& x, const Ref<T>& y) {
        return dis2((T*)nullptr, x.data(), y.data());
    };
    
    template<class T>
    Ref<T> operator ^ (const Ref<T>& x, const Ref<T>& y) {
        return eor2((T*)nullptr, x.data(), y.data());
    };
    
    template<class T>
    Ref<T> operator + (const Ref<T>& x, const Ref<T>& y) {
        return add2((T*)nullptr, x.data(), y.data());
    };
    
    template<class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> ch(const Ref<T>& x, const Ref<T>& y, const Ref<T>& z) {
        return ch((T*)nullptr, x.data(), y.data(), z.data());
    };
    
    template<class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> maj(const Ref<T>& x, const Ref<T>& y, const Ref<T>& z) {
        return maj((T*)nullptr, x.data(), y.data(), z.data());
    };
    
    template<class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> parity(const Ref<T>& x, const Ref<T>& y, const Ref<T>& z) {
        return parity((T*)nullptr, x.data(), y.data(), z.data());
    };
    
    // N-ary
    //   n = (Ref&)
    
    template<class T, std::size_t SIZE>
    Ref<T> con(const Ref<T> (&args)[SIZE]) {
        return con((T*)nullptr, ref_array_data(args), SIZE);
    };
    
    template<class T, std::size_t SIZE>
    Ref<T> dis(const Ref<T> (&args)[SIZE]) {
        return dis((T*)nullptr, ref_array_data(args), SIZE);
    };
    
    template<class T, std::size_t SIZE>
    Ref<T> eor(const Ref<T> (&args)[SIZE]) {
        return eor((T*)nullptr, ref_array_data(args), SIZE);
    };
    
    template<class T, std::size_t SIZE>
    Ref<T> add(const Ref<T> (&args)[SIZE]) {
        return add((T*)nullptr, ref_array_data(args), SIZE);
    };
    
    // Incremental
    //   & ()= (Ref&, ...)
    
    template<class T>
    Ref<T> operator &= (Ref<T>& x, const Ref<T>& y) {
        return con2(x.data(), x.data(), y.data());
    };
    
    template<class T>
    Ref<T> operator |= (Ref<T>& x, const Ref<T>& y) {
        return dis2(x.data(), x.data(), y.data());
    };
    
    template<class T>
    Ref<T> operator ^= (Ref<T>& x, const Ref<T>& y) {
        return eor2(x.data(), x.data(), y.data());
    };
    
    template<class T>
    Ref<T> operator += (Ref<T>& x, const Ref<T>& y) {
        return add2(x.data(), x.data(), y.data());
    };
    
    // Shifting
    //   n = (Ref&, ...)
    
    template<class T>
    inline Ref<T> shr(const Ref<T>& value, const int n) {
        return shr((T*)nullptr, value.data(), n);
    };
    
    template<class T>
    inline Ref<T> shl(const Ref<T> &value, const int n) {
        return shl((T*)nullptr, value.data(), n);
    };
    
    template<class T>
    inline Ref<T> rotr(const Ref<T> &value, const int n) {
        return rotr((T*)nullptr, value.data(), n);
    };
    
    template<class T>
    inline Ref<T> rotl(const Ref<T> &value, const int n) {
        return rotl((T*)nullptr, value.data(), n);
    };
    
};

#endif /* operators_hpp */
