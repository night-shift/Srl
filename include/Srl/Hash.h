#ifndef SRL_HASH_H
#define SRL_HASH_H

#include "Common.h"
#include "Heap.h"
#include "Aux.h"

#include <math.h>

namespace Srl { namespace Lib {

    namespace Aux {

        inline size_t hash_fnc(const uint8_t* bytes, size_t nbytes);

        inline size_t round_pow2(size_t sz)
        {
            return pow(2, ceil(log2(sz)));
        }
    }

    template<class T> struct HashSrl {
        size_t operator() (const T& t) const { return Aux::hash_fnc(t.data(), t.size()); }
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
        std::pair<bool, Val*> insert (const Key& key, const Val& val);
        std::pair<bool, Val*> insert_hash (size_t hash, const Val& val);

        void foreach_entry(const std::function<void(size_t, Val&)>& fnc);

        size_t num_entries() const { return this->elements; }

        void clear();

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

        Entry** table   = nullptr;
        HashFnc hash_fnc;
        Heap    heap;

        void   redistribute();
        size_t get_bucket(size_t hash);

        Entry** alloc_table();

        template<class T>
        typename std::enable_if<std::is_trivially_destructible<T>::value, void>::type
        destroy() { }

        template<class T>
        typename std::enable_if<!std::is_trivially_destructible<T>::value, void>::type
        destroy();
    };

    template<class K, class V, class H>
    HTable<K, V, H>& HTable<K, V, H>::operator= (HTable<K, V, H>&& m)
    {
        this->destroy<V>();

        this->limit    = m.limit;
        this->elements = m.elements;

        m.limit    = 0;
        m.elements = 0;

        this->cap         = m.cap;
        this->load_factor = m.load_factor;

        this->heap  = std::move(m.heap);
        this->table = m.table;

        return *this;
    }

namespace Aux {

    template<size_t word_length> struct Arch;

    template<> struct Arch<8> {

        static const uint64_t Mul  = 0xC6A4A7935BD1E995;
        static const uint64_t Base = 0xCBF29CE484222325;
        static const uint64_t Shr  = 47;

        static size_t hash_rem (const uint8_t* bytes, size_t nbytes, uint64_t hash)
        {
            switch(nbytes % 8) {
                case 7 : hash ^= (uint64_t)bytes[6] << 48;
                case 6 : hash ^= (uint64_t)bytes[5] << 40;
                case 5 : hash ^= (uint64_t)bytes[4] << 32;
                case 4 : hash ^= (uint64_t)bytes[3] << 24;
                case 3 : hash ^= (uint64_t)bytes[2] << 16;
                case 2 : hash ^= (uint64_t)bytes[1] << 8;
                case 1 : hash ^= (uint64_t)bytes[0];
                         hash *= Mul;
            };

            hash ^= hash >> Shr;
            hash *= Mul;
            hash ^= hash >> Shr;

            return hash;
        }
    };

    template<> struct Arch<4> {
        
        static const uint32_t Mul  = 0x5BD1E995;
        static const uint32_t Base = 0x811C9DC5;
        static const uint32_t Shr  = 24;

        static size_t hash_rem (const uint8_t* bytes, size_t nbytes, uint32_t hash)
        {
            switch(nbytes % 4) {
                case 3 : hash ^= (uint32_t)bytes[2] << 16;
                case 2 : hash ^= (uint32_t)bytes[1] << 8;
                case 1 : hash ^= (uint32_t)bytes[0];
                         hash *= Mul;
            };

            hash ^= hash >> 13;
            hash *= Mul;
            hash ^= hash >> 15;

            return hash;
        }
    };

    inline size_t murmur_hash2(const uint8_t * bytes, size_t nbytes)
    {
        const auto wordlen = sizeof(size_t);

        const auto mul  = Aux::Arch<wordlen>::Mul;
        const auto shr  = Aux::Arch<wordlen>::Shr;
        const auto base = Aux::Arch<wordlen>::Base;

        size_t hash = base ^ (nbytes * mul);

        for(; nbytes >= wordlen; nbytes -= wordlen, bytes += wordlen) {

            auto b = read<size_t>(bytes);

            b *= mul; 
            b ^= b >> shr; 
            b *= mul; 

            hash ^= b;
            hash *= mul; 
        }

        return Aux::Arch<wordlen>::hash_rem(bytes, nbytes, hash);
    }     

    inline size_t hash_fnc(const uint8_t* bytes, size_t nbytes)
    {
        return murmur_hash2(bytes, nbytes);
    }
    
} } }

#endif
