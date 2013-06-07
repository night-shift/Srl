#ifndef SRL_CAST_HPP
#define SRL_CAST_HPP

#include <cstdint>

namespace Srl { namespace Lib {

    template<class T>
    T Read_Cast(const uint8_t* address)
    {
        static_assert(std::is_integral<T>::value ||
                      std::is_floating_point<T>::value, "Cast to non-numeric type.");

        assert(address != nullptr && "Attempting to read from nullptr");

        #ifndef SRL_UNALIGNED_READ /* default: aligned reads */
            if(reinterpret_cast<size_t>(address) % sizeof(T) == 0) {

                return *reinterpret_cast<const T*>(address);

            } else {
                T t;
                memcpy(reinterpret_cast<uint8_t*>(&t), address, sizeof(T));
                return t;
            }
        #else
            return *reinterpret_cast<const T*>(address);
        #endif
    }

} }

#endif
