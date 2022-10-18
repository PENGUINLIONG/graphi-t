// General purpose object pool.
// @PENGUINLIONG
#pragma once
#include <map>
#include <memory>
#include <vector>
#include <array>

namespace liong {
namespace pool {

template<typename TKey, typename TValue>
struct PoolInner {
    std::map<TKey, std::vector<TValue>> items;
};

template<typename TKey, typename TValue>
struct PoolItemInner {
  PoolInner<TKey, TValue>* pool;
  TKey key;
  TValue value;

  PoolItemInner(PoolInner<TKey, TValue>* pool, TKey&& key, TValue&& value) :
    pool(pool), key(std::move(key)), value(std::move(value)) {}
  ~PoolItemInner() {
    pool->items[std::move(key)].emplace_back(std::move(value));
  }
};

template<typename TKey, typename TValue>
struct PoolItem {
  std::shared_ptr<PoolItemInner<TKey, TValue>> inner;

  PoolItem() = default;
  PoolItem(PoolInner<TKey, TValue>* pool, TKey&& key, TValue&& value) :
    inner(std::make_shared<PoolItemInner<TKey, TValue>>(
      pool, std::move(key), std::move(value))) {}

  inline bool is_valid() const {
    return inner != nullptr;
  }

  inline TValue& value() {
    return inner->value;
  }
  inline const TValue& value() const {
    return inner->value;
  }

  inline void release() {
    inner.reset();
  }
};

template<typename TKey, typename TValue>
struct Pool {
  PoolInner<TKey, TValue> inner;

  inline bool has_free_item(const TKey& key) const {
    auto it = inner.items.find(key);
    if (it != inner.items.end() && !it->second.empty()) {
      return true;
    }
    return false;
  }
  inline PoolItem<TKey, TValue> create(TKey&& key, TValue&& value) {
    return PoolItem<TKey, TValue>(
      &inner,
      std::move(key),
      std::move(value));
  }
  inline PoolItem<TKey, TValue> acquire(TKey&& key) {
    std::vector<TValue>& pool = inner.items.at(key);
    TValue value = std::move(pool.back());
    pool.pop_back();
    return create(std::move(key), std::move(value));
  }
};

} // namespace pool
} // namespace liong
