#ifndef FROZENSET_H
#define FROZENSET_H

#include <unordered_map>


template <class T, class Hash = std::hash<T>>
class FrozenSet {
  std::unordered_set<T, Hash> items;

public:
  FrozenSet() {}

  FrozenSet(std::initializer_list<T> values) :
    items{values.begin(), values.end()} {}
  
  template <class RandomAccessIterator>
  FrozenSet(RandomAccessIterator begin, RandomAccessIterator end) :
    items{begin, end} {}

  auto begin() const { return items.begin(); }
  auto end() const { return items.end(); }


  bool operator==(FrozenSet const& rhs) const {
    return items == rhs.items;
  }

  FrozenSet operator+(FrozenSet const& rhs) const {
    auto new_set = FrozenSet(items.begin(), items.end());
    new_set.items.insert(rhs.items.begin(), rhs.items.end());
    return new_set;
  }

  FrozenSet operator-(FrozenSet const& rhs) const {
    auto new_set = FrozenSet(items.begin(), items.end());
    for (const auto& item : rhs.items)
      new_set.items.erase(item);
    return new_set;
  }

  size_t size() const {
    return items.size();
  }
    
};

#endif // FROZENSET_H
