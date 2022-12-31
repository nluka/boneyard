#ifndef CPPLIB_RLE_TESTS_HPP
#define CPPLIB_RLE_TESTS_HPP

#include "config.hpp"
#include <string>

#if TEST_RLE

#include "../includes/lengthof.hpp"
#include "../includes/test.hpp"
#include "../includes/RLE.hpp"
#include "../includes/sequence-gen.hpp"

void RLE_tests() {
  {
    SETUP_SUITE("RLE");

    auto const testCase = [&s](
      char const *const name_,
      uint8_t const *inputUnencoded,
      size_t const inputUnencodedSize,
      uint8_t const *inputEncoded,
      size_t const inputEncodedSize
    ){
      std::string const name(name_);

      {
        auto const encodedResult = RLE::encode(inputUnencoded, inputUnencodedSize);
        s.assert(
          (name + " encode").c_str(),
          encodedResult.size() == inputEncodedSize &&
          std::memcmp(encodedResult.data(), inputEncoded, inputEncodedSize) == 0
        );
      }

      {
        auto const decodedResult = RLE::decode(inputEncoded, inputEncodedSize);
        s.assert(
          (name + " decode").c_str(),
          decodedResult.size() == inputUnencodedSize &&
          std::memcmp(decodedResult.data(), inputUnencoded, inputUnencodedSize) == 0
        );
      }

      {
        std::ofstream file("RLE.chunks", std::ios::binary);
        RLE::encode_to_file(file, inputUnencoded, inputEncodedSize);
      }
      {
        std::ifstream file("RLE.chunks", std::ios::binary);
        auto const decodedResult = RLE::decode_from_file(file);
        s.assert(
          (name + " encode to file, decode from file").c_str(),
          std::memcmp(decodedResult, inputUnencoded, inputUnencodedSize) == 0
        );
      }
    };

    { // chunks:                  1:ht
      uint8_t const unencoded[] {       1, 2, 3, 4, 5 };
      uint8_t const encoded[]   { 0, 5, 1, 2, 3, 4, 5 };
      testCase(
        "ht5",
        unencoded, lengthof(unencoded),
        encoded, lengthof(encoded)
      );
    }

    { // chunks:                  1:hm
      uint8_t const unencoded[] {    0, 0, 0, 0, 0 };
      uint8_t const encoded[]   { 5, 0             };
      testCase(
        "hm5",
        unencoded, lengthof(unencoded),
        encoded, lengthof(encoded)
      );
    }

    { // chunks:                  1:hm     /**/ 2:hm     /**/ 3:ht
      uint8_t const unencoded[] {    1, 1, /**/    2, 2, /**/       3 };
      uint8_t const encoded[]   { 2, 1,    /**/ 2, 2,    /**/ 0, 1, 3 };
      testCase(
        "hm2|hm2|ht1",
        unencoded, lengthof(unencoded),
        encoded, lengthof(encoded)
      );
    }

    { // chunks:                  1:hm     /**/ 2:ht     /**/ 3:hm
      uint8_t const unencoded[] {    1, 1, /**/       2, /**/    3, 3 };
      uint8_t const encoded[]   { 2, 1,    /**/ 0, 1, 2, /**/ 2, 3    };
      testCase(
        "hm2|ht1|hm2",
        unencoded, lengthof(unencoded),
        encoded, lengthof(encoded)
      );
    }

    { // chunks:                  1:ht     /**/ 2:hm     /**/ 3:ht
      uint8_t const unencoded[] {       1, /**/    2, 2, /**/       3 };
      uint8_t const encoded[]   { 0, 1, 1, /**/ 2, 2,    /**/ 0, 1, 3 };
      // more optimal:            0, 5, 1,         2, 2,            3
      testCase(
        "ht1|hm2|ht1",
        unencoded, lengthof(unencoded),
        encoded, lengthof(encoded)
      );
    }

    { // chunks:                  1:ht     /**/ 2:hm        /**/ 3:ht
      uint8_t const unencoded[] {       1, /**/    2, 2, 2, /**/       3 };
      uint8_t const encoded[]   { 0, 1, 1, /**/ 3, 2,       /**/ 0, 1, 3 };
      // more optimal:            0, 5, 1,         2, 2, 2,            3
      testCase(
        "ht1|hm3|ht1",
        unencoded, lengthof(unencoded),
        encoded, lengthof(encoded)
      );
    }

    { // chunks:                           1:hm
      uint8_t const unencoded[UINT8_MAX] {            0 };
      uint8_t const encoded[]            { UINT8_MAX, 0 };
      testCase(
        "hm255",
        unencoded, lengthof(unencoded),
        encoded, lengthof(encoded)
      );
    }

    { // chunks:                               1:hm          /**/ 2:hm
      uint8_t const unencoded[UINT8_MAX + 1] { 0                       };
      uint8_t const encoded[]                { UINT8_MAX, 0, /**/ 1, 0 };
      testCase(
        "hm255|hm1",
        unencoded, lengthof(unencoded),
        encoded, lengthof(encoded)
      );
    }

    { // chunks:                                 1:hm          /**/ 2:hm
      uint8_t const unencoded[UINT8_MAX + 100] { 0                         };
      uint8_t const encoded[]                  { UINT8_MAX, 0, /**/ 100, 0 };
      testCase(
        "hm255|hm100",
        unencoded, lengthof(unencoded),
        encoded, lengthof(encoded)
      );
    }

    { // chunks:                               1:hm          /**/ 2:hm
      uint8_t const unencoded[2 * UINT8_MAX] { 0                               };
      uint8_t const encoded[]                { UINT8_MAX, 0, /**/ UINT8_MAX, 0 };
      testCase(
        "hm255|hm255",
        unencoded, lengthof(unencoded),
        encoded, lengthof(encoded)
      );
    }

    { // chunks:                                       1:hm          /**/ 2:hm          /**/ 3:hm
      uint8_t const unencoded[(2 * UINT8_MAX) + 100] { 0                                            };
      uint8_t const encoded[]                        { UINT8_MAX, 0, /**/ UINT8_MAX, 0, /**/ 100, 0 };
      testCase(
        "hm255|hm255|hm100",
        unencoded, lengthof(unencoded),
        encoded, lengthof(encoded)
      );
    }

    { // chunks:                                     1:hm          /**/ 2:hm          /**/ 3:hm          /**/ 4:hm
      uint8_t const unencoded[(3 * UINT8_MAX) + 2] { 0                                                             };
      uint8_t const encoded[]                      { UINT8_MAX, 0, /**/ UINT8_MAX, 0, /**/ UINT8_MAX, 0, /**/ 2, 0 };

      testCase(
        "hm255|hm255|hm255|hm2",
        unencoded, lengthof(unencoded),
        encoded, lengthof(encoded)
      );
    }

    {
      uint8_t unencoded[(UINT8_MAX * 2) + 2];
      seqgen::populate(unencoded, "0..254, 0..254, 0,1");

      // 1 byte for count `0`, 1 byte for length, 255 bytes for data
      size_t const maxheteroChunkEncodingLen = 2 + static_cast<size_t>(UINT8_MAX);

      uint8_t encoded[(maxheteroChunkEncodingLen * 2) + (1 + 1 + 2)];
      seqgen::populate(encoded, "0,255,0..254, 0,255,0..254, 0,2,0,1");

      testCase(
        "ht255 ht255 ht2",
        unencoded, lengthof(unencoded),
        encoded, lengthof(encoded)
      );
    }
  }
}

#endif // TEST_RLE

#endif // CPPLIB_RLE_TESTS_HPP
