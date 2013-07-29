#ifndef SRL_REGISTRATION_H
#define SRL_REGISTRATION_H

#include "String.h"
#include "Hash.h"
#include "Exception.h"
#include "Resolve.h"

#include <functional>

namespace Srl {

    struct TypeID { virtual const char* name() const = 0; };

    namespace Lib {

        template<> struct Fnv1a<String> {
            inline size_t operator() (const String& str)
            {
                return hash_fnv1a(str.data(), str.size());
            }
        };

        class Registrations {

        public:
            template<class T>
            void add(const String& id)
            {
                auto r = this->table.insert(id, [](){ return Ctor<T>::Create_New(); });

                if(r.first) {
                    throw Exception("Class id " + id.unwrap(false) + " duplicated.");
                }
            }

            template<class T>
            T* create(const String& id)
            {
                auto* fnc = this->table.get(id);

                if(!fnc) {
                    throw Exception("Class id " + id.unwrap(false) + " not registered.");
                }

                return static_cast<T*>((*fnc)());
            }

        private:
            HashTable<String, std::function<void*(void)>, 16> table;
        };

        Registrations* registrations();

        template<class T>
        class TypeRegistration : public TypeID {

        private:
            const char* type_id;

        public:
            TypeRegistration(const char* type_id_) : type_id(type_id_)
            {
                Lib::registrations()->add<T>(type_id);
            }

            const char* name() const override { return this->type_id; }
        };
    }

    template<class T>
    Lib::TypeRegistration<T> register_type(const char* type_id)
    {
        return Lib::TypeRegistration<T>(type_id);
    }
}

#endif
