#ifndef SRL_HASH_H
#define SRL_HASH_H

#include "Common.h"
#include "Heap.h"

#include<functional>

namespace Srl { namespace Lib {

    /* Fowler–Noll–Vo hash function, suggested parameters from http://isthe.com/chongo/tech/comp/fnv/ */

    template<size_t machine_word_size> struct HashParamsFnv1;

    template<> struct HashParamsFnv1<8> {
        static const uint64_t Prime = 0x100000001B3;
        static const uint64_t Base  = 0xCBF29CE484222325;
    };

    template<> struct HashParamsFnv1<4> {
        static const uint32_t Prime = 0x01000193;
        static const uint32_t Base  = 0x811C9DC5;
    };

    template<size_t N> size_t
    hash_fnv1a (const uint8_t* bytes, size_t hash_base = HashParamsFnv1<sizeof(size_t)>::Base);

    template<size_t N> constexpr size_t
    hash_fnv1a (const char(&str)[N], size_t hash_base = HashParamsFnv1<sizeof(size_t)>::Base);

    inline size_t
    hash_fnv1a (const uint8_t* bytes, size_t nbytes, size_t hash_base = HashParamsFnv1<sizeof(size_t)>::Base);

    template<class T> struct HashFnv1a { };

    template <class Key, class Val, size_t NBuckets = 32>
    class HashTable {

    public:
        Val* get (const Key& key);
        /* fst -> exists? snd -> entry */
        std::pair<bool, Val*> insert (const Key& key, const Val& val);
        std::pair<bool, Val*> insert_hash (size_t hash, const Val& val);

        ~HashTable() { clear<Val>(); }

        HashTable() { }

        HashTable(const HashTable& m) = default;

        HashTable& operator= (HashTable&& m) = default;

    private: 
        struct Entry {
            Entry (size_t hash_, const Val& val_)
                : hash(hash_), val(val_) { }

            size_t hash;
            Val    val;
            Entry* le = nullptr;
            Entry* gr = nullptr;
        };

        HashFnv1a<Key> hash_fnc;

        Entry* table[NBuckets] { };
        Heap<Entry> heap;

        template<class T>
        typename std::enable_if<std::is_trivially_destructible<T>::value, void>::type
        clear() { }

        template<class T>
        typename std::enable_if<!std::is_trivially_destructible<T>::value, void>::type
        clear();
    };


} }

#include "Hash.hpp"

#endif
