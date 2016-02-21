#ifndef SRL_HASH_H
#define SRL_HASH_H

#include "Common.h"
#include "Heap.h"
#include "Aux.h"

#include <cmath>

namespace Srl { namespace Lib {

    namespace Aux {

        uint64_t hash_fnc(const uint8_t* bytes, size_t nbytes);

        inline size_t round_pow2(size_t sz)
        {
            return std::pow(2, std::ceil(std::log2(sz)));
        }
    }

    template<class T> struct HashSrl {
        uint64_t operator() (const T& t) const { return Aux::hash_fnc(t.data(), t.size()); }
    };

    template <class Key, class Val, class HashFnc = HashSrl<Key>>
    class HTable {

    public:
        HTable(size_t buckets = 64, double load_factor_ = 1.0)
            : load_factor(fmax(load_factor_, 0.1)), cap(Aux::round_pow2(buckets)) { }

        ~HTable() { destroy<Val>(); }


        HTable(const HTable& m) = default;

        HTable(HTable&& m) { *this = std::forward<HTable>(m); }

        HTable& operator= (HTable&& m);

        Val* get (const Key& key);
        /* fst -> exists? snd -> entry */
        template<class KV, class... Args>
        std::pair<bool, Val*> insert (KV&& key, Args&&... args);

        void foreach(const std::function<void(const Key&, Val&)>& fnc);

        void foreach_break(const std::function<bool(const Key&, Val&)>& fnc);

        void remove(const Key& key);

        size_t num_entries() const { return this->elements; }

        void clear();

    private:
        struct Entry {

            template<class KV, class... Args> Entry (uint64_t hash_, KV&& key_, Args&&... args)
                : hash(hash_), key(std::forward<KV>(key_)), val(std::forward<Args>(args)...) { }

            uint64_t hash;
            Entry*   next = nullptr;
            Key      key;
            Val      val;
        };

        double load_factor;
        size_t cap;

        size_t limit    = 0;
        size_t elements = 0;

        Entry** table   = nullptr;
        HashFnc hash_fnc;
        Heap    heap;

        void   redistribute();
        size_t get_bucket(uint64_t hash);

        Entry** alloc_table();

        Entry* get_rm (const Key& k, uint64_t hash);

        template<class KV, class... Args>
        std::pair<bool, Val*> insert_hash (uint64_t hash, KV&& key, Args&&... args);

        template<class T>
        typename std::enable_if<std::is_trivially_destructible<T>::value, void>::type
        destroy(Val&) { }

        template<class T>
        typename std::enable_if<!std::is_trivially_destructible<T>::value, void>::type
        destroy(Val&);

        template<class T>
        typename std::enable_if<std::is_trivially_destructible<T>::value, void>::type
        destroy() { }

        template<class T>
        typename std::enable_if<!std::is_trivially_destructible<T>::value, void>::type
        destroy();
    };


} }

#endif
