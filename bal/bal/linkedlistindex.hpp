//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef linkedlistindex_hpp
#define linkedlistindex_hpp

#include "assertlevels.hpp"
#include "containerindex.hpp"

namespace bal {
    
    // A simple structure for a linked list element
    struct list_index_item_t {
        container_offset_t next_offset;
        container_offset_t container_offset;
    };
    
    typedef enum {liipkRoot, liipkAfter} list_index_insertion_point_kind_t;
    
    struct list_index_insertion_point_t: container_index_insertion_point_t {
        list_index_insertion_point_kind_t kind;
        container_offset_t offset; // instance offset for liipkRoot, index offset otherwise
    };
    
    template<class FILTER_T>
    class Filterable {
    public:
        typedef bool (FILTER_T::*is_included_t)(const container_offset_t offset) const;
    };
    
    // iterates over all list items for particular instance
    // used as a default iterator for LinkedListsIndex
    template<class> class LinkedListsIndexInstanceOffsetIterator;
    
    // Lists stores and manipulates a set of lists
    // formed from objects stored in its data Container
    // for lists, insertion point is simply a pointer to offset value
    // where the new element is inserted
    template<typename CONTAINER_DATA_T>
    class LinkedListsIndex: public ContainerIndex<list_index_item_t, CONTAINER_DATA_T, LinkedListsIndexInstanceOffsetIterator<CONTAINER_DATA_T>, list_index_insertion_point_t> {
        template<class>
        friend class LinkedListsIndexInstanceOffsetIterator;
        template<typename, class FILTER_T, typename Filterable<FILTER_T>::is_included_t>
        friend class LinkedListsIndexInstanceFilteredOffsetIterator;
        
    private:
        using base_t = ContainerIndex<list_index_item_t, CONTAINER_DATA_T, LinkedListsIndexInstanceOffsetIterator<CONTAINER_DATA_T>, list_index_insertion_point_t>;
        
    public:
        LinkedListsIndex(const Container<CONTAINER_DATA_T>* const p_container): base_t(p_container) {};
        
        // iterate over all items in the list, call the supplied instance method
        template<typename CALLER_T, typename RESULT_T, RESULT_T (CALLER_T::*container_offset_function)(const container_offset_t offset) const, RESULT_T RESULT_CONTINUE>
        inline RESULT_T iterate_const(const CALLER_T* const p_caller, const container_offset_t instance_offset) const {
            RESULT_T result = RESULT_CONTINUE;
            container_offset_t offset = instance_offset < this->instances_.size_ ? this->instances_.data_[instance_offset] : CONTAINER_END;
            while (offset != CONTAINER_END) {
                result = (p_caller->*container_offset_function)(this->data_[offset].container_offset);
                if (result == RESULT_CONTINUE) {
                    offset = this->data_[offset].next_offset;
                } else {
                    break;
                };
            };
            return result;
        };
    };
    
    // single index is used to keep many lists at the same time
    // list instance offset is required for iterator to function for a particular list
    // returns data offset for the target container
    
    template<typename CONTAINER_DATA_T>
    class LinkedListsIndexInstanceOffsetIterator {
    protected:
        const LinkedListsIndex<CONTAINER_DATA_T>& index_;
        container_offset_t instance_offset_ = CONTAINER_END;
        container_offset_t item_offset_ = CONTAINER_END;
        
    public:
        LinkedListsIndexInstanceOffsetIterator(const LinkedListsIndex<CONTAINER_DATA_T>& index): index_(index) {};
        
        // positions at the first list item for the given instance
        // returns offset of the corresponding container data element or CONTAINER_END
        inline container_offset_t first(const container_offset_t instance_offset) {
            instance_offset_ = instance_offset;
            item_offset_ = instance_offset >= index_.instances_.size_ ? CONTAINER_END : index_.instances_.data_[instance_offset];
            return item_offset_ == CONTAINER_END ? CONTAINER_END : index_.data_[item_offset_].container_offset;
        };
        
        // moves to the next list item for the stored instance
        // returns offset of the corresponding container data element or CONTAINER_END
        inline container_offset_t next() {
            if (item_offset_ != CONTAINER_END) {
                item_offset_ = index_.data_[item_offset_].next_offset;
                if (item_offset_ != CONTAINER_END) {
                    return index_.data_[item_offset_].container_offset;
                };
            };
            return CONTAINER_END;
        };
    };
     
    // iterates items for the specified list instance
    // returns container offset for each item or COMTAINER_END at the end
    // skips items where IS_INCLUDE filter method returns false
    template<typename CONTAINER_DATA_T, class FILTER_T, typename Filterable<FILTER_T>::is_included_t IS_INCLUDED>
    class LinkedListsIndexInstanceFilteredOffsetIterator {
    private:
        const LinkedListsIndex<CONTAINER_DATA_T>& index_;
        container_offset_t instance_offset_ = CONTAINER_END;
        container_offset_t item_offset_ = CONTAINER_END;
        const FILTER_T& filter_;
        
    public:
        LinkedListsIndexInstanceFilteredOffsetIterator(const LinkedListsIndex<CONTAINER_DATA_T>& index, const FILTER_T& filter): index_(index), filter_(filter) {};
        
        inline container_offset_t first(const container_offset_t instance_offset) {
            instance_offset_ = instance_offset;
            item_offset_ = instance_offset < this->index_.instances_.size_ ? index_.instances_.data_[instance_offset] : CONTAINER_END;
            
            while (item_offset_ != CONTAINER_END) {
                const container_offset_t result = index_.data_[item_offset_].container_offset;
                if ((filter_.*IS_INCLUDED)(result)) {
                    return result;
                };
                item_offset_ = index_.data_[item_offset_].next_offset;
            };
            return CONTAINER_END;
        };
        
        inline container_offset_t next() {
            while (item_offset_ != CONTAINER_END) {
                item_offset_ = index_.data_[item_offset_].next_offset;
                if (item_offset_ != CONTAINER_END) {
                    const container_offset_t result = index_.data_[item_offset_].container_offset;
                    if ((filter_.*IS_INCLUDED)(result)) {
                        return result;
                    };
                };
            };
            return CONTAINER_END;
        };
    };
    
    template<typename CONTAINER_DATA_T>
    class SimpleLinkedListsIndex: public LinkedListsIndex<CONTAINER_DATA_T> {
        template<typename, class FILTER_T, typename Filterable<FILTER_T>::is_included_t>
        friend class SimpleLinkedListsIndexInstanceFilteredOptimizingOffsetIterator;
        template<typename, unsigned, class FILTER_T, typename Filterable<FILTER_T>::is_included_t>
        friend class SimpleLinkedListsIndexMergedFilteredOptimizingInstanceOffsetIterator;

    private:
        Container<container_offset_t> instances_last_;
        
    public:
        SimpleLinkedListsIndex(const Container<CONTAINER_DATA_T>* const p_container): LinkedListsIndex<CONTAINER_DATA_T>(p_container) {};
        
        size_t memory_size() const override {
            return LinkedListsIndex<CONTAINER_DATA_T>::memory_size() + instances_last_.memory_size();
        };
        
        void reset(const container_size_t instances_size, const container_size_t index_size) override {
            LinkedListsIndex<CONTAINER_DATA_T>::reset(instances_size, index_size);
            instances_last_.reset(instances_size);
            instances_last_.append(CONTAINER_END, instances_size);
        };
        
        // always append to the end of the list, assume index is updated together with the container
        // i.e. container_offset is always at the end of the container
        inline void append(const container_size_t instance_offset, const container_size_t container_offset) {
            if (instance_offset >= this->instances_.size_) {
                this->instances_.append(CONTAINER_END, instance_offset - this->instances_.size_ + 1);
                this->instances_last_.append(CONTAINER_END, instance_offset - instances_last_.size_ + 1);
            };
            
            this->reserve(1); // size_ is the valid offset for the new item
            this->data_[this->size_] = { CONTAINER_END, container_offset };
            if (this->instances_.data_[instance_offset] == CONTAINER_END) {
                // the instance list is empty
                assert(instances_last_.data_[instance_offset] == CONTAINER_END);
                this->instances_.data_[instance_offset] = this->size_;
            } else {
                // after the last item
                assert(instances_last_.data_[instance_offset] != CONTAINER_END);
                this->data_[instances_last_.data_[instance_offset]].next_offset = this->size_;
            }
            instances_last_.data_[instance_offset] = this->size_;
            this->size_++;
        };
    };
    
    // iterates items for the specified list instance
    // returns container offset for each item or COMTAINER_END at the end
    // skips items where IS_INCLUDE filter method returns false
    // TODO: update instance_last_ when removing?
    template<typename CONTAINER_DATA_T, class FILTER_T, typename Filterable<FILTER_T>::is_included_t IS_INCLUDED>
    class SimpleLinkedListsIndexInstanceFilteredOptimizingOffsetIterator {
    private:
        const SimpleLinkedListsIndex<CONTAINER_DATA_T>& index_;
        container_offset_t instance_offset_ = CONTAINER_END;
        container_offset_t item_offset_ = CONTAINER_END;
        const FILTER_T& filter_;
        
    public:
        SimpleLinkedListsIndexInstanceFilteredOptimizingOffsetIterator(const SimpleLinkedListsIndex<CONTAINER_DATA_T>& index, const FILTER_T& filter): index_(index), filter_(filter) {};
        
        inline container_offset_t first(const container_offset_t instance_offset) {
            instance_offset_ = instance_offset;
            item_offset_ = instance_offset < this->index_.instances_.size_ ? index_.instances_.data_[instance_offset] : CONTAINER_END;
            
            while (item_offset_ != CONTAINER_END) {
                const container_offset_t result = index_.data_[item_offset_].container_offset;
                if ((filter_.*IS_INCLUDED)(result)) {
                    return result;
                };
                item_offset_ = index_.data_[item_offset_].next_offset;
            };
            return CONTAINER_END;
        };
        
        inline container_offset_t next() {
            list_index_item_t* const index_data = index_.data_;
            while (item_offset_ != CONTAINER_END) {
                const container_offset_t next_item_offset = index_data[item_offset_].next_offset;
                if (next_item_offset != CONTAINER_END) {
                    const container_offset_t result = index_data[next_item_offset].container_offset;
                    if ((filter_.*IS_INCLUDED)(result)) {
                        item_offset_ = next_item_offset;
                        return result;
                    } else {
                        index_data[item_offset_].next_offset = index_data[next_item_offset].next_offset;
                    };
                } else {
                    index_.instances_last_.data_[instance_offset_] = item_offset_;
                    item_offset_ = CONTAINER_END;
                };
            };
            return CONTAINER_END;
        };
    };
    
    // iterates items for the specified 3 instances
    // merges the lists to avoid duplicates
    // returns container offset for each item or COMTAINER_END at the end
    // skips items where IS_INCLUDE filter method returns false
    template<typename CONTAINER_DATA_T, unsigned INSTANCES_SIZE, class FILTER_T, typename Filterable<FILTER_T>::is_included_t IS_INCLUDED>
    class SimpleLinkedListsIndexMergedFilteredOptimizingInstanceOffsetIterator {
        static_assert(INSTANCES_SIZE > 0 && INSTANCES_SIZE <= 32, "bitwise mask size range for multi-element matches");
    private:
        typedef struct state_t {
            uint32_t instance_bit;
            container_offset_t instance_offset;
            container_offset_t index_offset_prev;
            container_offset_t index_offset;
            state_t* p_next;
        } state_t;
        
    private:
        const SimpleLinkedListsIndex<CONTAINER_DATA_T>& index_;
        state_t state_[INSTANCES_SIZE];
        state_t* p_current_state_; // state with minimal index_offset
        container_offset_t data_offset_; // last accessed value; index cannot change
        // match identificator for the last returned data offset (not data_offset_)
        // bits correspond to indexes of the matching instances
        // multiple can be set at the same time
        uint32_t instance_bits_;
        
        const FILTER_T& filter_;
        
    public:
        SimpleLinkedListsIndexMergedFilteredOptimizingInstanceOffsetIterator(const SimpleLinkedListsIndex<CONTAINER_DATA_T>& index, const FILTER_T& filter): index_(index), filter_(filter) {};
        
        const uint32_t& instance_bits = instance_bits_;
        
        inline container_offset_t first(const container_offset_t instance_offset[INSTANCES_SIZE]) {
            p_current_state_ = nullptr;
            
            for (unsigned i = 0; i < INSTANCES_SIZE; i++) {
                container_offset_t index_offset;
                if (instance_offset[i] < this->index_.instances_.size_) {
                    index_offset = index_.instances_.data_[instance_offset[i]];
                } else {
                    index_offset = CONTAINER_END;
                };
                
                // build list ordered by index_offset; without empty instance lists
                if (index_offset != CONTAINER_END) {
                    state_t* p_state = state_ + i;
                    p_state->instance_bit = 0x1 << i;
                    p_state->instance_offset = instance_offset[i];
                    p_state->index_offset_prev = CONTAINER_END;
                    p_state->index_offset = index_offset;
                
                    if (p_current_state_ == nullptr || p_current_state_->index_offset > p_state->index_offset) {
                        p_state->p_next = p_current_state_;
                        p_current_state_ = p_state;
                    } else {
                        state_t* p_sorted = p_current_state_; // index_offset lower than p_state
                        while (p_sorted->p_next != nullptr && p_sorted->p_next->index_offset < p_state->index_offset) {
                            p_sorted = p_sorted->p_next;
                        };
                        p_state->p_next = p_sorted->p_next;
                        p_sorted->p_next = p_state;
                    };
                };
            };
            
            if (p_current_state_ != nullptr) {
                data_offset_ = index_.data_[p_current_state_->index_offset].container_offset;
                return next();
            } else {
                return CONTAINER_END;
            };
        };
        
        inline container_offset_t next() {
            list_index_item_t* const index_data = index_.data_;
            state_t* p_state = p_current_state_;
            
            // if data_offset_ is included, iterate and return
            // otherwise, exclude it and repeat
            while (p_state != nullptr) {
                _assert_level_4(data_offset_ != CONTAINER_END);
                if ((filter_.*IS_INCLUDED)(data_offset_)) {
                    instance_bits_ = p_state->instance_bit;
                    while (true) {
                        // 1. move p_current_state_ index instance to the next item;
                        //    p_current_state_ corresponds to the cached data_offset_
                        // 2. choose new minimal p_current_state_ instance
                        p_state->index_offset_prev = p_state->index_offset;
                        p_state->index_offset = index_data[p_state->index_offset].next_offset;
                        if (p_state->index_offset != CONTAINER_END) {
                            if (p_state->p_next != nullptr) {
                                if (p_state->p_next->index_offset > p_state->index_offset) {
                                    // the index item of the next instance is higher than
                                    // the next item of the p_state instance
                                    // data offset must be different as well respectively
                                    // terminate and return data_offset_
                                    const container_offset_t data_offset = data_offset_;
                                    data_offset_ = index_data[p_state->index_offset].container_offset;
                                    _assert_level_4(data_offset != data_offset_);
                                    return data_offset;
                                } else {
                                    // the index item of the next instance is lower than
                                    // the next item of the p_state instance (equal not possible)
                                    // update the p_state list and repeat for the new p_current_state_
                                    // if the data offset is the same; otherwise break
                                    _assert_level_4(p_state->p_next->index_offset != p_state->index_offset);
                                    state_t* p_sorted = p_current_state_ = p_state->p_next;
                                    while (p_sorted->p_next != nullptr && p_sorted->p_next->index_offset < p_state->index_offset) {
                                        p_sorted = p_sorted->p_next;
                                    };
                                    p_state->p_next = p_sorted->p_next;
                                    p_sorted->p_next = p_state;
                                    p_state = p_current_state_;
                                }
                            }
                        } else {
                            p_current_state_ = p_state = p_state->p_next;
                        };
                        if (p_state != nullptr) {
                            // next instance; if data offset is different, terminate
                            const container_offset_t data_offset = data_offset_;
                            data_offset_ = index_.data_[p_state->index_offset].container_offset;
                            if (data_offset != data_offset_) {
                                return data_offset;
                            } else {
                                instance_bits_ |= p_state->instance_bit;
                            };
                        } else {
                            return data_offset_; // iterator's end
                        };
                    }
                } else {
                    // exclude (possibly several) index-offset(s)
                    // which have the same excluded data_offset
                    // start with p_state
                    // then repeat with new data_offset_ if available
                    // can break but cannot return within the cycle
                    while (true) {
                        // 1. exclude p_state->index_offset; index can only grow
                        // 2. update the sorted state list if necessary
                        p_state->index_offset = index_data[p_state->index_offset].next_offset;
                        if (p_state->index_offset_prev == CONTAINER_END) {
                            index_.instances_.data_[p_state->instance_offset] = p_state->index_offset;
                        } else {
                            index_data[p_state->index_offset_prev].next_offset = p_state->index_offset;
                        };
                        // if CONTAINER_END, exclude p_state and update instances_last_
                        // otherwise, reposition p_state within the sorted list
                        // break if the new data_offset is different
                        if (p_state->index_offset != CONTAINER_END) {
                            // last index element is the same or greater than the first one
                            _assert_level_4(index_.instances_.data_[p_state->instance_offset] <= index_.instances_last_.data_[p_state->instance_offset]);
                            if (p_state->p_next != nullptr) {
                                if (p_state->p_next->index_offset < p_state->index_offset) {
                                    state_t* p_sorted = p_current_state_ = p_state->p_next;
                                    while (p_sorted->p_next != nullptr && p_sorted->p_next->index_offset < p_state->index_offset) {
                                        p_sorted = p_sorted->p_next;
                                    };
                                    p_state->p_next = p_sorted->p_next;
                                    p_sorted->p_next = p_state;
                                    p_state = p_current_state_;
                                }
                            }
                        } else {
                            // if the last element becomes lower than p_state->index_offset_prev
                            // this means the lst element has been updated concurrently
                            // keep the lower value; normally, the last element can only grow
                            if (p_state->index_offset_prev == CONTAINER_END ||
                                index_.instances_last_.data_[p_state->instance_offset] > p_state->index_offset_prev) {
                                index_.instances_last_.data_[p_state->instance_offset] = p_state->index_offset_prev;
                            };
                            // last index element is the same or greater than the first one
                            _assert_level_4(index_.instances_.data_[p_state->instance_offset] <= index_.instances_last_.data_[p_state->instance_offset]);
                            p_current_state_ = p_state = p_state->p_next;
                        };
                        
                        if (p_state != nullptr) {
                            // next instance; if data offset is different, break
                            // otherwise continue excluding
                            const container_offset_t data_offset = data_offset_;
                            data_offset_ = index_.data_[p_state->index_offset].container_offset;
                            if (data_offset != data_offset_) {
                                break;
                            };
                        } else {
                            break; // iterator's end
                        };
                    };
                };
            };
            return CONTAINER_END;
        };
    };
}

#endif /* linkedlistindex_hpp */
