//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef field_hpp
#define field_hpp

#include "assertlevels.hpp"
#include "referenceable.hpp"

namespace bal {
    
    // Primitive is an abstract base class for BAL primitives e.g. bits and words
    // defines basic assignment, encoding and deconing methods
    // Parameters:
    //   T - final descendant class
    template <class T>
    class Element: public virtual Referenceable {
    public:
        virtual T* assign(const T* const value) = 0;
        virtual T* assign(const T& value) { return this->assign(&value); };

        // indicates a constant, i.e. a scalar value
        virtual bool is_constant() const = 0;
    };
    
    // static_assert failure value for templates
    template<typename T>
    struct not_implemented: std::false_type {};
    
    template <class T>
    using Function2_ = Ref<T> (T* const r, const T* const x, const T* const y);
    
    // reduces N-ary operation through a sequence of binary operations assuming associativity
    // prefer to std::accumulate() and std::reduce() to reuse instances/memory where possible
    // also, to ensure result is not one of the arguments
    template <class T>
    Ref<T> reduce(T* const r, Function2_<T> function, const T* const args[], const std::size_t args_size) {
        _assert_level_1(args_size >= 2);
        Ref<T> result = function(r, args[0], args[1]);
        for (auto i = 2; i < args_size; i++) {
            _assert_level_1(r != args[i]); // because its overwritten already
            result = function(result, result, args[i]);
        };
        return result;
    };
    
    // the below templates define operations for Element descendants
    // they are mostly stubs as actuall impementation should be specialized
    // r is an optional result instance which may or may not be used
    //   that is, specific implementation can return a different instance
    
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> inv(T* const r, const T* const x) {
        static_assert(not_implemented<T>::value, "not implemented");
    };

    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> con2(T* const r, const T* const x, const T* const y) {
        static_assert(not_implemented<T>::value, "not implemented");
    };
    
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> dis2(T* const r, const T* const x, const T* const y) {
        static_assert(not_implemented<T>::value, "not implemented");
    };
    
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> eor2(T* const r, const T* const x, const T* const y) {
        static_assert(not_implemented<T>::value, "not implemented");
    };
    
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::valueint>::type = 0>
    Ref<T> add2(T* const r, const T* const x, const T* const y) {
        static_assert(not_implemented<T>::value, "not implemented");
    };
    
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> con2_resolver(T* const r, const T* const x, const T* const y) {
        return com2(r, x, y);
    };
    
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> dis2_resolver(T* const r, const T* const x, const T* const y) {
        return dis2(r, x, y);
    };
    
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> eor2_resolver(T* const r, const T* const x, const T* const y) {
        return eor2(r, x, y);
    };
    
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> add2_resolver(T* const r, const T* const x, const T* const y) {
        return add2(r, x, y);
    };

    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> con(T* const r, const T* const args[], const std::size_t args_size) {
        return reduce(r, con2_resolver, args, args_size);
    };
    
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> dis(T* const r, const T* const args[], const std::size_t args_size) {
        return reduce(r, dis2_resolver, args, args_size);
    };
    
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> eor(T* const r, const T* const args[], const std::size_t args_size) {
        return reduce(r, eor2_resolver, args, args_size);
    };
    
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> add(T* const r, const T* const args[], const std::size_t args_size) {
        return reduce(r, add2_resolver, args, args_size);
    };
    
    // n can be positive or negative, positive means high to low index
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> shr(T* const r, const T* const value, const int n) {
        static_assert(not_implemented<T>::value, "not implemented");
    };
    
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> shl(T* const r, const T* const value, const int n) {
        return shr(r, value, -n);
    };
    
    // n can be positive or negative, positive means high to low index
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> rotr(T* const r, const T* const value, const int n) {
        static_assert(not_implemented<T>::value, "not implemented");
    };
    
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> rotl(T* const r, const T* const value, const int n) {
        return rotr(r, value, -n);
    };

    // x&y ^ ~x&z
    // = x&y^(1^x)&z = x&y^x&z^z = x&(y^z)^z
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> ch(T* const r, const T* const x, const T* const y, const T* const z) {
        Ref<T> t = eor2(r, y, z);
        t = con2(t.data(), x, t.data());
        return eor2(t.data(), t.data(), z);
    };

    // x&y ^ x&z ^ y&z
    // = x&(y^z) ^ y&z
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> maj(T* const r, const T* const x, const T* const y, const T* const z) {
        Ref<T> t1 = eor2(r, y, z);
        t1 = con2(t1.data(), x, t1.data());
        Ref<T> t2 = con2((T*)nullptr, y, z);
        return eor2(t1.data(), t1.data(), t2.data());
    };
    
    // x ^ y ^ z
    template <class T, typename std::enable_if<std::is_base_of<Element<T>, T>::value, int>::type = 0>
    Ref<T> parity(T* const r, const T* const x, const T* const y, const T* const z) {
        const T* const args[3] = { x, y, z };
        return eor(r, args, 3);
    };
};

#endif /* field_hpp */
