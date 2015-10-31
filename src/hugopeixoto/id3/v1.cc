#include "hugopeixoto/id3/v1.h"
#include "hugopeixoto/id3/utils.h"
#include <cstdlib>

struct id3_v1 {
  char tag[3];
  char title[30];
  char artist[30];
  char album[30];
  char year[4];
  char comment[30];
  unsigned char genre;
};

Nullable<id3::MusicMetadata> id3::v1::load(const std::string& filename) {
  MusicMetadata result;

  id3_v1 tags;

  FILE *fp = fopen(filename.c_str(), "rb");

  result.version = "ID3v1.0";

  if (fp) {
    if (fseek(fp, -128, SEEK_END) == 0) {
      if (fread(&tags, 128, 1, fp) == 1) {
        if (!strncmp(tags.tag, "TAG", 3)) {
          result.artist = utils::trim(std::string(tags.artist, 30));
          result.album = utils::trim(std::string(tags.album, 30));
          result.title = utils::trim(std::string(tags.title, 30));
          result.year = atoi(std::string(tags.title, 30).c_str());

          if (tags.comment[28] == 0) {
            result.version = "ID3v1.1";
            result.track_no = tags.comment[29];
          }

          fclose(fp);
          return result;
        }
      }
    }
    fclose(fp);
  }

  return Nullable<id3::MusicMetadata>();
}
