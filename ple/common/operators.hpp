//
//  Propositional Logic Engine (PLE) Library
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef operators_hpp
#define operators_hpp

#include "ref.hpp"
#include "entity.hpp"
#include "operations.hpp"

namespace ple {

    // PropositionalLogicEntity, PropositionalLogicEntity2, AriphmeticEntity2
    //   n = (Ref&, ...)
    
    template<class T>
    inline Ref<T> operator ~ (const Ref<T>& value) {
        Ref<T> result;
        result.new_instance();
        result->inv(value);
        return result;
    };
    
    template<class T>
    inline Ref<T> operator & (const Ref<T>& x, const Ref<T>& y) {
        Ref<T> result;
        result.new_instance();
        result->con2(x, y);
        return result;
    };
    
    template<class T>
    inline Ref<T> operator | (const Ref<T>& x, const Ref<T>& y) {
        Ref<T> result;
        result.new_instance();
        result->dis2(x, y);
        return result;
    };
    
    template<class T>
    inline Ref<T> operator ^ (const Ref<T>& x, const Ref<T>& y) {
        Ref<T> result;
        result.new_instance();
        result->eor2(x, y);
        return result;
    };
    
    template<class T>
    inline Ref<T> operator + (const Ref<T>& x, const Ref<T>& y) {
        Ref<T> result;
        result.new_instance();
        result->add2(x, y);
        return result;
    };
    
    template<class T>
    inline Ref<T> ch(const Ref<T>& x, const Ref<T>& y, const Ref<T>& z) {
        Ref<T> result;
        result.new_instance();
        result->ch(x, y, z);
        return result;
    };
    
    template<class T>
    inline Ref<T> maj(const Ref<T>& x, const Ref<T>& y, const Ref<T>& z) {
        Ref<T> result;
        result.new_instance();
        result->maj(x, y, z);
        return result;
    };
    
    template<class T>
    inline Ref<T> parity(const Ref<T>& x, const Ref<T>& y, const Ref<T>& z) {
        Ref<T> result;
        result.new_instance();
        result->parity(x, y, z);
        return result;
    };
    
    // PropositionalLogicEntity, AriphmeticEntity
    //   n = (RefArray&)
    
    template<class T>
    inline Ref<T> con(const RefArray<T> &args) {
        Ref<T> result;
        result.new_instance();
        result->con(args.data(), args.size());
        return result;
    };
    
    template<class T>
    inline Ref<T> dis(const RefArray<T> &args) {
        Ref<T> result;
        result.new_instance();
        result->dis(args.data(), args.size());
        return result;
    };
    
    template<class T>
    inline Ref<T> eor(const RefArray<T> &args) {
        Ref<T> result;
        result.new_instance();
        result->eor(args.data(), args.size());
        return result;
    };
    
    template<class T>
    inline Ref<T> add(const RefArray<T> &args) {
        Ref<T> result;
        result.new_instance();
        result->add(args.data(), args.size());
        return result;
    };
    
    // PropositionalLogicEntity2, AriphmeticEntity2
    //   & ()= (Ref&, ...)
    
    template<class T>
    inline Ref<T>& operator &= (Ref<T>& x, const Ref<T>& y) {
        // copy x to avoid overlap
        Ref<T> x_ = new T(x.ref());
        x->con2(x_, y);
        return x;
    };
    
    template<class T>
    inline Ref<T>& operator |= (Ref<T>& x, const Ref<T>& y) {
        // copy x to avoid overlap
        Ref<T> x_ = new T(x.ref());
        x->dis2(x_, y);
        return x;
    };
    
    template<class T>
    inline Ref<T>& operator ^= (Ref<T>& x, const Ref<T>& y) {
        x->eor2(x, y);
        return x;
    };
    
    template<class T>
    inline Ref<T>& operator += (Ref<T>& x, const Ref<T>& y) {
        x->add2(x, y);
        return x;
    };
    
    // ShiftableEntity
    //   n = (Ref&, ...)
    
    template<class T>
    inline Ref<T> shr(const Ref<T>& value, const int n) {
        Ref<T> result;
        result.new_instance();
        result->shr(value, n);
        return result;
    };
    
    template<class T>
    inline Ref<T> shl(const Ref<T> &value, const int n) {
        Ref<T> result;
        result.new_instance();
        result->shl(value, n);
        return result;
    };
    
    /*
    template<class T, template<class> class T_REF>
    inline T_REF<T> operator >> (const T_REF<T> &value, const int n) {
        return shr(value, n);
    };
    
    template<class T, template<class> class T_REF>
    inline T_REF<T> operator << (const T_REF<T> &value, const int n) {
        return shl(value, n);
    };
    */
    
    template<class T>
    inline Ref<T> rotr(const Ref<T> &value, const int n) {
        Ref<T> result;
        result.new_instance();
        result->rotr(value, n);
        return result;
    };
    
    template<class T>
    inline Ref<T> rotl(const Ref<T> &value, const int n) {
        Ref<T> result;
        result.new_instance();
        result->rotl(value, n);
        return result;
    };
    
};

#endif /* operators_hpp */
