//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef binarytreeindex_hpp
#define binarytreeindex_hpp

#include "containerindex.hpp"

namespace bal {

    // "bti" refers to binary tree index
    // "offset" refers to index offset as opposed to container offset;
    // those two are the same when the index contains the data itself
    
#define _bti_item(p_data, offset) ((p_data) + (offset))
    
#define _bti_item_parent_offset(p_item) (*(p_item))
#define _bti_item_right_offset(p_item) (*((p_item) + 1))
#define _bti_item_left_offset(p_item) (*((p_item) + 2))
#define _bti_item_container_offset_ncdi(p_item) (*((p_item) + 3))
#define _bti_item_container_offset(p_item, offset) (contains_data_itself ? (offset) : _bti_item_container_offset_ncdi(p_item))
    
#define _bti_parent_offset(p_data, offset) _bti_item_parent_offset(_bti_item(p_data, offset))
#define _bti_right_offset(p_data, offset) _bti_item_right_offset(_bti_item(p_data, offset))
#define _bti_left_offset(p_data, offset) _bti_item_left_offset(_bti_item(p_data, offset))
#define _bti_container_offset(p_data, offset) _bti_item_container_offset(_bti_item(p_data, offset), offset)
    
#define _bti_item_container_item(p_item) (contains_data_itself ? ((p_item) + 3) : \
    this->p_container_->data_ + _bti_item_container_offset_ncdi(p_item))


    typedef enum {btipkRoot, btipkLeft, btipkRight, btipkCurrent} binary_tree_insertion_point_kind_t;
    
    struct avl_tree_insertion_point_t: container_index_insertion_point_t {
        binary_tree_insertion_point_kind_t kind;
        container_offset_t offset; // instance offset for btipkRoot, index offset otherwise
    };
    
    // iterates over all list items for particular instance
    // used as the default iterator for AvlTreesIndex
    template<typename, typename, bool>
    class BinaryTreesIndexInstanceOffsetIterator;
    
    template<typename CONTAINER_DATA_T, typename INSERTION_POINT_T, bool contains_data_itself>
    class BinaryTreesIndex: public ContainerIndex<container_offset_t, CONTAINER_DATA_T, BinaryTreesIndexInstanceOffsetIterator<CONTAINER_DATA_T, INSERTION_POINT_T, contains_data_itself>, INSERTION_POINT_T> {
    private:
        using base_t = ContainerIndex<container_offset_t, CONTAINER_DATA_T, BinaryTreesIndexInstanceOffsetIterator<CONTAINER_DATA_T, INSERTION_POINT_T, contains_data_itself>, INSERTION_POINT_T>;
        
        template<typename, typename, bool>
        friend class BinaryTreesIndexInstanceOffsetIterator;
        
    public:
        BinaryTreesIndex(const Container<CONTAINER_DATA_T>* const p_container): base_t(p_container) {};
        
        using instance_iterator_t = BinaryTreesIndexInstanceOffsetIterator<CONTAINER_DATA_T, INSERTION_POINT_T, contains_data_itself>;
    };
    
    template<typename CONTAINER_DATA_T, typename INSERTION_POINT_T, bool contains_data_itself>
    class BinaryTreesIndexInstanceOffsetIterator {
    public:
        using index_t = BinaryTreesIndex<CONTAINER_DATA_T, INSERTION_POINT_T, contains_data_itself>;
        
    protected:
        const index_t& index_;
        container_offset_t instance_offset_ = CONTAINER_END;
        container_offset_t item_offset_ = CONTAINER_END;
        
    public:
        BinaryTreesIndexInstanceOffsetIterator(const index_t& index): index_(index) {};
        
        // positions at the first list item for the given instance
        // returns offset of the corresponding container data element or CONTAINER_END
        inline container_offset_t first(const container_offset_t instance_offset) {
            instance_offset_ = instance_offset;
            item_offset_ = instance_offset >= index_.instances_.size_ ? CONTAINER_END : index_.instances_.data_[instance_offset];
            // start from the deepest leftmost leaf node
            if (item_offset_ != CONTAINER_END) {
                while (_bti_left_offset(index_.data_, item_offset_) != CONTAINER_END) {
                    item_offset_ = _bti_left_offset(index_.data_, item_offset_);
                };
            };
            return item_offset_ == CONTAINER_END ? CONTAINER_END : _bti_container_offset(index_.data_, item_offset_);
        };
        
        // moves to the next list item for the stored instance
        // returns offset of the corresponding container data element or CONTAINER_END
        inline container_offset_t next() {
            if (item_offset_ != CONTAINER_END) {
                if (_bti_right_offset(index_.data_, item_offset_) != CONTAINER_END) {
                    // there is a right element - move one right then the deepest leftmost element
                    item_offset_ = _bti_right_offset(index_.data_, item_offset_);
                    while (item_offset_ != CONTAINER_END && _bti_left_offset(index_.data_, item_offset_) != CONTAINER_END) {
                        item_offset_ = _bti_left_offset(index_.data_, item_offset_);
                    };
                } else {
                    // no right element, move up as long as we come from right
                    // if coming from right and reached the root - its the end
                    while (item_offset_ != CONTAINER_END) {
                        container_offset_t parent_offset = _bti_parent_offset(index_.data_, item_offset_);
                        if (parent_offset != CONTAINER_END && _bti_right_offset(index_.data_, parent_offset) == item_offset_) {
                            item_offset_ = parent_offset;
                        } else {
                            item_offset_ = parent_offset;
                            break;
                        };
                    };
                };
            };
            return item_offset_ == CONTAINER_END ? CONTAINER_END : _bti_container_offset(index_.data_, item_offset_);
        };
    };
    
    typedef struct {
        container_offset_t parent_offset;
        container_offset_t left_offset;
        container_offset_t right_offset;
        container_offset_t container_offset;
    } avl_tree_index_item_t;
    
    // TODO: does not implement rollback while changing matched items within the transaction bound
    // for this implementation, this must be addressed externally e.g. by descendants
    template<typename CONTAINER_DATA_T, bool contains_data_itself,
             typename Container<CONTAINER_DATA_T>::comparator_p comparator,
             typename BinaryTreesIndex<CONTAINER_DATA_T, avl_tree_insertion_point_t, contains_data_itself>::instance_determiner_p instance_determiner>
    class AvlTreesIndex: public BinaryTreesIndex<CONTAINER_DATA_T, avl_tree_insertion_point_t, contains_data_itself> {
    public:
        using base_t = BinaryTreesIndex<CONTAINER_DATA_T, avl_tree_insertion_point_t, contains_data_itself>;
        using insertion_point_t = typename base_t::insertion_point_t;
        
    private:
        // find index offset given container offset value
        // return CONTAINER_END if not found
        // use the fact the list is sorted, i.e. larger offsets contain larger container offset values
        // lightweight search meaning without use of the comparison function
        container_offset_t _find_container_offset(const container_offset_t container_offset) const {
            _assert_level_0(!contains_data_itself);
            if (this->size_ > 0 && container_offset != CONTAINER_END) {
                container_offset_t l = 0;
                container_offset_t h = this->size_ - 4;
                while (h > l) {
                    const container_offset_t m = ((h + l) / 2) & 0xFFFFFFFC;
                    if (_bti_container_offset(this->data_, m) > container_offset) {
                        h = m;
                    } else if (_bti_container_offset(this->data_, m) < container_offset) {
                        l = m;
                    } else {
                        return m;
                    };
                };
            };
            return CONTAINER_END;
        };
        
        inline void _update_parent(const container_offset_t offset, const container_offset_t new_offset) {
            const container_offset_t* const p_item = _bti_item(this->data_, offset);
            const container_offset_t parent_offset = _bti_item_parent_offset(p_item);
            if (parent_offset != CONTAINER_END) {
                container_offset_t* const p_parent = _bti_item(this->data_, parent_offset);
                if (offset == _bti_item_right_offset(p_parent)) {
                    _bti_item_right_offset(p_parent) = new_offset;
                } else {
                    _assert_level_1(offset == _bti_item_left_offset(p_parent));
                    _bti_item_left_offset(p_parent) = new_offset;
                };
            } else {
                const container_offset_t instance_offset = instance_determiner(_bti_item_container_item(p_item));
                _assert_level_1(instance_offset < this->instances_.size_);
                _assert_level_1(this->instances_.data_[instance_offset] == offset);
                this->instances_.data_[instance_offset] = new_offset;
            };
            
            if (new_offset != CONTAINER_END) {
                _bti_parent_offset(this->data_, new_offset) = parent_offset;
            };
        };
        
        inline void _remove(const container_offset_t offset) {
            _assert_level_0(offset < this->size_);
            const container_offset_t* const p_item = _bti_item(this->data_, offset);
            container_offset_t new_offset = CONTAINER_END;
            // check right then left then parent
            if (_bti_item_right_offset(p_item) != CONTAINER_END) {
                new_offset = _bti_item_right_offset(p_item);
                if (_bti_item_left_offset(p_item) != CONTAINER_END) {
                    // append left child as the leftmost leaf node of the right tree
                    container_offset_t leaf_offset = new_offset;
                    while (_bti_left_offset(this->data_, leaf_offset) != CONTAINER_END) {
                        leaf_offset = _bti_left_offset(this->data_, leaf_offset);
                    };
                    _bti_left_offset(this->data_, leaf_offset) = _bti_item_left_offset(p_item);
                    _bti_parent_offset(this->data_, _bti_item_left_offset(p_item)) = leaf_offset;
                };
            } else if (_bti_item_left_offset(p_item) != CONTAINER_END) {
                // right_offset is necessarily CONTAINER_END in this case
                new_offset = _bti_item_left_offset(p_item);
            };
            
            _update_parent(offset, new_offset);
        };
        
        // replace one of the child nodes with self
        template<bool b_merge, bool b_with_left, bool b_become_left = false>
        inline void _merge_or_swap_child(const container_offset_t offset) {
            container_offset_t* const p_item = _bti_item(this->data_, offset);
            container_offset_t child_offset = b_with_left ? _bti_item_left_offset(p_item) : _bti_item_right_offset(p_item);
            _assert_level_1(child_offset != CONTAINER_END);
            container_offset_t* p_child = _bti_item(this->data_, child_offset);
            const container_offset_t other_child_offset = (b_with_left ? _bti_item_right_offset(p_item) : _bti_item_left_offset(p_item));
            
            if (b_merge) {
                _bti_item_left_offset(p_item) = _bti_item_left_offset(p_child);
                if (_bti_item_left_offset(p_item) != CONTAINER_END) {
                    _assert_level_2(_bti_parent_offset(this->data_, _bti_item_left_offset(p_item)) == child_offset);
                    _bti_parent_offset(this->data_, _bti_item_left_offset(p_item)) = offset;
                };
                _bti_item_right_offset(p_item) = _bti_item_right_offset(p_child);
                if (_bti_item_right_offset(p_item) != CONTAINER_END) {
                    _assert_level_2(_bti_parent_offset(this->data_, _bti_item_right_offset(p_item)) == child_offset);
                    _bti_parent_offset(this->data_, _bti_item_right_offset(p_item)) = offset;
                };
                p_child = p_item;
                child_offset = offset;
            } else { // swap
                _assert_level_1((b_become_left || _bti_item_right_offset(p_child) == CONTAINER_END) &&
                                (!b_become_left || _bti_item_left_offset(p_child) == CONTAINER_END));
                (b_become_left ? _bti_item_left_offset(p_child) : _bti_item_right_offset(p_child)) = _bti_item_parent_offset(p_child);
                _update_parent(offset, child_offset);
                _bti_item_parent_offset(p_item) = child_offset;
                _bti_item_left_offset(p_item) = CONTAINER_END;
                _bti_item_right_offset(p_item) = CONTAINER_END;
            };
            
            if (other_child_offset != CONTAINER_END) {
                // the other child becomes the rightmost or leftmost furtherst leaf respectively
                while ((b_with_left ? _bti_item_right_offset(p_child) : _bti_item_left_offset(p_child)) != CONTAINER_END) {
                    child_offset = (b_with_left ? _bti_item_right_offset(p_child) : _bti_item_left_offset(p_child));
                    p_child = this->data_ + child_offset;
                };
                
                (b_with_left ? _bti_item_right_offset(p_child) : _bti_item_left_offset(p_child)) = other_child_offset;
                _bti_parent_offset(this->data_, other_child_offset) = child_offset;
            };
        };
        
    protected:
        // insert the provided index element (offset) as specified by the insertion point
        // only inserts as root or into leaf nodes; no balancing for now
        inline void _update(const container_offset_t offset, const insertion_point_t& insertion_point) {
            _assert_level_1(this->insertion_point_is_valid(insertion_point));
            this->insertion_point_invalidate();
            
            container_offset_t* const p_item = _bti_item(this->data_, offset);
            
            if (insertion_point.kind == btipkCurrent) {
                _assert_level_0(insertion_point.offset != CONTAINER_END);
                _assert_level_0(insertion_point.container_offset != CONTAINER_END);
                const container_offset_t* const p_original_item = _bti_item(this->data_, insertion_point.offset);
                _update_parent(insertion_point.offset, offset);
                _bti_item_right_offset(p_item) = _bti_item_right_offset(p_original_item);
                if (_bti_item_right_offset(p_item) != CONTAINER_END) {
                    _bti_parent_offset(this->data_, _bti_item_right_offset(p_item)) = offset;
                };
                _bti_item_left_offset(p_item) = _bti_item_left_offset(p_original_item);
                if (_bti_item_left_offset(p_item) != CONTAINER_END) {
                    _bti_parent_offset(this->data_, _bti_item_left_offset(p_item)) = offset;
                };
            } else if (insertion_point.kind == btipkRoot) {
                _assert_level_1(insertion_point.offset != CONTAINER_END);
                
                if (insertion_point.offset >= this->instances_.size_) {
                    this->instances_.append(CONTAINER_END, insertion_point.offset - this->instances_.size_ + 1);
                };
                container_offset_t* const p_offset = this->instances_.data_ + insertion_point.offset;
                _assert_level_1(*p_offset == CONTAINER_END);
                *p_offset = offset;
                _bti_item_parent_offset(p_item) = CONTAINER_END;
                _bti_item_right_offset(p_item) = CONTAINER_END;
                _bti_item_left_offset(p_item) = CONTAINER_END;
            } else {
                _assert_level_0(insertion_point.container_offset == CONTAINER_END);
                _assert_level_1(insertion_point.offset < this->size_);
                _bti_item_parent_offset(p_item) = insertion_point.offset;
                container_offset_t* const p_parent = _bti_item(this->data_, insertion_point.offset);
                if (insertion_point.kind == btipkLeft) {
                    _bti_item_right_offset(p_item) = CONTAINER_END;
                    _bti_item_left_offset(p_item) = _bti_item_left_offset(p_parent);
                    if (_bti_item_left_offset(p_item) != CONTAINER_END) {
                        _assert_level_2(_bti_parent_offset(this->data_, _bti_item_left_offset(p_item)) == _bti_item_parent_offset(p_item));
                        _bti_parent_offset(this->data_, _bti_item_left_offset(p_item)) = offset;
                    };
                    _bti_item_left_offset(p_parent) = offset;
                } else if (insertion_point.kind == btipkRight) {
                    _bti_item_left_offset(p_item) = CONTAINER_END;
                    _bti_item_right_offset(p_item) = _bti_item_right_offset(p_parent);
                    if (_bti_item_right_offset(p_item) != CONTAINER_END) {
                        _assert_level_2(_bti_parent_offset(this->data_, _bti_item_right_offset(p_item)) == _bti_item_parent_offset(p_item));
                        _bti_parent_offset(this->data_, _bti_item_right_offset(p_item)) = offset;
                    };
                    _bti_item_right_offset(p_parent) = offset;
                } else {
                    _assert_level_0(false);
                };
            };
        };
        
    public:
        AvlTreesIndex(): base_t(this) { _assert_level_0(contains_data_itself); };
        AvlTreesIndex(const Container<CONTAINER_DATA_T>* const p_container): base_t(p_container) { _assert_level_0(!contains_data_itself); };
        
        // container_offset must always remain the same for an index item
        // to guarantee sequence of offset(s) matching container_offset(s)
        // to avoid duplicates, corresponding index item is replaced
        // for Root/Current insertion points
        // for contains_data_itself, the descendant must take care of the allocation
        inline void append(const container_offset_t container_offset, const insertion_point_t& insertion_point) {
            static_assert(!contains_data_itself, "the descendant must redefine if the data is merged with the index");
            this->reserve(4);
            _bti_container_offset(this->data_, this->size_) = container_offset;
            _update(this->size_, insertion_point);
        };
        
        // updates the index entry for a given container_offset
        // ensuring it is removed from the previous location and
        // is located according to the provided insertion_point
        // repurposes index entry to maintain relationship between offsets
        // if insertion_point.container_offset is specified (root/current)
        //   it identifies the element to replace in the index
        //   alternatively if find performance is an issue
        //   is it possible to insert the new element in front of it
        inline void update(const container_offset_t container_offset, const insertion_point_t& insertion_point) {
            const container_offset_t offset = contains_data_itself ? container_offset : _find_container_offset(container_offset);
            _assert_level_0(offset != CONTAINER_END);
            container_offset_t* const p_item = _bti_item(this->data_, offset);
            
            if (insertion_point.kind == btipkCurrent) {
                if (insertion_point.offset == _bti_item_left_offset(p_item)) {
                    _merge_or_swap_child<true, true>(offset);
                    return;
                };
                if (insertion_point.offset == _bti_item_right_offset(p_item)) {
                    _merge_or_swap_child<true, false>(offset);
                    return;
                };
            } else if (insertion_point.kind == btipkLeft) {
                if (insertion_point.offset == offset) {
                    // inserting a child of the item to be removed - simple replacement, nothing to do
                    _assert_level_1(_bti_item_left_offset(p_item) == CONTAINER_END);
                    return;
                };
                if (insertion_point.offset == _bti_item_left_offset(p_item)) {
                    _merge_or_swap_child<false, true, true>(offset);
                    return;
                };
                if (insertion_point.offset == _bti_item_right_offset(p_item)) {
                    _merge_or_swap_child<false, false, true>(offset);
                    return;
                };
            } else if (insertion_point.kind == btipkRight) {
                if (insertion_point.offset == offset) {
                    // inserting a child of the item to be removed - simple replacement, nothing to do
                    _assert_level_1(_bti_item_right_offset(p_item) == CONTAINER_END);
                    return;
                };
                if (insertion_point.offset == _bti_item_left_offset(p_item)) {
                    _merge_or_swap_child<false, true, false>(offset);
                    return;
                };
                if (insertion_point.offset == _bti_item_right_offset(p_item)) {
                    _merge_or_swap_child<false, false, false>(offset);
                    return;
                };
            };
                
            // check inserting instead of the item to be removed - no changes - should never happen
            _assert_level_2(insertion_point.kind != btipkCurrent || insertion_point.offset != offset);
            _assert_level_2(insertion_point.kind != btipkRoot ||
                            insertion_point.offset >= this->instances_.size_ ||
                            this->instances_.data_[insertion_point.offset] != offset);
            
            // cannot append to the deleted item
            _assert_level_0(insertion_point.kind == btipkRoot || insertion_point.offset != offset);
            _assert_level_0(insertion_point.kind != btipkRoot || insertion_point.offset != this->instances_.data_[insertion_point.offset]);
            
            _remove(offset);
            _update(offset, insertion_point);
        };
        
        /* unused
        // removes given container_offset from the index
        // fails if not found
        // keeps the element in memory but excludes it from the index
        inline void remove(const container_offset_t container_offset) {
            _remove(contains_data_itself ? container_offset : _find_container_offset(container_offset));
        };
        */
        
        // find a match for p_object starting from the supplied index offset
        // returns container offset of the first matching object from the list if found,
        // otherwise returns CONTAINER_END
        inline container_offset_t find(const CONTAINER_DATA_T* const p_object) const {
            const container_offset_t instance_offset = instance_determiner(p_object);
            container_offset_t offset = instance_offset < this->instances_.size_ ? this->instances_.data_[instance_offset] : CONTAINER_END;
            while (offset != CONTAINER_END) {
                const container_offset_t* const p_item = _bti_item(this->data_, offset);
                int result = comparator(p_object, _bti_item_container_item(p_item));
                if (result > 0) {
                    offset = _bti_item_right_offset(p_item);
                } else if (result < 0) {
                    offset = _bti_item_left_offset(p_item);
                } else {
                    return _bti_item_container_offset(p_item, offset);
                };
            };
            return CONTAINER_END;
        };
        
        // find a match for p_object for the specific instance
        // return container offset of the matching object if found
        // if not found, return CONTAINER_END plus p_index_offset, an insertion point
        // insertion point is an index offset reference which can be updated if a new item is inserted
        // Note: p_index_offset will be invalidated when the list memory is reallocated
        inline void find(const CONTAINER_DATA_T* const p_object, insertion_point_t &insertion_point) const {
            this->insertion_point_init(insertion_point);
            insertion_point.container_offset = CONTAINER_END;
            // take the root element from the instance
            const container_offset_t instance_offset = instance_determiner(p_object);
            container_offset_t offset = instance_offset < this->instances_.size_ ?
                                            this->instances_.data_[instance_offset] : CONTAINER_END;
            if (offset != CONTAINER_END) {
                while (true) {
                    const container_offset_t* const p_item = _bti_item(this->data_, offset);
                    int result = comparator(p_object, _bti_item_container_item(p_item));
                    if (result > 0) {
                        if (_bti_item_right_offset(p_item) == CONTAINER_END) {
                            insertion_point.kind = btipkRight;
                            insertion_point.offset = offset;
                            return;
                        };
                        offset = _bti_item_right_offset(p_item);
                    } else if (result < 0) {
                        if (_bti_item_left_offset(p_item) == CONTAINER_END) {
                            insertion_point.kind = btipkLeft;
                            insertion_point.offset = offset;
                            return;
                        };
                        offset = _bti_item_left_offset(p_item);
                    } else {
                        insertion_point.kind = btipkCurrent;
                        insertion_point.offset = offset;
                        insertion_point.container_offset = _bti_item_container_offset(p_item, offset);
                        return;
                    };
                };
            } else {
                insertion_point.kind = btipkRoot;
                insertion_point.offset = instance_offset;
            };
        };
        
        // if the insertion point is a replacement, it is only invalid if its parent points elsewheee
        bool insertion_point_is_valid(const insertion_point_t& insertion_point) const override {
            bool result = base_t::insertion_point_is_valid(insertion_point);
            if (!result && insertion_point.version_stamp != CONTAINER_END && insertion_point.kind == btipkCurrent) {
                _assert_level_0(insertion_point.offset < this->size_);
                const CONTAINER_DATA_T* const p_item = _bti_item(this->data_, insertion_point.offset);
                if (_bti_item_parent_offset(p_item) != CONTAINER_END) {
                    const CONTAINER_DATA_T* const p_parent = _bti_item(this->data_, _bti_item_parent_offset(p_item));
                    result = (_bti_item_left_offset(p_parent) == insertion_point.offset) ||
                                (_bti_item_right_offset(p_parent) == insertion_point.offset);
                } else if _bti_item_parent_offset(p_item) {
                    const container_offset_t instance_offset = instance_determiner(_bti_item_container_item(p_item));
                    _assert_level_0(instance_offset < this->instances_.size_);
                    result = this->instances_.data_[instance_offset] == insertion_point.offset;
                };
            };
            return result;
        };
        
        // initialize insertion point to replace the supplied container offset
        // vaidity of the item is checked by verifying its parent
        // insertion point is only updated if the offset is valid
        inline void insertion_point_from_container_offset(insertion_point_t& insertion_point, const container_offset_t offset) {
            static_assert(contains_data_itself, "not implemented");
            if (insertion_point.version_stamp == CONTAINER_END ||
                insertion_point.kind != btipkCurrent || insertion_point.offset != offset) {
                _assert_level_0(offset < this->size_);
                const CONTAINER_DATA_T* const p_item = _bti_item(this->data_, offset);
                bool result = false;
                if (_bti_item_parent_offset(p_item) != CONTAINER_END) {
                    const CONTAINER_DATA_T* const p_parent = _bti_item(this->data_, _bti_item_parent_offset(p_item));
                    result = (_bti_item_left_offset(p_parent) == offset) ||
                                (_bti_item_right_offset(p_parent) == offset);
                } else if _bti_item_parent_offset(p_item) {
                    const container_offset_t instance_offset = instance_determiner(_bti_item_container_item(p_item));
                    _assert_level_0(instance_offset < this->instances_.size_);
                    result = this->instances_.data_[instance_offset] == offset;
                };
                if (result) {
                    this->insertion_point_init(insertion_point);
                    insertion_point.kind = btipkCurrent;
                    insertion_point.offset = offset;
                    insertion_point.container_offset = _bti_item_container_offset(p_item, offset);
                };
            };
        };
    };
};

#endif /* binarytreeindex_hpp */
