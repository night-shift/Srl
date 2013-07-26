#ifndef SRL_HASH_HPP
#define SRL_HASH_HPP

#include "Hash.h"

namespace Srl { namespace Lib {

    namespace Aux {

        template<size_t N> void fnv1a_unfold(const uint8_t* bytes, size_t& hash)
        {
            hash = (hash ^ *bytes) * HashParamsFnv1<sizeof(size_t)>::Prime;
            fnv1a_unfold<N - 1>(bytes + 1, hash);
        }

        template<> inline void fnv1a_unfold<0>(const uint8_t*, size_t&) { }
    }

    template<size_t NBytes>
    size_t hash_fnv1a(const uint8_t* bytes, size_t hash_base)
    {
        Aux::fnv1a_unfold<NBytes>(bytes, hash_base);
        return hash_base;
    }

    template<size_t N>
    constexpr size_t hash_fnv1a (const char(&str)[N], size_t hash_base)
    {
        return hash_fnv1a<N>(str, hash_base);
    }

    inline size_t hash_fnv1a(const uint8_t* bytes, size_t nbytes, size_t hash_base)
    {

        for(auto unroll = nbytes / 8; unroll > 0;
            unroll--, bytes += 8) {

            Aux::fnv1a_unfold<8>(bytes, hash_base);
        }

        auto rem = nbytes % 8;

        switch(rem) {
            case 0 : break;
            case 1 : Aux::fnv1a_unfold<1>(bytes, hash_base); break;
            case 2 : Aux::fnv1a_unfold<2>(bytes, hash_base); break;
            case 3 : Aux::fnv1a_unfold<3>(bytes, hash_base); break;
            case 4 : Aux::fnv1a_unfold<4>(bytes, hash_base); break;
            case 5 : Aux::fnv1a_unfold<5>(bytes, hash_base); break;
            case 6 : Aux::fnv1a_unfold<6>(bytes, hash_base); break;
            case 7 : Aux::fnv1a_unfold<7>(bytes, hash_base); break;
        }

        return hash_base;
    }

    template<class K, class V, size_t N>
    HashTable<K, V, N>::HashTable()
    {
        memset(this->table, 0, N * sizeof(Entry*));
    }

    template<class K, class V, size_t N>
    V* HashTable<K, V, N>::get(const V& val)
    {
        auto hash = hash_fnc(val);
        Entry* entry = table[hash % N];
        V* rslt;

        while(!rslt && entry) {

            if(entry->hash == hash) {
                rslt = &entry->val;
            } else {
                entry = entry->next;
            }
        }

        return rslt;
    }
        
    template<class K, class V, size_t N>
    std::pair<bool, V*> HashTable<K, V, N>::insert(const K& key, const V& val)
    {
        auto hash = hash_fnc(key);
        return insert_hash(hash, val);
    }

    template<class K, class V, size_t N>
    std::pair<bool, V*> HashTable<K, V, N>::insert_hash(size_t hash, const V& val)
    {
        auto bucket = hash % N;

        Entry* entry = table[bucket];
        Entry* last = nullptr;

        while(entry) {

            if(entry->hash == hash) return { true, &entry->val };

            last = entry;
            entry = entry->next;
        }

        entry = this->heap.create(hash, val);

        if(last) {
            last->next = entry;

        } else {
            table[bucket] = entry;
        }

        return { false, &entry->val };
    }

    template<class K, class V, size_t N> 
    template<class T>
    typename std::enable_if<!std::is_trivially_destructible<T>::value, void>::type
    HashTable<K, V, N>::clear()
    {
        for(auto i = 0U; i < N; i++) {
            auto* entry = table[i];
            while(entry) {
                entry->~V();
                entry = entry->next;
            }
        }
    }
} }

#endif
