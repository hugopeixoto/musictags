//#include <iostream>
//#include <cstring>
//#include <cstdlib>
//#include <cstdio>
//#include <cctype>
//#include <stdint.h>
//#include <iconv.h>

#include "hugopeixoto/id3.h"
#include "hugopeixoto/id3/v1.h"
#include "hugopeixoto/id3/v2.h"

Nullable<id3::MusicMetadata> id3::load(const std::string& filename) {
  auto v2 = id3::v2::load(filename);

  if (!v2.null()) {
    return v2;
  }

  return id3::v1::load(filename);
}
