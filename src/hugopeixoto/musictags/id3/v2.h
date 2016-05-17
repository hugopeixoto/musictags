#ifndef HUGOPEIXOTO_MUSICTAGS_ID3_V2_H_
#define HUGOPEIXOTO_MUSICTAGS_ID3_V2_H_

#include "hugopeixoto/musictags.h"

namespace musictags {
  namespace id3 {
    namespace v2 {
      Optional<Metadata> load(FILE* fp);
    }
  }
}

#endif
