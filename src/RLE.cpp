#include <functional>

#include "../includes/RLE.hpp"

static
void do_encoding(
  uint8_t const *const blob,
  size_t const blobSize,
  std::function<void(uint8_t)> writeByte
) {
  typedef std::pair<size_t, size_t> Chunk;
  std::vector<Chunk> homogeneousChunks{};

  // record all homogeneous chunks
  {
    bool insideHomogeneousChunk = false;
    size_t leftBorder = std::string::npos;
    size_t rightBorder = std::string::npos;

    for (size_t i = 0; i < blobSize - 1; ++i) {
      uint8_t const currByte = blob[i];
      uint8_t const nextByte = blob[i + 1];
      bool const areBytesSame = currByte == nextByte;
      bool const onSecondLastByte = (i == blobSize - 2);

      if (insideHomogeneousChunk) {
        if (onSecondLastByte && areBytesSame) {
          // we are situated on the second last byte,
          // currently inside a homogeneous chunk,
          // and the very last byte is part of the chunk
          rightBorder = i + 1;
          homogeneousChunks.emplace_back(leftBorder, rightBorder);
          insideHomogeneousChunk = false;
        } else if (!areBytesSame) {
          // we are situated on the last byte of a homogeneous chunk
          rightBorder = i;
          homogeneousChunks.emplace_back(leftBorder, rightBorder);
          insideHomogeneousChunk = false;
        }
      } else {
        if (onSecondLastByte && areBytesSame) {
          // we are situated on the second last byte of data,
          // entering a homogenous chunk of length 2
          // -> second last byte == last byte
          leftBorder = i;
          rightBorder = i + 1;
          insideHomogeneousChunk = true;
          homogeneousChunks.emplace_back(leftBorder, rightBorder);
        } else if (areBytesSame) {
          // just entered a homogeneous chunk,
          // we are situated on the first byte of it
          leftBorder = i;
          insideHomogeneousChunk = true;
        }
      }
    }
  }

  // break up any homogenous chunks longer than UINT8_MAX into smaller ones
  for (size_t i = 0; i < homogeneousChunks.size(); ++i) {
    auto &chunk = homogeneousChunks[i];
    size_t const chunkLen = chunk.second - chunk.first + 1;

    if (chunkLen > UINT8_MAX) {
      std::pair<size_t, size_t> const newChunk(
        chunk.first,
        chunk.first + UINT8_MAX - 1
      );

      chunk.first += UINT8_MAX;

      // do this emplacement operation at the end because it invalidates `chunk`
      homogeneousChunks.emplace(homogeneousChunks.begin() + i, newChunk);
      // `chunk` ends up being 1 ahead of `newChunk`
    }
  }

  // the first `sizeof(size_t)` bytes are dedicated to the unencoded blob size,
  // because this makes it easy to reserve the right amount of memory when
  // decoding
  {
    uint8_t const *const blobSizeBytes = reinterpret_cast<uint8_t const *>(
      blobSize
    );
    for (size_t i = 0; i < sizeof(size_t); ++i) {
      writeByte(blobSizeBytes[i]);
    }
  }

  // now we write the encoded data to the destination

  Chunk const *nextHomogChunk = homogeneousChunks.empty()
    ? nullptr
    : &homogeneousChunks[0];

  size_t pos = 0;
  while (pos < blobSize) {
    size_t const distToNextHomogChunk =
      nextHomogChunk == nullptr
        ? std::string::npos
        : nextHomogChunk->first - pos;

    if (distToNextHomogChunk == 0) { // homogeneous chunk
      size_t const count = nextHomogChunk->second - nextHomogChunk->first + 1;
      uint8_t const value = blob[pos];

      writeByte(static_cast<uint8_t>(count));
      writeByte(value);

      pos += count;
      nextHomogChunk += 1;

      bool const noMoreHomogChunksLeft =
        nextHomogChunk == homogeneousChunks.data() + homogeneousChunks.size();

      if (noMoreHomogChunksLeft) {
        nextHomogChunk = nullptr;
      }
    } else { // heterogeneous chunk
      size_t const bytesRemaining = blobSize - pos;
      size_t const length = std::min(
        std::min(
          distToNextHomogChunk, // might be std::string::npos
          bytesRemaining
        ),
        // ensure it's not more than what 1 byte can represent
        static_cast<size_t>(UINT8_MAX)
      );

      writeByte(0);
      writeByte(static_cast<uint8_t>(length));
      for (size_t i = 0; i < length; ++i) {
        writeByte(blob[pos + i]);
      }

      pos += length;
    }
  }
}

static
uint8_t* do_decoding(std::function<uint8_t(size_t&)> readBlobByte) {
  size_t pos = 0;

  size_t const blobSize = [&pos, &readBlobByte](){
    size_t val = 0;
    for (size_t i = 0; i <= sizeof(val); ++i) {
      uint8_t const byte = readBlobByte(pos);
      std::memcpy((&val) + i, &byte, sizeof(uint8_t));
    }
    return val;
  }();

  // our "blob" of decoded data:
  uint8_t *const blob = new uint8_t[blobSize];

  auto const blobAppendByte = [blob](uint8_t const byte){
    static size_t pos = 0;
    blob[pos++] = byte;
  };

  while (pos < blobSize) {
    uint8_t const count = readBlobByte(pos);

    if (count == 0) { // hetero chunk
      uint8_t const len = readBlobByte(pos);
      for (size_t i = 1; i <= len; ++i) {
        uint8_t const byte = readBlobByte(pos);
        blobAppendByte(byte);
      }
    } else { // homog chunk
      uint8_t const data = readBlobByte(pos);
      for (size_t i = 1; i <= count; ++i) {
        blobAppendByte(data);
      }
    }
  }

  return blob;
}

std::vector<uint8_t> RLE::encode(
  uint8_t const *const blob,
  size_t const blobSize
) {
  std::vector<uint8_t> encoding{};

  auto const writeByte = [&encoding](uint8_t const byte){
    encoding.push_back(byte);
  };

  do_encoding(blob, blobSize, writeByte);

  return encoding;
}

uint8_t *RLE::decode(
  uint8_t const *const encoded,
  size_t const encodedSize
) {
  auto const readBlobByte = [blob](size_t &pos){
    return blob[pos++];
  };

  return do_decoding(readBlobByte);
}

void RLE::encode_to_file(
  std::ofstream &file,
  uint8_t const *data,
  size_t const size
) {
  auto const writeByte = [&file](uint8_t const byte){
    file.write(reinterpret_cast<char const *>(byte), sizeof(byte));
  };

  do_encoding(data, size, writeByte);
}

uint8_t *RLE::decode_from_file(std::ifstream& file) {

}

size_t RLE::get_blob_size(uint8_t const *const blob) {
  size_t size{};
  std::memcpy(&size, blob, sizeof(size_t));
  return size;
}
