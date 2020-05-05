//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef referenceable_hpp
#define referenceable_hpp

#include <cstddef>
#include <type_traits>
#include <ostream>
#include "assertlevels.hpp"

namespace bal {
    
    class Referenceable {
    private:
        mutable unsigned int ref_count_ = 0;
    public:
        Referenceable() = default;
        // copy constructor, must reset ref_count, i.e. do nothing;
        Referenceable(const Referenceable &value) {};
        virtual ~Referenceable() = default;
        // reference management
        void add_ref() const { this->ref_count_++; };
        void release() const { if (this->ref_count_ < 2) delete this; else this->ref_count_--; };
    };
    
    template<class T, typename value_t>
    class Assignable {
    public:
        virtual void assign(const value_t value) = 0;
        virtual operator const value_t() const = 0;
        
        static T* static_assign(T* instance, const value_t value) {
            instance = new T();
            instance->assign(value);
            return instance;
        };
    };

    template<class T, typename value_t,
            typename std::enable_if<std::is_base_of<Assignable<T, value_t>, T>::value, int>::type = 0>
    bool operator==(const T& lhs, const value_t& rhs) { return (value_t)lhs == rhs; };

    template<class T, typename value_t,
            typename std::enable_if<std::is_base_of<Assignable<T, value_t>, T>::value, int>::type = 0>
    bool operator==(const value_t& lhs, const T& rhs) { return lhs == (value_t)rhs; };

    template <class T>
    class Ref {
        /*
        static_assert(std::is_base_of<Referenceable, T>::value, "T must be a descendant of Referenceable");
        */
    private:
        mutable T* value_;
        
    protected:
        T* set_value(T* value) const {
            if (value_ != value) {
                if (value_ != nullptr)
                    value_->release();
                if (value != nullptr)
                    value->add_ref();
                value_ = value;
            };
            return value;
        };
        
    public:
        Ref(): value_(nullptr) {};
        Ref(T* const value): value_(value) {
            if (value != nullptr) {
                value->add_ref();
            };
        };
        
        Ref(const Ref& value): Ref(value.value_) {};
        Ref(Ref&& value) { operator=(std::move(value)); };

        ~Ref() { this->set_value(nullptr); };
        
        operator T* () const { return value_; };
        T* operator -> () const { return value_; };
        T* data() const { return value_; };

        Ref& operator = (T* const value) {
            this->set_value(value);
            return *this;
        };
        
        Ref& operator = (const Ref& value) {
            this->set_value(value.value_);
            return *this;
        };
        
        Ref& operator = (Ref&& value) {
            value_ = value.value_;
            value.value_ = nullptr;
            return *this;
        };
        
        // generic assignment
        
        template<typename value_t,
            typename std::enable_if<std::is_base_of<Assignable<T, value_t>, T>::value, int>::type = 0>
        Ref(const value_t& value): value_(nullptr) { this->operator=(value); };
        
        template<typename value_t,
            typename std::enable_if<std::is_base_of<Assignable<T, value_t>, T>::value, int>::type = 0>
        Ref& operator = (const value_t& value) {
            return operator=(T::static_assign(value_, value));
        };
        
        template<typename value_t,
            typename std::enable_if<std::is_base_of<Assignable<T, value_t>, T>::value, int>::type = 0>
        operator const value_t() const { return *(value_); };
        
        template<typename value_t,
            typename std::enable_if<std::is_base_of<Assignable<T, value_t>, T>::value, int>::type = 0>
        friend bool operator==(const Ref& lhs, const value_t& rhs) { return (value_t)*lhs.value_ == rhs; };

        template<typename value_t,
            typename std::enable_if<std::is_base_of<Assignable<T, value_t>, T>::value, int>::type = 0>
        friend bool operator==(const value_t& rhs, const Ref& lhs) { return lhs == (value_t)*rhs.value_; };

        // default output
        
        friend std::ostream& operator << (std::ostream& stream, const Ref<T>& value) {
            _assert_level_0(value.value_ != nullptr);
            if (value.value_ != nullptr) {
                stream << *(value.value_);
            } else {
                stream << "?";
            };
            return stream;
        };
        
        // default output - iterate array elements
        template<size_t SIZE>
        friend std::ostream& operator << (std::ostream& stream, const Ref<T>(&value)[SIZE]) {
            stream << "{";
            for (auto i = 0; i < SIZE; i++) {
                if (i > 0) {
                    stream << ", ";
                };
                stream << value[i];
            };
            stream << "}";
            return stream;
        };
        
        friend void swap(Ref &x, Ref& y) {
            T* swap_value = x.value_;
            x.value_ = y.value_;
            y.value_ = swap_value;
        };
    };
    
    template<class T, size_t SIZE>
    T* const * const ref_array_data(const Ref<T>(&value)[SIZE]) {
        static_assert(sizeof(Ref<T>) == sizeof(T*), "");
        return (T* const * const)value;
    };
    
    template<class T, typename std::enable_if<std::is_base_of<Referenceable, T>::value, int>::type = 0>
    void new_instance(Ref<T>& value) {
        value = new T();
    };
    
    template<class T, typename std::enable_if<std::is_base_of<Referenceable, T>::value, int>::type = 0>
    Ref<T> new_instance_if_unassigned(T* const value) {
        Ref<T> result = value;
        if (value == nullptr) {
            new_instance(result);
        };
        return result;
    };
};

#endif /* referenceable_hpp */
