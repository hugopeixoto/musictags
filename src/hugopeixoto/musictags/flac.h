#ifndef HUGOPEIXOTO_MUSICTAGS_FLAC_H_
#define HUGOPEIXOTO_MUSICTAGS_FLAC_H_

#include <string>
#include "hugopeixoto/musictags.h"

namespace musictags {
  namespace flac {
    ::Nullable<Metadata> load(FILE* fp);
  }
}

#endif

