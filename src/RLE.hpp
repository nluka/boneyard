/*
  This run-length encoding implementation uses 2 kinds of "chunks":

  - homogeneous chunks represent contiguous sequences of bytes which are
    all the same.

      homogeneous chunks are encoded as:
        1. "count" byte (1-255)
        2. "value" byte (0-255)

  - heterogeneous chunks represent contiguous sequences of bytes where no two
    adjacent bytes have the same value. Since homogenenous chunks cannot have a
    "count" byte with value 0, we can use this to indicate a different kind of
    chunk - a hetergeneous one.

      heterogeneous chunks are encoded as:
        1. "count" byte (0)
        2. "length" byte (1-255)
        3. "value" byte (0-255)
*/

#ifndef CPPLIB_RLE_HPP
#define CPPLIB_RLE_HPP

#include <fstream>
#include <vector>

// Run-length encoding module.
namespace RLE {

std::vector<uint8_t> encode(uint8_t const *data, size_t size);

uint8_t *decode(uint8_t const *encoded, size_t size);

void encode_to_file(
  std::ofstream &file,
  uint8_t const *data,
  size_t size
);

uint8_t *decode_from_file(std::ifstream &file);

size_t get_blob_size(uint8_t const *data);

} // namespace RLE

#endif // CPPLIB_RLE_HPP
