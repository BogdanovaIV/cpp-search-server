#pragma once
#include<mutex>
template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
        void operator+=(const Value& value) {
            ref_to_value = ref_to_value + value;
        }
    };

    explicit ConcurrentMap(size_t bucket_count) :vector_map(bucket_count), mutex_(bucket_count) {
        count_ = bucket_count;
    }

    Access operator[](const Key& key) {
        uint64_t key_ = static_cast<uint64_t>(key);
        uint64_t dif = key_ / count_;
        uint64_t number = key_ - dif * count_;
        return { std::lock_guard(mutex_[number]) , vector_map[number][key] };
    }

    void erase(const Key& key) {
        uint64_t key_ = static_cast<uint64_t>(key);
        uint64_t dif = key_ / count_;
        uint64_t number = key_ - dif * count_;
        std::lock_guard guard(mutex_[number]);
        vector_map[number].erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        int i = 0;
        for (auto& map_ : vector_map) {
            std::lock_guard guard(mutex_[i]);
            for (auto& pair_ : map_) {
                result[pair_.first] = pair_.second;
            }
            ++i;
        }
        return result;
    }

private:
    std::vector<std::map<Key, Value>> vector_map;
    size_t count_ = 0;
    std::vector<std::mutex> mutex_;
};
