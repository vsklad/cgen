//
//  Propositional Logic Engine (PLE) Library
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef ref_hpp
#define ref_hpp

#include <assert.h>
#include "referenceable.hpp"
#include "reducible.hpp"
#include "variables.hpp"

namespace ple {
    
    template<class T> class Ref;
    
    template<class T>
    class Allocator {
    public:
        virtual T* new_instance(Ref<T>* ref) {
            return ref->set_value(new T());
        };
        virtual T* new_constant(Ref<T>* ref, const typename T::ScalarValue value) {
            return new_instance(ref)->assign(value);
        };
        virtual T* new_variable(Ref<T>* ref, VariableGenerator& generator) {
            return new_instance(ref)->assign(generator);
        };
    };
    
    template<class T>
    class RefTarget: public virtual Referenceable {
    public:
        using Allocator = Allocator<T>;
    };
    
    // may not contain any member variables, static only
    // may not contain virtual methods
    template<class T>
    class Ref: public ReferenceableRef<T>  {
    private:
        friend class Allocator<T>;
        static typename T::Allocator allocator_;
        
    public:
        Ref() = default;
        Ref(T* const value): ReferenceableRef<T>(value) {};
        Ref(const Ref &value): ReferenceableRef<T>(value) {};
        Ref(const typename T::ScalarValue value) { allocator_.new_constant(this, value); };
        Ref(VariableGenerator& generator) { allocator_.new_variable(this, generator); };
        
        inline operator const typename T::ScalarValue() const { return *(this->data()); };
        
        inline Ref& operator = (const Ref& value) {
            ReferenceableRef<T>::operator=(value);
            return *this;
        };
        
        inline Ref& operator = (T* const value) {
            ReferenceableRef<T>::operator=(value);
            return *this;
        };
        
        inline Ref& operator = (const typename T::ScalarValue value) {
            allocator_.new_constant(this, value);
            return *this;
        };
        
        inline void new_instance() {
            allocator_.new_instance(this);
        };
        
        static typename T::Allocator& allocator() { return allocator_; };
    };
    
    template<class T> typename T::Allocator Ref<T>::allocator_;
    
    template<class T>
    class RefArray {
    protected:
        const ArraySize size_;
        Ref<T>* values_ = nullptr;
    
    protected:
        // accepts anything assignable to T_REF as initializer return type
        template<typename F>
        inline void initialize(F &initializer) {
            for (ArraySize i = 0; i < size_; i++) {
                this->values_[i] = initializer(i);
            };
        };
        
        // runs function for every element
        
        inline void execute_elementwise(typename Reducible<T>::Function1 function, const RefArray* const x) {
            for (ArraySize i = 0; i < size_; i++) {
                values_[i].new_instance();
                (values_[i]->*function)(x->data()[i]);
            };
        };
        
        inline void execute_elementwise(typename Reducible<T>::Function2 function, const RefArray* const x, const RefArray* const y) {
            for (ArraySize i = 0; i < size_; i++) {
                values_[i].new_instance();
                (values_[i]->*function)(x->data()[i], y->data()[i]);
            };
        };
        
        inline void execute_elementwise(typename Reducible<T>::Function3 function, const RefArray* const x, const RefArray* const y, const RefArray* const z) {
            for (ArraySize i = 0; i < size_; i++) {
                values_[i].new_instance();
                (values_[i]->*function)(x->data()[i], y->data()[i], z->data()[i]);
            };
        };
        
        template <class T_DESCENDANT>
        inline void execute_elementwise(typename Reducible<T>::FunctionN function, const T_DESCENDANT* const args[], const ArgsSize args_size) {
            for (ArraySize i = 0; i < size_; i++) {
                const T* args_i[args_size];
                for (int arg_index=0; arg_index < args_size; arg_index++)
                    args_i[arg_index] = args[arg_index]->data()[i];
                
                values_[i].new_instance();
                (values_[i]->*function)(args_i, args_size);
            };
        };
        
        inline void assign(const RefArray* value) {
            for (ArraySize i = 0; i < size_; i++) {
                values_[i] = value->values_[i];
            };
        };
        
    protected:
        // for StaticRefArray to implement alternative memory allocations
        RefArray(Ref<T>* values, const ArraySize size): values_(values), size_(size) {};
        
    public:
        RefArray(const ArraySize size): size_(size) {
            values_ = new Ref<T>[size];
        };
        
        ~RefArray() {
            delete[] values_;
        };
        
        RefArray(const std::initializer_list<Ref<T>> &values): RefArray(values.size()) {
            assert(sizeof(Ref<T>) == sizeof(T*));
            RefArray<T>::operator=(values);
        };
         
        RefArray(const std::initializer_list<T*> &values): RefArray(values.size()) {
            assert(sizeof(Ref<T>) == sizeof(T*));
            for (ArraySize i=0; i < size_; i++) {
                values_[i] = *(values.begin() + i);
            };
        };
        
        RefArray(const std::initializer_list<typename T::ScalarValue> &values): RefArray(values.size()) {
            assert(sizeof(Ref<T>) == sizeof(T*));
            for (ArraySize i = 0; i < size_; i++) {
                values_[i] = *(values.begin() + i);
            };
        };
        
        inline RefArray& operator = (VariableGenerator& generator) {
            for (ArraySize i = 0; i < size_; i++) {
                values_[i] = generator;
            };
            return *this;
        };
        
        inline RefArray& operator = (const std::initializer_list<Ref<T>> &values) {
            for (ArraySize i=0; i < size_; i++) {
                values_[i] = *(values.begin() + i);
            };
            return *this;
        };

        inline Ref<T>& operator [] (ArraySize index) { return values_[index]; };
        inline const Ref<T>& operator [] (ArraySize index) const { return values_[index]; };
        inline T* const * const data() const { return (T* const * const)(values_); };
        inline const ArraySize size() const { return size_; };
        
        // default output - iterate elements
        friend std::ostream& operator << (std::ostream& stream, const RefArray& value) {
            stream << "{";
            for (ArraySize i = 0; i < value.size_; i++) {
                if (i > 0) {
                    stream << ", ";
                };
                stream << *value.values_[i].data();
            };
            stream << "}";
            return stream;
        };
    };
    
    template<class T, ArraySize SIZE>
    class StaticRefArray: public RefArray<T> {
    private:
        Ref<T> static_values_[SIZE];
        
    public:
        StaticRefArray(): RefArray<T>(static_values_, SIZE) {};
        ~StaticRefArray() {
            RefArray<T>::values_ = nullptr;
        };
    };
};

#endif /* ref_hpp */
