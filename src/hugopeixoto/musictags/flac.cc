#include "hugopeixoto/musictags/flac.h"
#include "hugopeixoto/musictags/utils.h"
#include <cstdlib>

struct flac_header {
  uint8_t type;
  uint32_t length;

  bool read(FILE* fp) {
    uint8_t buffer[4];

    fread(buffer, 4, 1, fp);

    type = buffer[0];
    length = buffer[1] << 16 | buffer[2] << 8 | buffer[3] << 0;

    return true;
  }

  uint8_t section() const {
    return type & 0x7F;
  }

  bool last() const {
    return type & 0x80;
  }
};

Optional<musictags::Metadata> vorbis_load(FILE* fp) {
  musictags::Metadata result;

  uint32_t length;
  uint32_t comments;

  result.version = "FLAC";

  fread(&length, 4, 1, fp);
  fseek(fp, length, SEEK_CUR);

  fread(&comments, 4, 1, fp);

  for (uint32_t i = 0; i < comments; i++) {
    fread(&length, 4, 1, fp);

    char* bytes = new char[length];

    fread(bytes, length, 1, fp);

    std::string keyvalue(bytes, length);

    if (strstr(keyvalue.c_str(), "ARTIST=") == keyvalue.c_str()) {
      result.artist = keyvalue.substr(6+1);
    } else if (strstr(keyvalue.c_str(), "ALBUM=") == keyvalue.c_str()) {
      result.album = keyvalue.substr(5+1);
    } else if (strstr(keyvalue.c_str(), "TITLE=") == keyvalue.c_str()) {
      result.title = keyvalue.substr(5+1);
    } else if (strstr(keyvalue.c_str(), "TRACKNUMBER=") == keyvalue.c_str()) {
      result.track_no = atoi(keyvalue.substr(5+6+1).c_str());
    }

    delete[] bytes;
  }

  return result;
}


Optional<musictags::Metadata> musictags::flac::load(FILE* fp) {
  char buffer[4];

  if (fseek(fp, 0, SEEK_SET) == 0) {
    if (fread(&buffer, 4, 1, fp) == 1) {
      if (!strncmp(buffer, "fLaC", 4)) {
        flac_header header;

        do {
          header.read(fp);
          if (header.section() == 4) {
            return vorbis_load(fp);
          } else {
            fseek(fp, header.length, SEEK_CUR);
          }
        } while (!header.last());
      }
    }
  }

  return Optional<musictags::Metadata>();
}
