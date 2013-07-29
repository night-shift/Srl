#ifndef SRL_HASH_HPP
#define SRL_HASH_HPP

#include "Hash.h"

namespace Srl { namespace Lib {

    namespace Aux {

        template<size_t N> void fnv1a_unfold(const uint8_t* bytes, size_t& hash)
        {
            hash = (hash ^ *bytes) * ParamsFnv1<sizeof(size_t)>::Prime;
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
        return hash_fnv1a<N - 1>((const uint8_t*)str, hash_base);
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

    template<class K, class V, size_t N, class H>
    V* HashTable<K, V, N, H>::get(const K& key)
    {
        auto hash = hash_fnc(key);
        Entry* entry = table[hash % N];

        while(entry) {
            if(entry->hash == hash) {
                return &entry->val;

            } else {
                entry = entry->hash > hash ? entry->le : entry->gr;
            }
        }

        return nullptr;
    }
        
    template<class K, class V, size_t N, class H>
    std::pair<bool, V*> HashTable<K, V, N, H>::insert(const K& key, const V& val)
    {
        auto hash = hash_fnc(key);
        return insert_hash(hash, val);
    }

    template<class K, class V, size_t N, class H>
    std::pair<bool, V*> HashTable<K, V, N, H>::insert_hash(size_t hash, const V& val)
    {
        auto bucket  = hash % N;
        Entry* entry = table[bucket];
        Entry* node  = nullptr;

        while(entry) {

            if(entry->hash == hash) return { true, &entry->val };

            node = entry;
            entry = entry->hash > hash ? entry->le : entry->gr;
        }

        auto& slot = node ? node->hash > hash ? node->le : node->gr
                          : table[bucket];

        slot = this->heap.create(hash, val);

        return { false, &slot->val };
    }

    template<class K, class V, size_t N, class H> template<class T>
    typename std::enable_if<!std::is_trivially_destructible<T>::value, void>::type
    HashTable<K, V, N, H>::clear()
    {
        std::function<void(Entry*)> unfold;

        unfold = [&unfold](Entry* e) {
            e->val.~V();
            if(e->le) unfold(e->le);
            if(e->gr) unfold(e->gr);
        };

        for(auto i = 0U; i < N; i++) {
            if(table[i]) unfold(table[i]);
        }
    }
} }

#endif
