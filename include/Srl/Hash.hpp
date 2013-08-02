#ifndef SRL_HASH_HPP
#define SRL_HASH_HPP

#include "Hash.h"
#include "Aux.h"

namespace Srl { namespace Lib {

    namespace Aux {

        template<size_t word_length> struct Arch;

        template<> struct Arch<8> {

            static const uint64_t Mul   = 0xC6A4A7935BD1E995;
            static const uint64_t Base  = 0xCBF29CE484222325;
            static const uint64_t Shr   = 47;

            static size_t hash_rem (const uint8_t* bytes, size_t nbytes, size_t hash)
            {
                switch(nbytes % 8) {
                    case 7 : hash ^= (size_t)bytes[6] << 48;
                    case 6 : hash ^= (size_t)bytes[5] << 40;
                    case 5 : hash ^= (size_t)bytes[4] << 32;
                    case 4 : hash ^= (size_t)bytes[3] << 24;
                    case 3 : hash ^= (size_t)bytes[2] << 16;
                    case 2 : hash ^= (size_t)bytes[1] << 8;
                    case 1 : hash ^= (size_t)bytes[0];
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

            static size_t hash_rem (const uint8_t* bytes, size_t nbytes, size_t hash)
            {
                switch(nbytes % 4) {
                    case 3 : hash ^= (size_t)bytes[2] << 16;
                    case 2 : hash ^= (size_t)bytes[1] << 8;
                    case 1 : hash ^= (size_t)bytes[0];
                             hash *= Mul;
                };

                hash ^= hash >> 13;
                hash *= Mul;
                hash ^= hash >> 15;

                return hash;
            }
        };
    }

    inline size_t murmur_hash2(const uint8_t * bytes, size_t nbytes)
    {
        const size_t Wordlen = sizeof(size_t);

        const size_t mul  = Aux::Arch<Wordlen>::Mul;
        const size_t shr  = Aux::Arch<Wordlen>::Shr;
        const size_t base = Aux::Arch<Wordlen>::Base;

        size_t hash = base ^ (nbytes * mul);

        for(; nbytes >= Wordlen; nbytes -= Wordlen, bytes += Wordlen) {

            auto b = Aux::read<size_t>(bytes);
            b *= mul; 
            b ^= b >> shr; 
            b *= mul; 

            hash ^= b;
            hash *= mul; 
        }

        return Aux::Arch<Wordlen>::hash_rem(bytes, nbytes, hash);
    }     

    template<class K, class V, class H>
    size_t HTable<K, V, H>::get_bucket(size_t hash)
    {
        /* mix in the upper half bits of the hash unless table capacity is bigger 2 ^ (bits in word / 2) - 1 */
        const auto shift = sizeof(size_t) * 4;
        const auto max   = ~(size_t)0 >> shift;

        return (this->cap <= max ? hash ^ hash >> shift : hash) & (this->cap - 1);
    }

    template<class K, class V, class H>  __attribute__ ((noinline))
    void HTable<K, V, H>::redistribute()
    {
        if(this->limit == 0) {
            /* initial allocation */
            this->table.resize(cap);
            this->limit = cap * load_factor;

            return;
        }

        auto old_dim = this->cap;
        this->cap *= 2;
        this->limit = cap * load_factor;

        std::vector<Entry*> ntable { this->cap };

        for(auto i = 0U; i < old_dim; i++) {
            auto* entry = table[i];
            while(entry) {
                auto hash = entry->hash;
                auto bucket = get_bucket(hash);
                auto* slot = ntable[bucket];

                if(slot) while(true) {
                    if(!slot->next) {
                        slot->next = entry;
                        break;
                    }
                    slot = slot->next;

                } else {
                    ntable[bucket] = entry;
                }

                auto* tmp = entry;
                entry = entry->next;
                tmp->next = nullptr;
            }
        }

        this->table = std::move(ntable);
    }

    template<class K, class V, class H>
    V* HTable<K, V, H>::get(const K& key)
    {
        if(srl_unlikely(elements < 1)) return nullptr;

        auto hash    = hash_fnc(key);
        auto bucket  = get_bucket(hash);
        Entry* entry = table[bucket];

        while(entry) {
            if(entry->hash == hash) {
                return &entry->val;

            } else {
                entry = entry->next;
            }
        }

        return nullptr;
    }
        
    template<class K, class V, class H>
    std::pair<bool, V*> HTable<K, V, H>::insert(const K& key, const V& val)
    {
        auto hash = hash_fnc(key);
        return insert_hash(hash, val);
    }

    template<class K, class V, class H>
    std::pair<bool, V*> HTable<K, V, H>::insert_hash(size_t hash, const V& val)
    {
        if(srl_unlikely(this->elements >= this->limit)) this->redistribute();

        auto bucket  = get_bucket(hash);
        Entry* entry = table[bucket];
        Entry* node  = nullptr;

        while(entry) {

            if(entry->hash == hash) return { true, &entry->val };

            node = entry;
            entry = entry->next;
        }

        this->elements++;

        auto& slot = node ? node->next : table[bucket];
        slot = this->heap.create(hash, val);

        return { false, &slot->val };
    }

    template<class K, class V, class H> template<class T>
    typename std::enable_if<!std::is_trivially_destructible<T>::value, void>::type
    HTable<K, V, H>::clear()
    {
        if(this->elements < 1) return;

        for(auto i = 0U; i < this->cap; i++) {
            auto* entry = table[i];
            while(entry) {
                entry->val.~V();
                entry = entry->next;
            }
        }
    }

} }

#endif
