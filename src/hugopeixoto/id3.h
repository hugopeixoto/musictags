#ifndef HUGOPEIXOTO_ID3_
#define HUGOPEIXOTO_ID3_

#include <string>
#include "hugopeixoto/nullable.h"

namespace id3 {
  struct MusicMetadata {
    std::string version;
    std::string artist;
    std::string album;
    std::string title;
    uint16_t track_no;
    uint16_t year;
  };

  Nullable<MusicMetadata> load(const std::string& filename);
}

#endif
