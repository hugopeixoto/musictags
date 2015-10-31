#include "hugopeixoto/id3/v2.h"
#include "hugopeixoto/id3/utils.h"
#include <cstdlib>
#include <iconv.h>

struct id3_v2 {
  char magic[3];
  uint8_t major;
  uint8_t minor;
  uint8_t flags;
  uint8_t size[4];
};

static bool valid(const id3_v2 &tags) {
  return (memcmp(tags.magic, "ID3", 3) == 0) &&
         (tags.major < 0xFF && tags.minor < 0xFF) &&
         ((tags.flags & 0x0F) == 0) && ((tags.size[0] & 0x80) == 0) &&
         ((tags.size[1] & 0x80) == 0) && ((tags.size[2] & 0x80) == 0) &&
         ((tags.size[3] & 0x80) == 0);
}

uint32_t synchsafe_int(const uint8_t integer[4]) {
  return (integer[3] << 0) | (integer[2] << 7) | (integer[1] << 14) |
         (integer[0] << 21);
}

std::string readUTF_8(uint32_t size, char *src) {
  return std::string(src, size);
}

std::string readOther(const char* charset, uint32_t size, char *src) {
  iconv_t cd = iconv_open("UTF-8", charset);
  char *to = new char[size * 2];

  char *ibuf = src;
  char *obuf = to;
  size_t isize = size;
  size_t osize = size * 2;
  iconv(cd, &ibuf, &isize, &obuf, &osize);
  *obuf = 0;

  iconv_close(cd);

  std::string result(to);
  delete[] to;
  return result;
}

Nullable<std::string> read_string(FILE *fp, uint32_t size, uint16_t flags = 0) {
  uint8_t encoding;
  char *raw;

  if (flags & 0x0200) {
    // assert that 0x0100
    if (!(flags & 0x100)) {
      return Nullable<std::string>();
    }

    uint8_t buffer[4];
    uint32_t length;

    fread(buffer, 4, 1, fp);
    length = synchsafe_int(buffer);

    size -= 4;

    raw = new char[size];
    fread(raw, size, 1, fp);

    // printf("unsynched: %u -> %u\n", a_size, length);
    char *synched = new char[length];
    for (uint32_t i = 0, j = 0; i < size; ++i) {
      if (((uint8_t *)raw)[i] == 0xFF && i + 2 < size && raw[i + 1] == 0x00) {
        synched[j++] = raw[i]; // 0xFF
        synched[j++] = raw[i + 2];
        i += 2;
      } else {
        synched[j++] = raw[i];
      }
    }

    delete[] raw;
    raw = synched;
    size = length;

  } else {
    raw = new char[size];
    fread(raw, size, 1, fp);
  }

  encoding = raw[0];
  size--;

  switch (encoding) {
  case 0x00:
    return readOther("ISO-8859-1", size, raw + 1);
  case 0x01:
  case 0x02:
    return readOther("UTF-16", size, raw + 1);
  case 0x03:
    return readUTF_8(size, raw + 1);
  default:
    return Nullable<std::string>();
  }
}

Nullable<id3::MusicMetadata> load_v22(const id3_v2 &tags, FILE *fp) {
  id3::MusicMetadata result;

  result.version = "ID3v2.2";

  struct {
    uint8_t identifier[3];
    uint8_t size_[3];

    uint32_t size() const {
      return (size_[0] << 16) | (size_[1] << 8) | (size_[2] << 0);
    }

    bool null() const {
      return size() == 0 && identifier[0] == 0 && identifier[1] == 0 &&
             identifier[2] == 0;
    }

    bool is(const char* tag) const {
      return memcmp(identifier, tag, 3) == 0;
    }
  } FrameHeader;

  uint32_t tags_size = synchsafe_int(tags.size);
  uint32_t total_read = 0;
  while (total_read + 6 < tags_size) {
    fread(&FrameHeader, sizeof(FrameHeader), 1, fp);
    if (FrameHeader.null()) {
      break;
    }

    if (FrameHeader.is("TP1")) {
      result.artist = read_string(fp, FrameHeader.size()).get();
    } else if (FrameHeader.is("TAL")) {
      result.album = read_string(fp, FrameHeader.size()).get();
    } else if (FrameHeader.is("TT2")) {
      result.title = read_string(fp, FrameHeader.size()).get();
    } else if (FrameHeader.is("TRK")) {
      result.track_no = atoi(read_string(fp, FrameHeader.size()).get().c_str());
    } else {
      fseek(fp, FrameHeader.size(), SEEK_CUR);
    }

    total_read += 6 + FrameHeader.size();
  }

  return result;
}

struct ID3v2_3ExtendedHeader {
  uint32_t size;
  uint16_t flags;
  uint32_t padding_size;
};

Nullable<id3::MusicMetadata> load_v23(const id3_v2 &tags, FILE *fp) {
  id3::MusicMetadata result;

  result.version = "ID3v2.3";

  if (tags.flags & 0x40) {
    ID3v2_3ExtendedHeader ex_header;
    fread(&ex_header, sizeof(ex_header), 1, fp);
    if (ex_header.flags & 0x8000)
      fseek(fp, 4, SEEK_CUR); // skip the CRC
  }

  struct {
    uint8_t identifier[4];
    uint8_t size_[4];
    uint16_t flags;

    uint32_t size() const {
      return (size_[0] << 24) | (size_[1] << 16) | (size_[2] << 8) |
             (size_[3] << 0);
    }

    bool is(const char* tag) const {
      return memcmp(identifier, tag, 4) == 0;
    }
  } FrameHeader;

  uint8_t FinalFrame[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  uint32_t tags_size = synchsafe_int(tags.size);
  uint32_t total_read = 0;
  while (total_read + 10 < tags_size) {
    fread(&FrameHeader, sizeof(FrameHeader), 1, fp);
    if (memcmp(&FrameHeader, &FinalFrame, sizeof(FrameHeader)) == 0) {
      break;
    }

    if (FrameHeader.is("TPE1")) {
      result.artist = read_string(fp, FrameHeader.size()).get();
    } else if (FrameHeader.is("TALB")) {
      result.album = read_string(fp, FrameHeader.size()).get();
    } else if (FrameHeader.is("TIT2")) {
      result.title = read_string(fp, FrameHeader.size()).get();
    } else if (FrameHeader.is("TRCK")) {
      result.track_no = atoi(read_string(fp, FrameHeader.size()).get().c_str());
    } else {
      fseek(fp, FrameHeader.size(), SEEK_CUR);
    }

    total_read += 10 + FrameHeader.size();
  }

  return result;
}

Nullable<id3::MusicMetadata> load_v24(const id3_v2 &tags, FILE *fp) {
  id3::MusicMetadata result;

  result.version = "ID3v2.4";

  if (tags.flags & 0x40) {
    uint32_t extended_header_size;
    fread(&extended_header_size, 4, 1, fp);
    fseek(fp, extended_header_size - 4, SEEK_CUR);
  }

  struct {
    uint8_t identifier[4];
    uint8_t size_[4];
    uint16_t flags;
    uint32_t size() const { return synchsafe_int(size_); }
    bool is(const char* tag) const {
      return memcmp(identifier, tag, 4) == 0;
    }
  } FrameHeader;

  uint8_t FinalFrame[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  uint32_t tags_size = synchsafe_int(tags.size);
  uint32_t total_read = 0;
  while (total_read + 10 < tags_size) {
    fread(&FrameHeader, sizeof(FrameHeader), 1, fp);

    if (memcmp(&FrameHeader, &FinalFrame, sizeof(FrameHeader)) == 0) {
      break;
    }

    if (FrameHeader.is("TPE1")) {
      result.artist = read_string(fp, FrameHeader.size(), FrameHeader.flags).get();
    } else if (FrameHeader.is("TALB")) {
      result.album = read_string(fp, FrameHeader.size(), FrameHeader.flags).get();
    } else if (FrameHeader.is("TIT2")) {
      result.title = read_string(fp, FrameHeader.size(), FrameHeader.flags).get();
    } else if (FrameHeader.is("TRCK")) {
      result.track_no = atoi(read_string(fp, FrameHeader.size(), FrameHeader.flags).get().c_str());
    } else {
      fseek(fp, FrameHeader.size(), SEEK_CUR);
    }

    total_read += 10 + FrameHeader.size();
  }

  return result;
}


Nullable<id3::MusicMetadata> id3::v2::load(const std::string &filename) {
  id3_v2 tags;

  FILE *fp = fopen(filename.c_str(), "rb");

  if (fp) {
    if (fread(&tags, sizeof(tags), 1, fp) == 1) {
      if (valid(tags)) {
        //printf("ID3v2.%u.%u %s\n", tags.major, tags.minor, filename.c_str());

        // synchsafe(tags.size));
        switch (tags.major) {
        case 4:
          return load_v24(tags, fp);
        case 3:
          return load_v23(tags, fp);
        case 2:
          return load_v22(tags, fp);
        }
      }
    }
    fclose(fp);
  }

  return Nullable<id3::MusicMetadata>();
}
