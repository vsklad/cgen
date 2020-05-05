//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef gf2n_hpp
#define gf2n_hpp

#include <iomanip>
#include <limits>
#include "assertlevels.hpp"
#include "referenceable.hpp"
#include "field.hpp"

namespace bal {

    // GF(2^N) element is modelled as an array of GF(2) elements of size N
    // GF2E is the type implementing GF(2) element
    
    template<std::size_t N_, class GF2E>
    class GF2NElement:
        public virtual Element<GF2NElement<N_, GF2E>>,
        public Assignable<GF2NElement<N_, GF2E>, signed>,
        public Assignable<GF2NElement<N_, GF2E>, unsigned>,
        public Assignable<GF2NElement<N_, GF2E>, signed long>,
        public Assignable<GF2NElement<N_, GF2E>, unsigned long>,
        public Assignable<GF2NElement<N_, GF2E>, signed long long>,
        public Assignable<GF2NElement<N_, GF2E>, unsigned long long> {
            
    private:
        Ref<GF2E> bits_[N_];
            
    private:
        // generic Assignable implementation
        // assigning from
        //   shorter types: leading bits expanded using 0 if necessary
        //   longer types: can be assigned from only if excessive leading bits are all 0
        //   negative values: not supported by design and intended purpose
        //     therefore, minimal value for any type is 0
        // assigning to
        //   shorter type: require that excessive bits are all zero
        //   longer type: converts without any issues
            
        template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        static constexpr T size_in_bits_(const T value) {
            return (value > 0) ? size_in_bits_(value >> 1) + 1 : 0;
        };
            
        template<typename T, typename std::enable_if<std::is_integral<T>::value && (std::numeric_limits<T>::max() > 0), int>::type = 0>
        void assign_from(const T value) {
            _assert_level_0(value >= 0);
            constexpr T T_MAX_SIZE = size_in_bits_(std::numeric_limits<T>::max());
            if (T_MAX_SIZE <= N_) {
                // assigning from a shorter type or a same sized type
                // first, assign all lower bits available from value
                // second, assume the remaining leading bits are all 0
                auto i = 0;
                T max_i = std::numeric_limits<T>::max();
                while (max_i > 0) {
                    bits_[i] = unsigned((value >> i) & 0b1);
                    max_i >>= 1;
                    i++;
                };
                while (i < N_) {
                    bits_[i] = 0b0;
                    i++;
                };
            } else {
                // assigning form a longer type
                // first, assign all lower bits
                // second, check that the remaining leading bits are all 0
                auto value_ = value;
                auto i = 0;
                while (i < N_) {
                    bits_[i] = unsigned(value_ & 0b1);
                    value_ >>= 1;
                };
                _assert_level_0(value_ == 0);
            };
        };
            
        // relies on bit conversion to bool
        // fails if there is at least one non-constant bit
        template<typename T, typename std::enable_if<std::is_integral<T>::value && (std::numeric_limits<T>::max() > 0), int>::type = 0>
        T assign_to() const {
            T value = 0;
            constexpr T T_MAX_SIZE = size_in_bits_(std::numeric_limits<T>::max());
            if (T_MAX_SIZE < N_) {
                // assigning to a shorter type
                auto i = 0;
                T max_i = std::numeric_limits<T>::max();
                while (max_i > 0) {
                    if ((bool)bits_[i]) {
                        value |= 0b1 << i;
                    };
                    max_i >>= 1;
                    i++;
                };
                while (i < N_) {
                    _assert_level_0((bool)bits_[i] == false);
                    i++;
                };
            } else {
                // assigning to a longer type or a same sized type
                for (auto i = 0; i < N_; i++) {
                    if ((bool)bits_[i]) {
                        value |= 0b1 << i;
                    };
                };
            };
            return value;
        };

    public:
        static_assert(N_ > 0, "trivial field with 1 element is not supported");
        static constexpr auto N = N_;
            
        // Element
            
        GF2NElement* assign(const GF2NElement* const value) override {
            std::copy(std::begin(value->bits_), std::end(value->bits_), std::begin(bits_));
            return this;
        };

        bool is_constant() const override {
            for (auto i = 0; i < N_; i++) {
                if (bits_[i] == nullptr || !bits_[i]->is_constant()) {
                    return false;
                };
            };
            return true;
        };

        // Assignable

        void assign(const signed value) override {
            assign_from<signed>(value);
        };
        
        operator const signed() const override { return assign_to<signed>(); };
        using Assignable<GF2NElement<N_, GF2E>, signed>::static_assign;
            
        void assign(const unsigned value) override {
            assign_from<unsigned>(value);
        };
        
        operator const unsigned() const override { return assign_to<unsigned>(); };
        using Assignable<GF2NElement<N_, GF2E>, unsigned>::static_assign;
            
        void assign(const signed long value) override {
            assign_from<signed long>(value);
        };
        
        operator const signed long() const override { return assign_to<signed long>(); };
        using Assignable<GF2NElement<N_, GF2E>, signed long>::static_assign;
        
        void assign(const unsigned long value) override {
            assign_from<unsigned long>(value);
        };
        
        operator const unsigned long() const override { return assign_to<unsigned long>(); };
        using Assignable<GF2NElement<N_, GF2E>, unsigned long>::static_assign;
            
        void assign(const signed long long value) override {
            assign_from<signed long long>(value);
        };
        
        operator const signed long long() const override { return assign_to<signed long long>(); };
        using Assignable<GF2NElement<N_, GF2E>, signed long long>::static_assign;
        
        void assign(const unsigned long long value) override {
            assign_from<unsigned long long>(value);
        };
        
        operator const unsigned long long() const override { return assign_to<unsigned long long>(); };
        using Assignable<GF2NElement<N_, GF2E>, unsigned long long>::static_assign;
            
        // Operators
        
        inline Ref<GF2E>& operator [] (const std::size_t idx) { return bits_[idx]; };
        inline const Ref<GF2E>& operator [] (const std::size_t idx) const { return bits_[idx]; };
        inline const GF2E* const * const data() const { return ref_array_data(bits_); };
            
        friend std::ostream& operator << (std::ostream& stream, const GF2NElement& value) {
            static_assert(N <= (sizeof(unsigned long long) << 3), "N is too large for this implementation");
            static_assert(N % 4 == 0, "N must be aligned with 4 in this implementation");
            if (value.is_constant()) {
                return stream << "0x" << std::setfill('0') << std::setw(N>>2) << std::hex << (unsigned long long)value;
            } else {
                return stream << value.bits_;
            };
        };
    };

    template<std::size_t N, class GF2E>
    Ref<GF2NElement<N, GF2E>> inv(GF2NElement<N, GF2E>* const r,
                                  const GF2NElement<N, GF2E>* const x) {
        Ref<GF2NElement<N, GF2E>> result = new_instance_if_unassigned(r);
        for (auto i = 0; i < N; i++) {
            (*result)[i] = inv((*result)[i].data(), (*x)[i].data());
        };
        return result;
    };
    
    template<std::size_t N, class GF2E>
    Ref<GF2NElement<N, GF2E>> con2(GF2NElement<N, GF2E>* const r,
                                   const GF2NElement<N, GF2E>* const x,
                                   const GF2NElement<N, GF2E>* const y) {
        Ref<GF2NElement<N, GF2E>> result = new_instance_if_unassigned(r);
        for (auto i = 0; i < N; i++) {
            (*result)[i] = con2((*result)[i].data(), (*x)[i].data(), (*y)[i].data());
        };
        return result;
    };
    
    template<std::size_t N, class GF2E>
    Ref<GF2NElement<N, GF2E>> dis2(GF2NElement<N, GF2E>* const r,
                                   const GF2NElement<N, GF2E>* const x,
                                   const GF2NElement<N, GF2E>* const y) {
        Ref<GF2NElement<N, GF2E>> result = new_instance_if_unassigned(r);
        for (auto i = 0; i < N; i++) {
            (*result)[i] = dis2((*result)[i].data(), (*x)[i].data(), (*y)[i].data());
        };
        return result;
    };
    
    template<std::size_t N, class GF2E>
    Ref<GF2NElement<N, GF2E>> eor2(GF2NElement<N, GF2E>* const r,
                                   const GF2NElement<N, GF2E>* const x,
                                   const GF2NElement<N, GF2E>* const y) {
        Ref<GF2NElement<N, GF2E>> result = new_instance_if_unassigned(r);
        for (auto i = 0; i < N; i++) {
            (*result)[i] = eor2((*result)[i].data(), (*x)[i].data(), (*y)[i].data());
        };
        return result;
    };
    
    template<std::size_t N, class GF2E>
    Ref<GF2NElement<N, GF2E>> con(GF2NElement<N, GF2E>* const r,
                                  const GF2NElement<N, GF2E>* const args[], const std::size_t args_size) {
        Ref<GF2NElement<N, GF2E>> result = new_instance_if_unassigned(r);
        for (auto i = 0; i < N; i++) {
            const GF2E* args_i[args_size];
            for (auto j = 0; j < args_size; j++) {
                args_i[j] = (*args[j])[i];
            };
            (*result)[i] = con((*result)[i].data(), args_i, args_size);
        };
        return result;
    };
    
    template<std::size_t N, class GF2E>
    Ref<GF2NElement<N, GF2E>> dis(GF2NElement<N, GF2E>* const r,
                                  const GF2NElement<N, GF2E>* const args[], const std::size_t args_size) {
        Ref<GF2NElement<N, GF2E>> result = new_instance_if_unassigned(r);
        for (auto i = 0; i < N; i++) {
            const GF2E* args_i[args_size];
            for (auto j = 0; j < args_size; j++) {
                args_i[j] = (*args[j])[i];
            };
            (*result)[i] = dis((*result)[i].data(), args_i, args_size);
        };
        return result;
    };
    
    template<std::size_t N, class GF2E>
    Ref<GF2NElement<N, GF2E>> eor(GF2NElement<N, GF2E>* const r,
                                  const GF2NElement<N, GF2E>* const args[], const std::size_t args_size) {
        Ref<GF2NElement<N, GF2E>> result = new_instance_if_unassigned(r);
        for (auto i = 0; i < N; i++) {
            const GF2E* args_i[args_size];
            for (auto j = 0; j < args_size; j++) {
                args_i[j] = (*args[j])[i];
            };
            (*result)[i] = eor((*result)[i].data(), args_i, args_size);
        };
        return result;
    };
    
    // default add2() implementation
    // unoptimised approach to calculate result and carry through other operations
    template<std::size_t N, class GF2E>
    Ref<GF2NElement<N, GF2E>> add2(GF2NElement<N, GF2E>* const r,
                                   const GF2NElement<N, GF2E>* const x,
                                   const GF2NElement<N, GF2E>* const y) {
        Ref<GF2NElement<N, GF2E>> result = new_instance_if_unassigned(r);
        
        const GF2E* const * const xref = x->data();
        const GF2E* const * const yref = y->data();
        const GF2E* args[3];
        Ref<GF2E> br;
        Ref<GF2E> bc;
        
        for (auto i = 0; i < N; i++) {
            swap(br, bc);
            
            if (i < N - 1) {
                bc = maj(bc.data(), xref[i], yref[i], br.data());
            };
            
            args[0] = br;
            args[1] = xref[i];
            args[2] = yref[i];
            br = eor(br.data(), args, 3);
            
            // assign at the end because this may reset one of the arguments
            // if the same as x, reuse x[i] for x[i+1]
            if (r == x) {
                swap((*result)[i], br);
            } else {
                (*result)[i] = br;
                br = nullptr;
            };
        };
        
        return result;
    };
    
    template<std::size_t N, class GF2E>
    Ref<GF2NElement<N, GF2E>> ch(GF2NElement<N, GF2E>* const r,
                                 const GF2NElement<N, GF2E>* const x,
                                 const GF2NElement<N, GF2E>* const y,
                                 const GF2NElement<N, GF2E>* const z) {
        Ref<GF2NElement<N, GF2E>> result = new_instance_if_unassigned(r);
        for (auto i = 0; i < N; i++) {
            (*result)[i] = ch((*result)[i].data(), (*x)[i].data(), (*y)[i].data(), (*z)[i].data());
        };
        return result;
    };
    
    template<std::size_t N, class GF2E>
    Ref<GF2NElement<N, GF2E>> maj(GF2NElement<N, GF2E>* const r,
                                  const GF2NElement<N, GF2E>* const x,
                                  const GF2NElement<N, GF2E>* const y,
                                  const GF2NElement<N, GF2E>* const z) {
        Ref<GF2NElement<N, GF2E>> result = new_instance_if_unassigned(r);
        for (auto i = 0; i < N; i++) {
            (*result)[i] = maj((*result)[i].data(), (*x)[i].data(), (*y)[i].data(), (*z)[i].data());
        };
        return result;
    };
    
    template<std::size_t N, class GF2E>
    Ref<GF2NElement<N, GF2E>> parity(GF2NElement<N, GF2E>* const r,
                                     const GF2NElement<N, GF2E>* const x,
                                     const GF2NElement<N, GF2E>* const y,
                                     const GF2NElement<N, GF2E>* const z) {
        Ref<GF2NElement<N, GF2E>> result = new_instance_if_unassigned(r);
        for (auto i = 0; i < N; i++) {
            (*result)[i] = parity((*result)[i].data(), (*x)[i].data(), (*y)[i].data(), (*z)[i].data());
        };
        return result;
    };
    
    // n can be positive or negative, positive means high to low index
    template<std::size_t N, class GF2E>
    Ref<GF2NElement<N, GF2E>> shr(GF2NElement<N, GF2E>* const r,
                                  const GF2NElement<N, GF2E>* const value, const int n) {
        Ref<GF2NElement<N, GF2E>> result = new_instance_if_unassigned(r);
        for (auto i = 0; i < N; i++) {
            if ((0 <= (i + n)) && ((i + n) < N))
                (*result)[i] = (*value)[i + n];
            else
                (*result)[i] = 0b0;
        };
        return result;
    };
    
    // n can be positive or negative, positive means high to low index
    template<std::size_t N, class GF2E>
    Ref<GF2NElement<N, GF2E>> rotr(GF2NElement<N, GF2E>* const r,
                                   const GF2NElement<N, GF2E>* const value, const int n) {
        Ref<GF2NElement<N, GF2E>> result = new_instance_if_unassigned(r);
        for (auto i = 0; i < N; i++) {
            (*result)[i] = (*value)[(i + n) % N];
        };
        return result;
    };
};

#endif /* gf2n_hpp */
