#include "Srl/Srl.h"
#include "Srl/Lib.h"

using namespace std;
using namespace Srl;
using namespace Lib;

struct Base64Table {

    const uint8_t val_invalid = 255;
    const uint8_t val_fill    = 254;

    const char encode[64] {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };

    uint8_t decode[256];

    Base64Table()
    {
        for(auto i = 0U; i < 256U; i++) {
            decode[i] = val_invalid;
        }
        decode[(size_t)'='] = val_fill;
        for(auto i = 0U; i < 64U; i++) {
            decode[(size_t)encode[i]] = i;
        }
    }
};

vector<uint8_t> Tools::bytes_to_base64(const uint8_t* bytes, size_t nbytes)
{
    vector<uint8_t> basev(4 * ((nbytes + 2) / 3));
    Tools::bytes_to_base64(bytes, nbytes, basev);

    return basev;
}

size_t Tools::bytes_to_base64(const uint8_t* bytes, size_t nbytes, vector<uint8_t>& out)
{
    auto min_size = 4 * ((nbytes + 2) / 3);

    if(out.size() < min_size) {
        out.resize(min_size);
    }

    Base64Table table;
    uint8_t b0 = 0, b1 = 0, b2 = 0;

    for(size_t in_idx = 0U, out_idx = 0U; in_idx < nbytes;
               in_idx += 3, out_idx += 4) {

        b0 = bytes[in_idx];
        b1 = in_idx + 1 < nbytes ? bytes[in_idx + 1] : 0;
        b2 = in_idx + 2 < nbytes ? bytes[in_idx + 2] : 0;

        /* [BBBBBB]BB BBBBBBBB BBBBBBBB */
        out[out_idx]     = table.encode[b0 >> 2];
        /* PPPPPP [BB BBBB]BBBB BBBBBBBB */
        out[out_idx + 1] = table.encode[((b0 << 4) | (b1 >> 4)) & 0x3F];
        /* PPPPPP PPPPPP [BBBB BB]BBBBBB */
        out[out_idx + 2] = table.encode[((b1 << 2) | (b2 >> 6)) & 0x3F];
        /* PPPPPP PPPPPP PPPPPP [BBBBBB] */
        out[out_idx + 3] = table.encode[b2 & 0x3F];
    }

    auto rem = nbytes % 3;
    /* rem == 1 -> 2 equals signs, rem == 2 -> 1 equals sign */
    if(rem > 0) {
        out[min_size - 1] = '=';
    }
    if(rem == 1) {
        out[min_size - 2] = '=';
    }

    return min_size;
}

size_t Tools::get_base64_decoded_size(const char* str, size_t str_len)
{
    if(str_len < 4) {
        return 0;
    }

    auto rem  = str[str_len - 1] == '=' ? 1 : 0;
         rem += str[str_len - 2] == '=' ? 1 : 0;

    return (str_len / 4) * 3 - rem;
}

vector<uint8_t> Tools::base64_to_bytes(const char* str, size_t str_len)
{
    auto vsize = get_base64_decoded_size(str, str_len);

    vector<uint8_t> bytesv(vsize);
    base64_to_bytes(bytesv.data(), vsize, str, str_len);

    return bytesv;
}

size_t Tools::base64_to_bytes(uint8_t* dest, size_t dest_size, const char* str, size_t str_len)
{
    Base64Table table;

    if(str_len > 0 && str_len % 4 != 0) {
        throw Exception("Unable to convert base64 string to binary data." \
                        " Size of source must be a multiple of 4." );
    }

    for(size_t in_idx = 0, out_idx = 0; out_idx < dest_size;
               out_idx += 3, in_idx += 4) {

        uint8_t b0 = table.decode[ (size_t) str[in_idx] ],
                b1 = table.decode[ (size_t) str[in_idx + 1] ],
                b2 = table.decode[ (size_t) str[in_idx + 2] ],
                b3 = table.decode[ (size_t) str[in_idx + 3] ];

        if(b0 == table.val_invalid || b1 == table.val_invalid ||
           b2 == table.val_invalid || b3 == table.val_invalid) {
            throw Exception("Unable to convert base64 string to binary data. String malformed.");
        }
        /* [BBBBBB BB]BBBB BBBBBB BBBBBB */
        dest[out_idx] = b0 << 2 | b1 >> 4;
        /* PPPPPP PP[BBBB BBBB]BB BBBBBB */
        if(out_idx + 1 < dest_size) {
            dest[out_idx + 1] = b1 << 4 | b2 >> 2;
        }
        /* PPPPPP PPPPPP PPPP[BB BBBBBB] */
        if(out_idx + 2 < dest_size) {
            dest[out_idx + 2] = b2 << 6 | b3;
        }
    }

    return get_base64_decoded_size(str, str_len);
}

vector<uint8_t> Tools::bytes_to_hex(const uint8_t* bytes, size_t nbytes)
{
    const char hex_lk[] {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    vector<uint8_t> hexv(nbytes * 2);

    for(size_t in_idx = 0, out_idx = 0; in_idx < nbytes;
               in_idx++, out_idx += 2) {

        hexv[out_idx]     = hex_lk[(bytes[in_idx] >> 4) & 0x0F];
        hexv[out_idx + 1] = hex_lk[bytes[in_idx] & 0x0F];
    }

    return hexv;
}

vector<uint8_t> Tools::hex_to_bytes(const uint8_t* str, size_t str_len)
{
    vector<uint8_t> bytesv(str_len * 2);
    Tools::hex_to_bytes(bytesv.data(), bytesv.size(), str, str_len);

    return bytesv;
}

void Tools::hex_to_bytes(uint8_t* dest, size_t dest_size, const uint8_t* str, size_t str_len)
{
    if(str_len % 2 != 0) {
        throw Exception("Unable to convert hex string to binary data. Size of source must be even.");
    }

    if(dest_size < str_len / 2) {
        throw Exception("Unable to convert hex string to binary data. Destination buffer too small.");
    }

    const auto conv = [](uint8_t& c) {
        if(c >= '0' && c <= '9') {
            c -= '0';

        } else if(c >= 'A' && c <= 'F') {
            c = (c - 'A') + 10;

        } else if(c >= 'a' && c <= 'f') {
            c = (c - 'a') + 10;

        } else {
            throw Exception("Unable to convert hex string to binary data. String malformed.");
        }
    };

    uint8_t low = 0, high = 0;
    uint8_t packed = 0;

    for (size_t in_idx = 0, out_idx = 0; in_idx < str_len;
                in_idx += 2, out_idx++) {

        high = str[in_idx];
        low  = str[in_idx + 1];

        conv(high);
        conv(low);

        packed = (high << 4U) | low;
        dest[out_idx] = packed;
    }
}
