#include "hugopeixoto/musictags/id3/v2.h"
#include "hugopeixoto/musictags/utils.h"
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

template<typename Header>
Nullable<musictags::Metadata> load_v2x(const id3_v2 &tags, FILE *fp) {
  musictags::Metadata result;
  Header header;

  uint32_t tags_size = synchsafe_int(tags.size);
  uint32_t total_read = 0;
  while (total_read + header.length() < tags_size) {
    if (!header.read(fp)) {
      break;
    }

    if (header.is(Header::Artist)) {
      result.artist = read_string(fp, header.payload_length).get();
    } else if (header.is(Header::Album)) {
      result.album = read_string(fp, header.payload_length).get();
    } else if (header.is(Header::Title)) {
      result.title = read_string(fp, header.payload_length).get();
    } else if (header.is(Header::Track)) {
      result.track_no = atoi(read_string(fp, header.payload_length).get().c_str());
    } else {
      fseek(fp, header.payload_length, SEEK_CUR);
    }

    total_read += header.length() + header.payload_length;
  }

  return result;
}

struct v22_frame_header {
  uint8_t identifier[3];
  uint32_t payload_length;

  uint32_t length() const { return 6; }

  bool read(FILE* fp) {
    uint8_t buffer[6] = { 0 };
    fread(&buffer, length(), 1, fp);

    identifier[0] = buffer[0];
    identifier[1] = buffer[1];
    identifier[2] = buffer[2];

    payload_length = (buffer[3] << 16 | buffer[4] << 8 | buffer[5] << 0);

    return identifier[0] != 0 || identifier[1] != 0 || identifier[2] != 0 || payload_length != 0;
  }

  bool is(const char* tag) const {
    return memcmp(identifier, tag, 3) == 0;
  }

  constexpr static const char* const Artist = "TP1";
  constexpr static const char* const Album = "TAL";
  constexpr static const char* const Title = "TT2";
  constexpr static const char* const Track = "TRK";
};

Nullable<musictags::Metadata> load_v22(const id3_v2 &tags, FILE *fp) {
  auto result = load_v2x<v22_frame_header>(tags, fp);

  if (!result.null()) {
    result.get().version = "ID3v2.2";
  }

  return result;
}

struct ID3v2_3ExtendedHeader {
  uint32_t size;
  uint16_t flags;
  uint32_t padding_size;
};

struct v23_frame_header {
  uint8_t identifier[4];
  uint32_t payload_length;
  uint16_t flags;

  uint32_t length() const { return 10; }

  bool read(FILE* fp) {
    uint8_t buffer[10] = { 0 };
    fread(&buffer, length(), 1, fp);

    identifier[0] = buffer[0];
    identifier[1] = buffer[1];
    identifier[2] = buffer[2];
    identifier[3] = buffer[3];

    payload_length = (buffer[4] << 24 | buffer[5] << 16 | buffer[6] << 8 | buffer[7] << 0);

    flags = buffer[8] << 0 | buffer[9] << 8;

    return flags != 0 || payload_length != 0 || identifier[0] != 0 || identifier[1] != 0 || identifier[2] != 0 || identifier[3] != 0;
  }

  bool is(const char* tag) const {
    return memcmp(identifier, tag, 4) == 0;
  }

  constexpr static const char* const Artist = "TPE1";
  constexpr static const char* const Album = "TALB";
  constexpr static const char* const Title = "TIT2";
  constexpr static const char* const Track = "TRCK";
};

Nullable<musictags::Metadata> load_v23(const id3_v2 &tags, FILE *fp) {
  if (tags.flags & 0x40) {
    ID3v2_3ExtendedHeader ex_header;
    fread(&ex_header, sizeof(ex_header), 1, fp);
    if (ex_header.flags & 0x8000)
      fseek(fp, 4, SEEK_CUR); // skip the CRC
  }

  auto result = load_v2x<v23_frame_header>(tags, fp);

  if (!result.null()) {
    result.get().version = "ID3v2.3";
  }

  return result;
}

struct v24_frame_header {
  uint8_t identifier[4];
  uint32_t payload_length;
  uint16_t flags;

  uint32_t length() const { return 10; }

  bool read(FILE* fp) {
    uint8_t buffer[10] = { 0 };
    fread(&buffer, length(), 1, fp);

    identifier[0] = buffer[0];
    identifier[1] = buffer[1];
    identifier[2] = buffer[2];
    identifier[3] = buffer[3];

    payload_length = synchsafe_int(buffer + 4);

    flags = buffer[8] << 0 | buffer[9] << 8;

    return flags != 0 || payload_length != 0 || identifier[0] != 0 || identifier[1] != 0 || identifier[2] != 0 || identifier[3] != 0;
  }

  bool is(const char* tag) const {
    return memcmp(identifier, tag, 4) == 0;
  }

  constexpr static const char* const Artist = "TPE1";
  constexpr static const char* const Album = "TALB";
  constexpr static const char* const Title = "TIT2";
  constexpr static const char* const Track = "TRCK";
};


Nullable<musictags::Metadata> load_v24(const id3_v2 &tags, FILE *fp) {
  if (tags.flags & 0x40) {
    uint32_t extended_header_size;
    fread(&extended_header_size, 4, 1, fp);
    fseek(fp, extended_header_size - 4, SEEK_CUR);
  }

  auto result = load_v2x<v24_frame_header>(tags, fp);

  if (!result.null()) {
    result.get().version = "ID3v2.4";
  }

  return result;
}


Nullable<musictags::Metadata> musictags::id3::v2::load(FILE* fp) {
  id3_v2 tags;

  fseek(fp, 0, SEEK_SET);
  if (fread(&tags, sizeof(tags), 1, fp) == 1) {
    if (valid(tags)) {
      //printf("ID3v2.%u.%u %s\n", tags.major, tags.minor, filename.c_str());

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

  return Nullable<musictags::Metadata>();
}
