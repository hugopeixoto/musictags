#include "hugopeixoto/musictags.h"
#include "hugopeixoto/musictags/flac.h"
#include "hugopeixoto/musictags/id3/v1.h"
#include "hugopeixoto/musictags/id3/v2.h"

Optional<musictags::Metadata> musictags::load(const std::string& filename) {
  FILE *fp = fopen(filename.c_str(), "rb");

  auto loaders = {
    musictags::flac::load,
    musictags::id3::v2::load,
    musictags::id3::v1::load,
  };

  Optional<musictags::Metadata> result;

  for (const auto loader : loaders) {
    result = loader(fp);

    if (!result.none()) {
      break;
    }
  }

  fclose(fp);
  return result;
}
