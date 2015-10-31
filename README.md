musictags
=========

Extracts music tags from files.
It aims to parse all different kind of formats. Use `load` with a filename to get the tags.

Example usage:

```c++
#include "hugopeixoto/musictags.h"
#include <iostream>

int main(int argc, char* argv[]) {
  auto response = musictags::load(argv[1]);

  if (response.null()) {
    std::cout << "no tags detected in " << argv[1] << std::endl;
  } else {
    std::cout << response.get().version << std::endl;
    std::cout << "artist: " << response.get().artist << std::endl;
    std::cout << "album:  " << response.get().album << std::endl;
    std::cout << "track:  " << response.get().track_no << std::endl;
    std::cout << "title:  " << response.get().title << std::endl;
  }

  return 0;
}
```
