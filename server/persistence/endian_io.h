#ifndef SERVER_PERSISTENCE_ENDIAN_IO_H
#define SERVER_PERSISTENCE_ENDIAN_IO_H

#include <cstdint>
#include <istream>
#include <ostream>

// All multi-byte integers in the binary persistence format are stored
// little-endian, independent of the host architecture. These helpers make that
// contract explicit.
namespace endian_io {

inline void write_u16_le(std::ostream& os, uint16_t v) {
    const char b[2] = {static_cast<char>(v & 0xFF), static_cast<char>((v >> 8) & 0xFF)};
    os.write(b, sizeof(b));
}

inline void write_u32_le(std::ostream& os, uint32_t v) {
    const char b[4] = {static_cast<char>(v & 0xFF), static_cast<char>((v >> 8) & 0xFF),
                       static_cast<char>((v >> 16) & 0xFF), static_cast<char>((v >> 24) & 0xFF)};
    os.write(b, sizeof(b));
}

inline bool read_u16_le(std::istream& is, uint16_t& out) {
    unsigned char b[2];
    is.read(reinterpret_cast<char*>(b), sizeof(b));
    if (!is)
        return false;
    out = static_cast<uint16_t>(static_cast<uint16_t>(b[0]) | static_cast<uint16_t>(b[1]) << 8);
    return true;
}

inline bool read_u32_le(std::istream& is, uint32_t& out) {
    unsigned char b[4];
    is.read(reinterpret_cast<char*>(b), sizeof(b));
    if (!is)
        return false;
    out = static_cast<uint32_t>(b[0]) | (static_cast<uint32_t>(b[1]) << 8) |
          (static_cast<uint32_t>(b[2]) << 16) | (static_cast<uint32_t>(b[3]) << 24);
    return true;
}

}  // namespace endian_io

#endif  // SERVER_PERSISTENCE_ENDIAN_IO_H
