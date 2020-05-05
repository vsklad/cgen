//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef containerindex_hpp
#define containerindex_hpp

#include "container.hpp"
#include "assertlevels.hpp"

namespace bal {
    
    // insertion point implementations should derive from this
    // the approach should be valid for any append-only index
    // where index size_ can be used as a version stamp
    // to validate validity of the insertion point when it is obtaioned and
    // then used only after some other operations are done
    struct container_index_insertion_point_t {
        container_offset_t container_offset;
        container_size_t version_stamp;
    };
    
#define __insertion_point_t_init(value) value.version_stamp = CONTAINER_END;
    
    // ContainerIndex implements a set of index instances
    // each index instance is a sequence of index items
    // each index item maps to an offset in the container
    template<typename INDEX_DATA_T, typename CONTAINER_DATA_T, typename INSTANCE_ITERATOR_T, typename INSERTION_POINT_T>
    class ContainerIndex: protected Container<INDEX_DATA_T> {
        template<class>
        friend class ContainerIndexIterator;
        
        static_assert(std::is_base_of<container_index_insertion_point_t, INSERTION_POINT_T>::value, "Invalid type for INSERTION_POINT_T");
        
    public:
        using container_data_t = CONTAINER_DATA_T;
        using instance_iterator_t = INSTANCE_ITERATOR_T;
        using insertion_point_t = INSERTION_POINT_T;
        
    public:
        // a method for determining instance offset for container data
        // must either return instance offset or assert/throw an exception
        typedef container_offset_t (instance_determiner_t)(const container_data_t* const value);
        typedef instance_determiner_t* instance_determiner_p;
        
    protected:
        const Container<container_data_t>* const p_container_;
        Container<container_offset_t> instances_;
        
    private:
        container_size_t transaction_size_ = CONTAINER_END;
        container_size_t transaction_container_size_ = CONTAINER_END;
        container_size_t transaction_instances_size_ = CONTAINER_END;
        container_size_t version_stamp_ = 0;
        
    protected:
        virtual void rollback(const container_size_t size,
                              const container_size_t instances_size,
                              const container_size_t container_size) {
            assert(size != CONTAINER_END && this->size_ >= size);
            assert(instances_.size_ >= instances_size);
            this->size_ = size;
            instances_.size_ = instances_size;
        };
        
        inline void insertion_point_init(insertion_point_t& insertion_point) const {
            insertion_point.version_stamp = version_stamp_;
        };
        
        inline void insertion_point_invalidate() {
            version_stamp_++;
            assert(version_stamp_ < CONTAINER_END);
        };
        
    public:
        ContainerIndex(const Container<container_data_t>* const p_container): p_container_(p_container) {
            assert(p_container != nullptr);
        };
        
        virtual bool insertion_point_is_valid(const insertion_point_t& insertion_point) const {
            return insertion_point.version_stamp == version_stamp_;
        };
        
        virtual size_t memory_size() const {
            return Container<INDEX_DATA_T>::memory_size() + instances_.memory_size();
        };
        
        virtual void reset(const container_size_t instances_size, const container_size_t index_size) {
            _assert_level_1(!transaction_is_in());
            instances_.reset(instances_size);
            instances_.append(CONTAINER_END, instances_size);
            Container<INDEX_DATA_T>::reset(index_size);
        };
        
        inline void transaction_begin() {
            _assert_level_1(!transaction_is_in());
            transaction_size_ = this->size_;
            transaction_container_size_ = p_container_->size_;
            transaction_instances_size_ = instances_.size_;
        };
        
        inline void transaction_commit() {
            _assert_level_1(transaction_is_in());
            transaction_size_ = CONTAINER_END;
            transaction_container_size_ = CONTAINER_END;
            transaction_instances_size_ = CONTAINER_END;
        };
        
        inline void transaction_rollback() {
            _assert_level_1(transaction_is_in() && this->size_ >= transaction_size_);
            rollback(transaction_size_, transaction_instances_size_, transaction_container_size_);
            transaction_size_ = CONTAINER_END;
            transaction_container_size_ = CONTAINER_END;
            transaction_instances_size_ = CONTAINER_END;
        };
        
        inline bool transaction_is_in() const {
            return transaction_size_ != CONTAINER_END;
        };
        
        inline bool transaction_offset_is_immutable(const container_offset_t offset) const {
            return transaction_size_ != CONTAINER_END && offset < transaction_size_;
        };
    };
    
    // iterates over the whole index
    // when one instance is finished, starts the next one
    // returns data pointers for the target container
    // note that these pointers may be invalidated during
    // subsequent changes made to the container
    
    template<class INDEX_T>
    class ContainerIndexIterator {
    public:
        using container_data_t = typename INDEX_T::container_data_t;
        using instance_iterator_t = typename INDEX_T::instance_iterator_t;
        
    private:
        const INDEX_T& index_;
        instance_iterator_t instance_iterator_;
        container_offset_t instance_offset_;
        container_offset_t container_offset_;
        
    private:
        // find the next instance with at least one item
        inline void load_instance() {
            // move to the next instance which has items
            while (instance_offset_ < index_.instances_.size_ &&
                   index_.instances_.data_[instance_offset_] == CONTAINER_END) {
                instance_offset_++;
            };
            if (instance_offset_ < index_.instances_.size_) {
                container_offset_ = instance_iterator_.first(instance_offset_);
            } else {
                container_offset_ = CONTAINER_END;
            };
        };
        
        inline void next_item() {
            if (index_.instances_.size_ > 0) {
                container_offset_ = instance_iterator_.next();
                if (container_offset_ == CONTAINER_END) {
                    instance_offset_++;
                    load_instance();
                };
            };
        };
        
    public:
        // initialize at begin if b_begin is specified otherwise end
        ContainerIndexIterator(const INDEX_T& index, const bool b_begin): index_(index), instance_iterator_(index) {
            if (b_begin) {
                instance_offset_ = 0;
                load_instance();
            } else {
                instance_offset_ = index_.instances_.size_;
                container_offset_ = CONTAINER_END;
            };
        };
        
        container_data_t* operator *() {
            _assert_level_1(container_offset_ != CONTAINER_END);
            _assert_level_1(container_offset_ < index_.p_container_->size_);
            return index_.p_container_->data_ + container_offset_;
        };
        
        inline ContainerIndexIterator& operator ++() {
            next_item();
            return *this;
        };
        
        inline ContainerIndexIterator operator ++(int) {
            ContainerIndexIterator result = *this;
            next_item();
            return result;
        };
        
        inline bool operator == (const ContainerIndexIterator& rhs) const {
            return instance_offset_ == rhs.instance_offset_ && container_offset_ == rhs.container_offset_;
        };
        
        inline bool operator != (const ContainerIndexIterator& rhs) const {
            return !(*this == rhs);
        };
    };
}

#endif /* containerindex_hpp */
