#pragma once

#include <unordered_map>

// If the key is found, it returns it's value, otherwise it returns the default value given.
template <typename K, typename V>
V UnorderedMapGetOr(const std::unordered_map<K, V>& map, const K& key, const V& defaultValue)
{
    if (auto it = map.find(key); it != map.end())
    {
        return it->second;
    }
    return defaultValue;
}