#ifndef SRL_HASH_H
#define SRL_HASH_H

#include "Common.h"
#include "Heap.h"

#include <math.h>
#include <functional>

namespace Srl { namespace Lib {

    namespace Aux {

        inline size_t hash_fnc(const uint8_t* bytes, size_t nbytes);
    }

    template<class T> struct HashSrl {
        size_t operator() (const T& t) const { return Aux::hash_fnc(t.data(), t.size()); }
    };

    template <class Key, class Val, class HashFnc = HashSrl<Key>>
    class HTable {

    public:
        HTable(size_t buckets = 128, double load_factor_ = 1.0)
            : load_factor(load_factor_ < 0.1 ? 0.1 : load_factor_),
              /* ensure n buckets is power of 2 */
              cap(pow(2, ceil(log2(buckets)))) { }

        ~HTable() { clear<Val>(); }

        HTable(const HTable& m) = default;

        HTable(HTable&& m) { *this = std::forward<HTable>(m); }

        HTable& operator= (HTable&& m);

        Val* get (const Key& key);
        /* fst -> exists? snd -> entry */
        std::pair<bool, Val*> insert (const Key& key, const Val& val);
        std::pair<bool, Val*> insert_hash (size_t hash, const Val& val);

        void foreach_entry(const std::function<void(size_t, Val&)>& fnc);

        size_t num_entries() const { return this->elements; }

    private: 
        struct Entry {
            Entry (size_t hash_, const Val& val_)
                : hash(hash_), val(val_) { }

            size_t hash;
            Entry* next = nullptr;
            Val    val;
        };

        double load_factor;
        size_t cap;

        size_t limit    = 0; 
        size_t elements = 0;

        std::vector<Entry*> table;
        HashFnc hash_fnc;
        Heap<Entry> heap;

        void   redistribute();
        size_t get_bucket(size_t hash);

        template<class T>
        typename std::enable_if<std::is_trivially_destructible<T>::value, void>::type
        clear() { }

        template<class T>
        typename std::enable_if<!std::is_trivially_destructible<T>::value, void>::type
        clear();
    };

    template<class K, class V, class H>
    HTable<K, V, H>& HTable<K, V, H>::operator= (HTable<K, V, H>&& m)
    {
        this->clear<V>();

        this->limit    = m.limit;
        this->elements = m.elements;

        m.limit    = 0;
        m.elements = 0;

        this->cap         = m.cap;
        this->load_factor = m.load_factor;

        this->heap  = std::move(m.heap);
        this->table = std::move(m.table);

        return *this;
    }

    namespace Aux {

        template<size_t word_length> struct Arch;

        template<> struct Arch<8> {
            static const uint64_t Prime = 0x100000001B3;
            static const uint64_t Base  = 0xCBF29CE484222325;
        };

        template<> struct Arch<4> {
            static const uint32_t Prime = 0x01000193;
            static const uint32_t Base  = 0x811C9DC5;
        };

        inline size_t fnv1a(const uint8_t* bytes, size_t nbytes)
        {
            const auto apply = [](uint8_t byte, size_t hash) {
                return (hash ^ byte) * Arch<sizeof(size_t)>::Prime;
            };

            size_t h = Aux::Arch<sizeof(size_t)>::Base;

            for(; nbytes >= 4; nbytes -= 4, bytes += 4) {
                h = apply(bytes[0], h);
                h = apply(bytes[1], h);
                h = apply(bytes[2], h);
                h = apply(bytes[3], h);
            }

            switch(nbytes % 4) {
                case 3 : h = apply(bytes[2], h);
                case 2 : h = apply(bytes[1], h);
                case 1 : h = apply(bytes[0], h);
            }

            return h;
        }

        inline size_t hash_fnc(const uint8_t* bytes, size_t nbytes)
        {
            return fnv1a(bytes, nbytes);
        }
    }
} }

#endif
