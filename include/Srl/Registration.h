#ifndef SRL_REGISTRATION_H
#define SRL_REGISTRATION_H

#include "String.h"
#include "Hash.h"
#include "Exception.h"
#include "Resolve.h"

namespace Srl {

    struct TypeID { virtual const char* name() const = 0; };

    namespace Lib {

        class Registrations {

            typedef std::function<std::unique_ptr<void>(void)> Make;

        public:
            template<class T>
            void add(const String& id)
            {
                bool exists = this->insert(id, [] { return Ctor<T>::Create_New(); });

                if(exists) {
                    throw Exception("Class id " + id.unwrap(false) + " duplicated.");
                }
            }

            template<class T>
            std::unique_ptr<T> create(const String& id)
            {
                auto* make = this->find(id);

                if(!make) {
                    throw Exception("Class id " + id.unwrap(false) + " not registered.");
                }

                auto uptr = (*make)();

                return std::unique_ptr<T>(static_cast<T*>(uptr.release())); 
            }

        private:
            HTable<String, Make> table { 16 };

            bool  insert (const String& id, Make&& make);
            Make* find   (const String& id);
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
