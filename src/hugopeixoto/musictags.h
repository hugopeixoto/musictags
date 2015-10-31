#ifndef HUGOPEIXOTO_MUSICTAGS_H_
#define HUGOPEIXOTO_MUSICTAGS_H_

#include <string>
#include "hugopeixoto/nullable.h"

namespace musictags {
  struct Metadata {
    std::string version;
    std::string artist;
    std::string album;
    std::string title;
    uint16_t track_no;
    uint16_t year;
  };

  ::Nullable<Metadata> load(const std::string& filename);
}

#endif
