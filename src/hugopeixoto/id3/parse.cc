#include "hugopeixoto/id3.h"
#include "hugopeixoto/id3/v1.h"
#include "hugopeixoto/id3/v2.h"

Nullable<id3::MusicMetadata> id3::load(const std::string& filename) {
  FILE *fp = fopen(filename.c_str(), "rb");

  auto result = id3::v2::load(fp);

  if (result.null()) {
    result = id3::v1::load(fp);
  }

  fclose(fp);
  return result;
}
