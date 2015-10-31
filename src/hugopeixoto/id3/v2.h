#ifndef HUGOPEIXOTO_ID3_V2_H_
#define HUGOPEIXOTO_ID3_V2_H_

#include "hugopeixoto/id3.h"

namespace id3 {
  namespace v2 {
    Nullable<MusicMetadata> load(FILE* fp);
  }
}

#endif
