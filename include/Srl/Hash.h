#ifndef SRL_HASH_H
#define SRL_HASH_H

#include "Common.h"
#include "Heap.h"

#include <math.h>

namespace Srl { namespace Lib {

    inline size_t murmur_hash2 (const uint8_t * bytes, size_t nbytes);

    template<class T> struct Mmh2 {
        size_t operator() (const T& t) const { return murmur_hash2(t.data(), t.size()); }
    };

    template <class Key, class Val, class HashFnc = Mmh2<Key>>
    class HTable {

    public:
        Val* get (const Key& key);
        /* fst -> exists? snd -> entry */
        std::pair<bool, Val*> insert (const Key& key, const Val& val);
        std::pair<bool, Val*> insert_hash (size_t hash, const Val& val);

        ~HTable() { clear<Val>(); }

        HTable(size_t buckets = 128, double load_factor_ = 1.0)
            : load_factor(load_factor_ < 0.1 ? 0.1 : load_factor_),
              /* ensure n buckets is power of 2 */
              cap(pow(2, ceil(log2(buckets)))) { }

        HTable(const HTable& m) = default;

        HTable& operator= (HTable&& m) = default;

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

} }

#endif
