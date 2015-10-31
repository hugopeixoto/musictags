#include "hugopeixoto/musictags.h"
#include "hugopeixoto/musictags/id3/v1.h"
#include "hugopeixoto/musictags/id3/v2.h"

Nullable<musictags::Metadata> musictags::load(const std::string& filename) {
  FILE *fp = fopen(filename.c_str(), "rb");

  auto result = musictags::id3::v2::load(fp);

  if (result.null()) {
    result = musictags::id3::v1::load(fp);
  }

  fclose(fp);
  return result;
}
