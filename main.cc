#include "hugopeixoto/musictags.h"
#include <iostream>

int main(int argc, char* argv[]) {
  auto response = musictags::load(argv[1]);

  if (response.null()) {
    std::cout << "no tags detected in " << argv[1] << std::endl;
  } else {
    auto tags = response.get();
    std::cout << tags.version << std::endl;
    std::cout << "artist: " << tags.artist << std::endl;
    std::cout << "album:  " << tags.album << std::endl;
    std::cout << "track:  " << tags.track_no << std::endl;
    std::cout << "title:  " << tags.title << std::endl;
  }

  return 0;
}
