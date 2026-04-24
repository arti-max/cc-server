#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace WebSocketUtils {

    // --- BASE64 ENCODING ---
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    inline std::string base64_encode(const unsigned char* bytes_to_encode, unsigned int in_len) {
        std::string ret;
        int i = 0, j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        while (in_len--) {
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for(i = 0; (i <4) ; i++)
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i) {
            for(j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

            for (j = 0; (j < i + 1); j++)
                ret += base64_chars[char_array_4[j]];

            while((i++ < 3))
                ret += '=';
        }

        return ret;
    }

    // --- SHA1 HASHING (Macros version for stability) ---
    
    // Cyclic rotate left
    #define SHA1_ROL(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

    // Blk macro
    #define SHA1_BLK(i) (block[i&15] = SHA1_ROL(block[(i+13)&15] ^ block[(i+8)&15] ^ block[(i+2)&15] ^ block[i&15], 1))

    // R0-R4 Operations
    #define SHA1_R0(v,w,x,y,z,i) { z += ((w&(x^y))^y) + block[i] + 0x5a827999 + SHA1_ROL(v, 5); w = SHA1_ROL(w, 30); }
    #define SHA1_R1(v,w,x,y,z,i) { z += ((w&(x^y))^y) + SHA1_BLK(i) + 0x5a827999 + SHA1_ROL(v, 5); w = SHA1_ROL(w, 30); }
    #define SHA1_R2(v,w,x,y,z,i) { z += (w^x^y) + SHA1_BLK(i) + 0x6ed9eba1 + SHA1_ROL(v, 5); w = SHA1_ROL(w, 30); }
    #define SHA1_R3(v,w,x,y,z,i) { z += (((w|x)&y)|(w&x)) + SHA1_BLK(i) + 0x8f1bbcdc + SHA1_ROL(v, 5); w = SHA1_ROL(w, 30); }
    #define SHA1_R4(v,w,x,y,z,i) { z += (w^x^y) + SHA1_BLK(i) + 0xca62c1d6 + SHA1_ROL(v, 5); w = SHA1_ROL(w, 30); }

    inline void sha1_transform(uint32_t state[5], const uint8_t buffer[64]) {
        uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];
        uint32_t block[16];

        // Copy buffer to block (Big Endian)
        for (int i = 0; i < 16; i++) {
            block[i] = (buffer[i*4+0] << 24) | (buffer[i*4+1] << 16) | (buffer[i*4+2] << 8) | (buffer[i*4+3]);
        }

        // 4 rounds of 20 operations
        SHA1_R0(a,b,c,d,e, 0); SHA1_R0(e,a,b,c,d, 1); SHA1_R0(d,e,a,b,c, 2); SHA1_R0(c,d,e,a,b, 3);
        SHA1_R0(b,c,d,e,a, 4); SHA1_R0(a,b,c,d,e, 5); SHA1_R0(e,a,b,c,d, 6); SHA1_R0(d,e,a,b,c, 7);
        SHA1_R0(c,d,e,a,b, 8); SHA1_R0(b,c,d,e,a, 9); SHA1_R0(a,b,c,d,e,10); SHA1_R0(e,a,b,c,d,11);
        SHA1_R0(d,e,a,b,c,12); SHA1_R0(c,d,e,a,b,13); SHA1_R0(b,c,d,e,a,14); SHA1_R0(a,b,c,d,e,15);
        
        SHA1_R1(e,a,b,c,d,16); SHA1_R1(d,e,a,b,c,17); SHA1_R1(c,d,e,a,b,18); SHA1_R1(b,c,d,e,a,19);
        SHA1_R2(a,b,c,d,e,20); SHA1_R2(e,a,b,c,d,21); SHA1_R2(d,e,a,b,c,22); SHA1_R2(c,d,e,a,b,23);
        SHA1_R2(b,c,d,e,a,24); SHA1_R2(a,b,c,d,e,25); SHA1_R2(e,a,b,c,d,26); SHA1_R2(d,e,a,b,c,27);
        SHA1_R2(c,d,e,a,b,28); SHA1_R2(b,c,d,e,a,29); SHA1_R2(a,b,c,d,e,30); SHA1_R2(e,a,b,c,d,31);
        SHA1_R2(d,e,a,b,c,32); SHA1_R2(c,d,e,a,b,33); SHA1_R2(b,c,d,e,a,34); SHA1_R2(a,b,c,d,e,35);
        SHA1_R2(e,a,b,c,d,36); SHA1_R2(d,e,a,b,c,37); SHA1_R2(c,d,e,a,b,38); SHA1_R2(b,c,d,e,a,39);
        
        SHA1_R3(a,b,c,d,e,40); SHA1_R3(e,a,b,c,d,41); SHA1_R3(d,e,a,b,c,42); SHA1_R3(c,d,e,a,b,43);
        SHA1_R3(b,c,d,e,a,44); SHA1_R3(a,b,c,d,e,45); SHA1_R3(e,a,b,c,d,46); SHA1_R3(d,e,a,b,c,47);
        SHA1_R3(c,d,e,a,b,48); SHA1_R3(b,c,d,e,a,49); SHA1_R3(a,b,c,d,e,50); SHA1_R3(e,a,b,c,d,51);
        SHA1_R3(d,e,a,b,c,52); SHA1_R3(c,d,e,a,b,53); SHA1_R3(b,c,d,e,a,54); SHA1_R3(a,b,c,d,e,55);
        SHA1_R3(e,a,b,c,d,56); SHA1_R3(d,e,a,b,c,57); SHA1_R3(c,d,e,a,b,58); SHA1_R3(b,c,d,e,a,59);
        
        SHA1_R4(a,b,c,d,e,60); SHA1_R4(e,a,b,c,d,61); SHA1_R4(d,e,a,b,c,62); SHA1_R4(c,d,e,a,b,63);
        SHA1_R4(b,c,d,e,a,64); SHA1_R4(a,b,c,d,e,65); SHA1_R4(e,a,b,c,d,66); SHA1_R4(d,e,a,b,c,67);
        SHA1_R4(c,d,e,a,b,68); SHA1_R4(b,c,d,e,a,69); SHA1_R4(a,b,c,d,e,70); SHA1_R4(e,a,b,c,d,71);
        SHA1_R4(d,e,a,b,c,72); SHA1_R4(c,d,e,a,b,73); SHA1_R4(b,c,d,e,a,74); SHA1_R4(a,b,c,d,e,75);
        SHA1_R4(e,a,b,c,d,76); SHA1_R4(d,e,a,b,c,77); SHA1_R4(c,d,e,a,b,78); SHA1_R4(b,c,d,e,a,79);

        state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
    }

    // Undefine macros to keep namespace clean
    #undef SHA1_ROL
    #undef SHA1_BLK
    #undef SHA1_R0
    #undef SHA1_R1
    #undef SHA1_R2
    #undef SHA1_R3
    #undef SHA1_R4

    inline std::vector<uint8_t> sha1(const std::string& input) {
        uint32_t state[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
        uint8_t buffer[64];

        std::vector<uint8_t> data(input.begin(), input.end());
        size_t i = 0;

        for (i = 0; i + 64 <= data.size(); i += 64) {
            sha1_transform(state, &data[i]);
        }

        size_t remaining = data.size() - i;
        std::memcpy(buffer, &data[i], remaining);
        buffer[remaining++] = 0x80;

        if (remaining > 56) {
            std::memset(&buffer[remaining], 0, 64 - remaining);
            sha1_transform(state, buffer);
            remaining = 0;
        }
        std::memset(&buffer[remaining], 0, 56 - remaining);
        
        uint64_t bit_len = data.size() * 8;
        // SHA1 length is big-endian
        for (int k = 0; k < 8; k++) {
            buffer[63 - k] = (uint8_t)(bit_len >> (k * 8));
        }
        sha1_transform(state, buffer);

        std::vector<uint8_t> digest(20);
        for (int k = 0; k < 5; k++) {
            digest[k*4+0] = (state[k] >> 24) & 0xff;
            digest[k*4+1] = (state[k] >> 16) & 0xff;
            digest[k*4+2] = (state[k] >> 8) & 0xff;
            digest[k*4+3] = (state[k] >> 0) & 0xff;
        }
        return digest;
    }

    inline std::string generateAcceptKey(const std::string& clientKey) {
        std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        std::string concat = clientKey + magic;
        std::vector<uint8_t> hash = sha1(concat);
        return base64_encode(hash.data(), (unsigned int)hash.size());
    }
}
