#ifndef __VARINT_H__
#define __VARINT_H__

namespace Components {
    class VarInt {
        public:
        long long value;
        /* Load VarInt from 64-bit value */
        VarInt(long long value) {
            this->value = value;
        }
        /* Load VarInt from data bytes */
        VarInt(const unsigned char *data) {
            this->value = read(data, this->value);
        }
        /* Load VarInt from bytes at offset (reference) i */
        VarInt(const unsigned char *data, size_t &i) {
            i += read(&data[i], this->value);
        }
        size_t read(const unsigned char *data, long long *v) {
            return read(data, *v);
        }
        size_t read(const unsigned char *data, long long &v) {
            short shift = 0;
            size_t value = 0;
            size_t i = 0;

            while (data[i] & 0x80) {
                value |= (data[i++]&0x7F) << shift;
                shift += 7;
            }
            value |= data[i++];
            // Decode zigzag64
            v = (value << 1) ^ (value >> 63);
            return i;
        }
        /* Write this VarInt to a byte data buffer, return bytes written */
        size_t write(unsigned char *out) {
            size_t i = 0;
            return write(out, i);
        }
        /* Write this VarInt to a byte data buffer at offset (reference) i, return bytes written */
        size_t write(unsigned char *out, size_t &i) {
            size_t j = i;
            // Cast s64 to u64
            size_t value = this->value;
            // Encode zigzag64
            value = (value >> 1) ^ -(value & 1);
            // While the unsigned value is outside the 7 bit range,
            // shift out and write the low 7 bits, writing to out with the high bit set.
            while (value >= 0x80) {
                out[i++] = (value & 0x7F) | 0x80;
                value >>= 7;
            }
            out[i++] = value;
            return i - j;
        }
        operator long long() const {
            return value;
        }
    };
}

#endif