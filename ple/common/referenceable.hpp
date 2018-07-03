//
//  Propositional Logic Engine (PLE) Library
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef referenceable_hpp
#define referenceable_hpp

namespace ple {

    typedef size_t ArraySize;
    
    class Referenceable {
    private:
        mutable unsigned int ref_count_ = 0;
    public:
        Referenceable() = default;
        // copy constructor, must reset ref_count, i.e. do nothing;
        Referenceable(const Referenceable &value) {};
        virtual ~Referenceable() = default;
        // reference management
        inline void add_ref() const { this->ref_count_++; };
        inline void release() const { if (this->ref_count_ < 2) delete this; else this->ref_count_--; };
    };

    template <class T>
    class ReferenceableRef {
        friend class ReferenceableRef<T>;
    private:
        T* value_;
    protected:
        inline T* set_value(T* value) {
            if (this->value_ != value) {
                if (this->value_ != nullptr)
                    this->value_->release();
                if (value != nullptr)
                    value->add_ref();
                this->value_ = value;
            };
            return value;
        };
    public:
        ReferenceableRef(): value_(nullptr) {};
        ReferenceableRef(T* const value): value_(value) { if (value != nullptr) value->add_ref(); };
        ReferenceableRef(const ReferenceableRef &value): ReferenceableRef(value.value_) {};
        ~ReferenceableRef() { this->set_value(nullptr); };
        
        inline const operator T* const () const { return this->value_; };
        inline T* const operator -> () const { return this->value_; };
        inline T* const data() const { return this->value_; };

        inline ReferenceableRef& operator = (const ReferenceableRef& value) {
            this->set_value(value.value_);
            return *this;
        };
        
        inline ReferenceableRef& operator = (T* const value) {
            this->set_value(value);
            return *this;
        };
        
        friend inline void swap(ReferenceableRef &x, ReferenceableRef& y) {
            T* swap_value = x.value_;
            x.value_ = y.value_;
            y.value_ = swap_value;
        };
    };
};

#endif /* referenceable_hpp */
