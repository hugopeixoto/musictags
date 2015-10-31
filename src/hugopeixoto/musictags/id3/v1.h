#ifndef HUGOPEIXOTO_MUSICTAGS_ID3_V1_H_
#define HUGOPEIXOTO_MUSICTAGS_ID3_V1_H_

#include "hugopeixoto/musictags.h"

namespace musictags {
  namespace id3 {
    namespace v1 {
      Nullable<Metadata> load(FILE* fp);
    }
  }
}

#endif
