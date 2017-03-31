#include "adaptive_radix_tree_nodes.hpp"
#include "adaptive_radix_tree_index.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "../../../types.hpp"
#include "../../base_column.hpp"
#include "../../untyped_dictionary_column.hpp"
#include "../base_index.hpp"

namespace opossum {

static const uint8_t INVALID_INDEX = 255u;

/**
 *
 * Node4 has two arrays of length 4:
 *  - _partial_keys stores the contained partial_keys of its children
 *  - _children stores pointers to the children
 * _partial_key[i] is the partial_key for child _children[i]
 * default value of the _partial_keys array is 255u
 */

Node4::Node4(std::vector<std::pair<uint8_t, std::shared_ptr<Node>>> &children) {
  std::sort(children.begin(), children.end(),
            [](const std::pair<uint8_t, std::shared_ptr<Node>> &lhs,
               const std::pair<uint8_t, std::shared_ptr<Node>> &rhs) { return lhs.first < rhs.first; });
  _partial_keys.fill(INVALID_INDEX);
  for (uint8_t i = 0u; i < children.size(); ++i) {
    _partial_keys[i] = children[i].first;
    _children[i] = children[i].second;
  }
}

/**
 *
 * searches the child that satisfies the query (lower_bound/ upper_bound + partial_key)
 * calls the appropriate function on the child
 * in case the partial_key is not contained in this node, the query has to be adapted
 *
 *                          04 | 06 | 07 | 08
 *                           |    |    |    |
 *                   |-------|    |    |    |---------|
 *                   |            |    |              |
 *        01| 02 |ff|ff  01|02|03|04  06|07|bb|ff    00|a2|b7|fe
 *         |  |    |      |  |  |  |   |  |  |        |  |  |  |
 *
 * case0:  partial_key (e.g. 06) matches a value in the node
 *           call the query-function on the child at the matching position
 * case1a: partial_key (e.g. 09) is larger than any value in the node which is full
 *           call this->end() which calls end() on the last child
 * case1b: partial_key (e.g. e0 in child at 07) is larger than any value in the node which is not full (last ff does
 *              not have a matching child, it simply is the default value)
 *          call this->end() which calls end() on the last child
 * case2:  partial_key (e.g. 05) is not contained, but smaller than a value in the node
 *           call begin() on the next larger child (e.g. 06)
 **/

BaseIndex::Iterator Node4::_delegate_to_child(
    const AdaptiveRadixTreeIndex::BinaryComparable &key, size_t depth,
    std::function<Iterator(size_t, const AdaptiveRadixTreeIndex::BinaryComparable &, size_t)> function_to_delegate)
    const {
  auto partial_key = key[depth];
  for (uint8_t partial_key_id = 0; partial_key_id < 4; ++partial_key_id) {
    if (_partial_keys[partial_key_id] < partial_key) continue;  // key not found yet
    if (_partial_keys[partial_key_id] == partial_key)
      return function_to_delegate(partial_key_id, key, ++depth);  // case0
    if (_children[partial_key_id] == nullptr) return end();       // no more keys available, case1b
    return _children[partial_key_id]->begin();                    // case2
  }
  return end();  // case1a
}

BaseIndex::Iterator Node4::lower_bound(const AdaptiveRadixTreeIndex::BinaryComparable &key, size_t depth) const {
  return _delegate_to_child(key, depth, [this](size_t i, AdaptiveRadixTreeIndex::BinaryComparable key, size_t depth) {
    return _children[i]->lower_bound(key, depth);
  });
}

BaseIndex::Iterator Node4::upper_bound(const AdaptiveRadixTreeIndex::BinaryComparable &key, size_t depth) const {
  return _delegate_to_child(key, depth, [this](size_t i, AdaptiveRadixTreeIndex::BinaryComparable key, size_t depth) {
    return _children[i]->upper_bound(key, depth);
  });
}

BaseIndex::Iterator Node4::begin() const { return _children[0]->begin(); }

BaseIndex::Iterator Node4::end() const {
  for (uint8_t i = 4; i > 0; --i) {
    if (_children[i - 1] != nullptr) {
      return _children[i - 1]->end();
    }
  }
  throw std::logic_error("Empty _children array in Node4 should never happen");
}

/**
 *
 * Node16 has two arrays of length 16, very similar to Node4:
 *  - _partial_keys stores the contained partial_keys of its children
 *  - _children stores pointers to the children
 * _partial_key[i] is the partial_key for child _children[i]
 * default value of the _partial_keys array is 255u
 *
 */

Node16::Node16(std::vector<std::pair<uint8_t, std::shared_ptr<Node>>> &children) {
  std::sort(children.begin(), children.end(),
            [](const std::pair<uint8_t, std::shared_ptr<Node>> &lhs,
               const std::pair<uint8_t, std::shared_ptr<Node>> &rhs) { return lhs.first < rhs.first; });
  _partial_keys.fill(INVALID_INDEX);
  for (uint8_t i = 0u; i < children.size(); ++i) {
    _partial_keys[i] = children[i].first;
    _children[i] = children[i].second;
  }
}

/**
 * searches the child that satisfies the query (lower_bound/ upper_bound + partial_key)
 * calls the appropriate function on the child
 * in case the partial_key is not contained in this node, the query has to be adapted
 *
 *                          04|..|06 |07|..|e2
 *                           |    |    |    |
 *                   |-------|    |    |    |---------|
 *                   |            |    |              |
 *        01| 02 |ff|ff  01|02|03|04  06|07|bb|ff    00|a2|b7|..|fa|ff|ff
 *                                                    |  |  | ||  |
 *
 * case0:  partial_key (e.g. 06) matches a value in the node
 *           call the query-function on the child at the matching position
 * case1a: partial_key (e.g. fa) is larger than any value in the node which is full
 *           call this->end() which calls end() on the last child
 * case1b: partial_key (e.g. fb in child at e2) is larger than any value in the node which is not full (ffs do
 *              not have matching children in this example, it simply is the default value)
 *          call this->end() which calls end() on the last child
 * case2:  partial_key (e.g. 05) is not contained, but smaller than a value in the node
 *           call begin() on the next larger child (e.g. 06)
 **/

BaseIndex::Iterator Node16::_delegate_to_child(
    const AdaptiveRadixTreeIndex::BinaryComparable &key, size_t depth,
    std::function<Iterator(std::iterator_traits<std::array<uint8_t, 16>::iterator>::difference_type,
                           const AdaptiveRadixTreeIndex::BinaryComparable &, size_t)>
        function) const {
  auto partial_key = key[depth];
  auto partial_key_iterator = std::lower_bound(_partial_keys.begin(), _partial_keys.end(), partial_key);
  auto partial_key_pos = std::distance(_partial_keys.begin(), partial_key_iterator);

  if (*partial_key_iterator == partial_key) {
    return function(partial_key_pos, key, ++depth);  // case0
  }
  if (partial_key_pos >= 16) {
    return end();  // case 1a
  }
  if (_children[partial_key_pos] != nullptr) {
    return _children[partial_key_pos]->begin();  // case2
  }
  return end();  // case1b
}

BaseIndex::Iterator Node16::lower_bound(const AdaptiveRadixTreeIndex::BinaryComparable &key, size_t depth) const {
  return _delegate_to_child(
      key, depth, [this](std::iterator_traits<std::array<uint8_t, 16>::iterator>::difference_type partial_key_pos,
                         AdaptiveRadixTreeIndex::BinaryComparable key,
                         size_t depth) { return _children[partial_key_pos]->lower_bound(key, depth); });
}

BaseIndex::Iterator Node16::upper_bound(const AdaptiveRadixTreeIndex::BinaryComparable &key, size_t depth) const {
  return _delegate_to_child(
      key, depth, [this](std::iterator_traits<std::array<uint8_t, 16>::iterator>::difference_type partial_key_pos,
                         AdaptiveRadixTreeIndex::BinaryComparable key,
                         size_t depth) { return _children[partial_key_pos]->upper_bound(key, depth); });
}

BaseIndex::Iterator Node16::begin() const { return _children[0]->begin(); }

/**
 * _end searches the child with the largest partial key == the last child in the _children array.
 * As the _partial_keys array is filled with 255u per default, we expect the largest child at the index right infront
 * of the first entry with 255u. But 255u can also be a valid partial_key: In this case the _children array contains a
 * pointer at this index as a means to differentiate the two cases.
 */

BaseIndex::Iterator Node16::end() const {
  auto partial_key_iterator = std::lower_bound(_partial_keys.begin(), _partial_keys.end(), INVALID_INDEX);
  auto partial_key_pos = std::distance(_partial_keys.begin(), partial_key_iterator);
  if (_children[partial_key_pos] == nullptr) {
    // there does not exist a child with partial_key 255u, we take the partial_key in front of it
    return _children[partial_key_pos - 1]->end();
  } else {
    // there exists a child with partial_key 255u
    return _children[partial_key_pos]->end();
  }
}

/**
 *
 * Node48 has two arrays:
 *  - _index_to_child of length 256 that can be directly adressed
 *  - _children of length 48 stores pointers to the children
 * _index_to_child[partial_key] stores the index for the child in _childen
 * default value of the _index_to_child array is 255u. This is safe as the maximum value set in _index_to_child will be
 * 47 as this is the maximum index for _children.
 */

Node48::Node48(const std::vector<std::pair<uint8_t, std::shared_ptr<Node>>> &children) {
  _index_to_child.fill(INVALID_INDEX);
  for (uint8_t i = 0u; i < children.size(); ++i) {
    _index_to_child[children[i].first] = i;
    _children[i] = children[i].second;
  }
}

/**
 * searches the child that satisfies the query (lower_bound/ upper_bound + partial_key)
 * calls the appropriate function on the child
 * in case the partial_key is not contained in this node, the query has to be adapted
 *
 * _index_to_child:
 *      00|01|02|03|04|05|06|07|08|09|0a|...| fd |fe|ff|  index
 *      ff|ff|00|ff|ff|01|02|03|ff|04|ff|...|0x30|ff|ff|  value
 *
 * _children
 *      00|01|02|03|04|05|06|07|08|09|0a|...|0x30|
 *       |  |  |  |  |  |  |  |  |  |  | |||  |
 *
 *
 * case0:  partial_key (e.g. 05) matches a value in the node
 *           call the query-function on _children[_index_to_child[partial_key]
 * case1: partial_key (e.g. fe) is larger than any value in the node
 *           call this->end() which calls end() on the last child
 * case2:  partial_key (e.g. 04) is not contained, but smaller than a value in the node
 *           call begin() on the next larger child (e.g. 05)
 *
 * In order to find the next larger/ last child, we have to iterate through the _index_to_child array
 * This is expensive as the array is sparsely populated (at max 48 entries).
 * For the moment, all entries in _children are sorted, as we only bulk_insert records, so we could just iterate through
 * _children instead.
 * But this sorting is not necessarily the case when inserting is allowed (_index_to_child[new_partial_key] would get
 * the largest free index in _children).
 * For future safety, we decided against this more efficient implementation.
 *
 **/

BaseIndex::Iterator Node48::_delegate_to_child(
    const AdaptiveRadixTreeIndex::BinaryComparable &key, size_t depth,
    std::function<Iterator(uint8_t, const AdaptiveRadixTreeIndex::BinaryComparable &, size_t)> function) const {
  auto partial_key = key[depth];
  if (_index_to_child[partial_key] != INVALID_INDEX) {
    // case0
    return function(partial_key, key, ++depth);
  }
  for (uint16_t i = partial_key + 1; i < 256u; ++i) {
    if (_index_to_child[i] != INVALID_INDEX) {
      // case2
      return _children[_index_to_child[i]]->begin();
    }
  }
  // case1
  return end();
}

BaseIndex::Iterator Node48::lower_bound(const AdaptiveRadixTreeIndex::BinaryComparable &key, size_t depth) const {
  return _delegate_to_child(key, depth,
                            [this](uint8_t partial_key, AdaptiveRadixTreeIndex::BinaryComparable key, size_t depth) {
                              return _children[_index_to_child[partial_key]]->lower_bound(key, depth);
                            });
}

BaseIndex::Iterator Node48::upper_bound(const AdaptiveRadixTreeIndex::BinaryComparable &key, size_t depth) const {
  return _delegate_to_child(key, depth,
                            [this](uint8_t partial_key, AdaptiveRadixTreeIndex::BinaryComparable key, size_t depth) {
                              return _children[_index_to_child[partial_key]]->upper_bound(key, depth);
                            });
}

BaseIndex::Iterator Node48::begin() const {
  for (uint8_t i = 0; i < _index_to_child.size(); ++i) {
    if (_index_to_child[i] != INVALID_INDEX) {
      return _children[_index_to_child[i]]->begin();
    }
  }
  throw std::logic_error("Empty _index_to_child array in Node48 should never happen");
}

BaseIndex::Iterator Node48::end() const {
  for (uint8_t i = _index_to_child.size() - 1; i > 0; --i) {
    if (_index_to_child[i] != INVALID_INDEX) {
      return _children[_index_to_child[i]]->begin();
    }
  }
  throw std::logic_error("Empty _index_to_child array in Node48 should never happen");
}

/**
 *
 * Node256 has only one array: _children; which stores pointers to the children and can be directly adressed.
 *
 */

Node256::Node256(const std::vector<std::pair<uint8_t, std::shared_ptr<Node>>> &children) {
  for (uint16_t i = 0; i < children.size(); ++i) {
    _children[children[i].first] = children[i].second;
  }
}

/**
 * searches the child that satisfies the query (lower_bound/ upper_bound + partial_key)
 * calls the appropriate function on the child
 * in case the partial_key is not contained in this node, the query has to be adapted
 *
 *
 * _children
 *      00|01|02|03|04|05|06|07|08|09|0a|...|fd|fe|ff|
 *       |  |  |  |        |     |     | |||  |
 *
 *
 * case0:  _children[partial_key] (e.g. 03) contains a pointer to a child
 *           call the query-function on _children[partial_key]
 * case1: _children[partial_key] (e.g. fe) does contain a nullptr and so does every position afterwards
 *           call this->end() which calls end() on the last child (fd)
 * case2:  _children[partial_key] (e.g. 04)  does contain a nullptr, but there are valid pointers to children afterwards
 *           call begin() on the next larger child (e.g. 06)
 *
 * In order to find the next larger/ last child, we have to iterate through the _children array
 * This is not as expensive as for Node48 as the array has > 48 entries
 *
 **/

BaseIndex::Iterator Node256::_delegate_to_child(
    const AdaptiveRadixTreeIndex::BinaryComparable &key, size_t depth,
    std::function<Iterator(uint8_t, AdaptiveRadixTreeIndex::BinaryComparable, size_t)> function) const {
  auto partial_key = key[depth];
  if (_children[partial_key] != nullptr) {
    // case0
    return function(partial_key, key, ++depth);
  }
  for (uint16_t i = partial_key + 1; i < 256u; ++i) {
    if (_children[i] != nullptr) {
      // case2
      return _children[i]->begin();
    }
  }
  // case1
  return end();
}

BaseIndex::Iterator Node256::upper_bound(const AdaptiveRadixTreeIndex::BinaryComparable &key, size_t depth) const {
  return _delegate_to_child(key, depth,
                            [this](uint8_t partial_key, AdaptiveRadixTreeIndex::BinaryComparable key, size_t depth) {
                              return _children[partial_key]->upper_bound(key, depth);
                            });
}

BaseIndex::Iterator Node256::lower_bound(const AdaptiveRadixTreeIndex::BinaryComparable &key, size_t depth) const {
  return _delegate_to_child(key, depth,
                            [this](uint8_t partial_key, AdaptiveRadixTreeIndex::BinaryComparable key, size_t depth) {
                              return _children[partial_key]->lower_bound(key, depth);
                            });
}

BaseIndex::Iterator Node256::begin() const {
  for (uint8_t i = 0u; i < _children.size(); ++i) {
    if (_children[i] != nullptr) {
      return _children[i]->begin();
    }
  }
  throw std::logic_error("Empty _children array in Node256 should never happen");
}

BaseIndex::Iterator Node256::end() const {
  for (uint8_t i = _children.size() - 1; i >= 0; --i) {
    if (_children[i] != nullptr) {
      return _children[i]->begin();
    }
  }
  throw std::logic_error("Empty _children array in Node256 should never happen");
}

Leaf::Leaf(BaseIndex::Iterator &lower, BaseIndex::Iterator &upper) : _begin(lower), _end(upper) {}

BaseIndex::Iterator Leaf::lower_bound(const AdaptiveRadixTreeIndex::BinaryComparable &, size_t) const { return _begin; }

BaseIndex::Iterator Leaf::upper_bound(const AdaptiveRadixTreeIndex::BinaryComparable &, size_t) const { return _end; }

BaseIndex::Iterator Leaf::begin() const { return _begin; }

BaseIndex::Iterator Leaf::end() const { return _end; }

}  // namespace opossum