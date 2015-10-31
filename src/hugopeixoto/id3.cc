#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <stdint.h>
#include <iconv.h>

uint32_t synchsafe(const uint8_t integer[4]);

// Utf8 invalid
const char* trim_and_escape(const char* a_text, int size = 1000)
{
  static char text[128];

  const char* end = a_text + size;

  while (isspace(*a_text)) a_text++;

  int i = 0, j = 0;
  for (; a_text < end && *a_text; a_text++) {
    if (*a_text == '"') text[i++] = '\\';
    if (!isspace(*a_text)) j = i;
    text[i++] = *a_text;
  }

  text[j+1] = 0;
  return text;
}

char* readUTF_8 (FILE* a_file, uint32_t a_size, char* a_src)
{
  char* to = new char[a_size + 1];
  memcpy(to, a_src, a_size);
  to[a_size] = 0;
  return to;
}

char* readUTF_16 (FILE* a_file, uint32_t a_size, char* a_src)
{
  iconv_t cd  = iconv_open("UTF-8", "UTF-16");
  char* to    = new char[a_size];

  char* ibuf = a_src;
  char* obuf = to;
  size_t isize = a_size;
  size_t osize = a_size;
  iconv(cd, &ibuf, &isize, &obuf, &osize);
  *obuf = 0;

  iconv_close(cd);
  return to;
}

char* readISO_8859_1 (FILE* a_file, uint32_t a_size, char* a_src)
{
  iconv_t cd  = iconv_open("UTF-8", "ISO-8859-1");
  char* to    = new char[a_size * 2];

  char* ibuf = a_src;
  char* obuf = to;
  size_t isize = a_size;
  size_t osize = a_size * 2;
  iconv(cd, &ibuf, &isize, &obuf, &osize);
  *obuf = 0;

  iconv_close(cd);
  return to;
}

char* read_string (FILE* a_file, uint32_t a_size, uint16_t a_flags = 0)
{
  uint8_t  encoding;
  char* raw;

  if (a_flags & 0x0200) {
    // assert that 0x0100
    uint8_t  derp[4];
    uint32_t length;
    if (a_flags & 0x0100) {
      fread(derp, 4, 1, a_file);
      length = synchsafe(derp);
    }
    raw = new char[a_size - 4];
    fread(raw, a_size - 4, 1, a_file);

    // printf("unsynched: %u -> %u\n", a_size, length);
    char* synched = new char[length];
    for (uint32_t i = 0, j = 0; i < a_size - 4; ++i) {
      if (((uint8_t*)raw)[i] == 0xFF && i + 2 < a_size  - 4 && raw[i + 1] == 0x00) {
        synched[j++] = raw[i]; // 0xFF
        synched[j++] = raw[i + 2];
        i += 2;
      } else {
        synched[j++] = raw[i];
      }
    }

    delete[] raw;
    raw = synched;
    a_size = length;

  } else {
    raw = new char[a_size];
    fread(raw, a_size, 1, a_file);
  }


  encoding = raw[0];
  // printf("encoding: %u\n", encoding);
  switch (encoding) {
    case 0x00: return readISO_8859_1(a_file, a_size - 1, raw + 1);
    case 0x01: return readUTF_16(a_file, a_size - 1, raw + 1);
    case 0x02: return readUTF_16(a_file, a_size - 1, raw + 1);
    case 0x03: return readUTF_8(a_file, a_size - 1, raw + 1);
  }

  return NULL; 
}

uint32_t synchsafe(const uint8_t integer[4])
{
  return (integer[3] <<  0) |
         (integer[2] <<  7) |
         (integer[1] << 14) |
         (integer[0] << 21);
}

uint32_t synchunsafe(const uint8_t integer[4])
{
  return (integer[3] <<  0) |
         (integer[2] <<  8) |
         (integer[1] << 16) |
         (integer[0] << 24);
}


void print (const char* tag, char* utf8string)
{
  printf("\"%s\": \"%s\",\n", tag, trim_and_escape(utf8string));
  delete[] utf8string;
}

void print_const (const char* tag, const char* utf8string)
{
  printf("\"%s\": \"%s\",\n", tag, trim_and_escape(utf8string));
}

struct ID3v2Header {
  char      magic[3];
  uint8_t   major;
  uint8_t   minor;
  uint8_t   flags;
  uint8_t   size[4];

  bool matches () const {
    return (memcmp(magic, "ID3", 3) == 0) &&
           (major < 0xFF && minor < 0xFF) &&
           ((flags & 0x0F) == 0) &&
           ((size[0] & 0x80) == 0) &&
           ((size[1] & 0x80) == 0) &&
           ((size[2] & 0x80) == 0) &&
           ((size[3] & 0x80) == 0);
  }
};

struct ID3v2_3ExtendedHeader {
  uint32_t size;
  uint16_t flags;
  uint32_t padding_size;
};

int PrintID3v2_2 (const ID3v2Header& a_header, FILE* a_file)
{
  struct {
    uint8_t identifier[3];
    uint8_t size_[3];

    uint32_t size () const { return (size_[0] << 16) | (size_[1] << 8) | (size_[2] << 0); }
  } FrameHeader;

  uint8_t FinalFrame[6] = { 0, 0, 0, 0, 0, 0 };

  uint32_t tags_size = synchsafe(a_header.size);
  uint32_t total_read = 0;
  while (total_read + 6 < tags_size) {
    fread(&FrameHeader, sizeof(FrameHeader), 1, a_file);
    if (memcmp(&FrameHeader, &FinalFrame, sizeof(FrameHeader)) == 0) {
      break;
    }

    // printf("frame %.3s (%u)\n", FrameHeader.identifier, FrameHeader.size());
    if (memcmp(FrameHeader.identifier, "TP1", 3) == 0) {
      print("artist", read_string(a_file, FrameHeader.size()));
    } else if (memcmp(FrameHeader.identifier, "TAL", 3) == 0) {
      print("album", read_string(a_file, FrameHeader.size()));
    } else if (memcmp(FrameHeader.identifier, "TT2", 3) == 0) {
      print("title", read_string(a_file, FrameHeader.size()));
    } else if (memcmp(FrameHeader.identifier, "TRK", 3) == 0) {
      print("track", read_string(a_file, FrameHeader.size()));
    } else {
      fseek(a_file, FrameHeader.size(), SEEK_CUR);
    }

    total_read += 6 + FrameHeader.size();
  }

  return 0;
}

int PrintID3v2_3 (const ID3v2Header& a_header, FILE* a_file)
{
  if (a_header.flags & 0x40) {
    printf("Has extended header.\n");
    ID3v2_3ExtendedHeader ex_header;
    fread(&ex_header, sizeof(ex_header), 1, a_file);
    if (ex_header.flags & 0x8000)
      fseek(a_file, 4, SEEK_CUR); // skip the CRC
  }

  struct {
    uint8_t  identifier[4];
    uint8_t  size_[4];
    uint16_t flags;

    uint32_t size () const { return (size_[0] << 24) | (size_[1] << 16) | (size_[2] << 8) | (size_[3] << 0); }
  } FrameHeader;

  uint8_t FinalFrame[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  uint32_t tags_size = synchsafe(a_header.size);
  uint32_t total_read = 0;
  while (total_read + 10 < tags_size) {
    fread(&FrameHeader, sizeof(FrameHeader), 1, a_file);
    if (memcmp(&FrameHeader, &FinalFrame, sizeof(FrameHeader)) == 0) {
      break;
    }

    //printf("frame %.4s (%u)\n", FrameHeader.identifier, FrameHeader.size());

    if (memcmp(FrameHeader.identifier, "TPE1", 4) == 0) {
      print("artist", read_string(a_file, FrameHeader.size()));
    } else if (memcmp(FrameHeader.identifier, "TALB", 4) == 0) {
      print("album", read_string(a_file, FrameHeader.size()));
    } else if (memcmp(FrameHeader.identifier, "TIT2", 4) == 0) {
      print("title", read_string(a_file, FrameHeader.size()));
    } else if (memcmp(FrameHeader.identifier, "TRCK", 4) == 0) {
      print("track", read_string(a_file, FrameHeader.size()));
    } else {
      fseek(a_file, FrameHeader.size(), SEEK_CUR);
    }

    total_read += 10 + FrameHeader.size();
  }

  return 0;
}

int PrintID3v2_4 (const ID3v2Header& a_header, FILE* a_file)
{
  if (a_header.flags & 0x40) {
    uint32_t extended_header_size;
    printf("Has extended header.\n");
    fread(&extended_header_size, 4, 1, a_file);
    fseek(a_file, extended_header_size - 4, SEEK_CUR);
  }

  struct {
    uint8_t  identifier[4];
    uint8_t  size_[4];
    uint16_t flags;
    uint32_t size () const { return synchsafe(size_); }
  } FrameHeader;

  uint8_t FinalFrame[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  uint32_t tags_size = synchsafe(a_header.size);
  uint32_t total_read = 0;
  while (total_read + 10 < tags_size) {
    fread(&FrameHeader, sizeof(FrameHeader), 1, a_file);

    // printf("Frame header: %.4s\n", FrameHeader.identifier);
    // printf("Size bytes: %X %X %X %X\n", FrameHeader.size_[0],
    //                                     FrameHeader.size_[1],
    //                                     FrameHeader.size_[2],
    //                                     FrameHeader.size_[3]);
    // printf("Flags: %X\n", FrameHeader.flags);
    if (memcmp(&FrameHeader, &FinalFrame, sizeof(FrameHeader)) == 0) {
      break;
    }

    if (memcmp(FrameHeader.identifier, "TPE1", 4) == 0) {
      print("artist", read_string(a_file, FrameHeader.size(), FrameHeader.flags));
    } else if (memcmp(FrameHeader.identifier, "TALB", 4) == 0) {
      print("album", read_string(a_file, FrameHeader.size(), FrameHeader.flags));
    } else if (memcmp(FrameHeader.identifier, "TIT2", 4) == 0) {
      print("title", read_string(a_file, FrameHeader.size(), FrameHeader.flags));
    } else if (memcmp(FrameHeader.identifier, "TRCK", 4) == 0) {
      print("track", read_string(a_file, FrameHeader.size(), FrameHeader.flags));
    } else {
      fseek(a_file, FrameHeader.size(), SEEK_CUR);
    }

    total_read += 10 + FrameHeader.size();
  }

  return 0;
}

int PrintID3v2Tag (char* a_filename)
{
  ID3v2Header header;

  FILE *fp;
  
  fp = fopen(a_filename, "r"); /* read only */

  if (fp == NULL) { /* file didn't open */
    return -1;
  }

  if (fread(&header, sizeof(header), 1, fp) != 1) {
    return -1;
  }

  if (!header.matches()) {
    fclose(fp);
    return -1;
  }

  // printf("ID3v2.%u.%u %u\n", header.major, header.minor, synchsafe(header.size));

  puts("{");
  switch (header.major) {
    case 4: PrintID3v2_4(header, fp); break;
    case 3: PrintID3v2_3(header, fp); break;
    case 2: PrintID3v2_2(header, fp); break;
    default: fclose(fp); return -1;
  }
  printf("\"filename\": \"%s\"\n}\n", trim_and_escape(a_filename));

  fclose(fp);
  return 0;
}

int PrintID3v1Tag(char *sFileName) 
// code from id3 
{
  struct id3 {
    char tag[3];
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    /* With ID3 v1.0, the comment is 30 chars long */
    /* With ID3 v1.1, if comment[28] == 0 then comment[29] == tracknum */
    char comment[30];
    unsigned char genre;
  } id3v1tag;

  FILE *fp;
  
  fp = fopen(sFileName, "r"); /* read only */

  if (fp == NULL) { /* file didn't open */
    fprintf(stderr, "fopen: %s: ", sFileName);
    perror("id3v2");
    return -1;
  }
  if (fseek(fp, -128, SEEK_END) < 0) {
    /* problem rewinding */
  } else { /* we rewound successfully */ 
    if (fread(&id3v1tag, 128, 1, fp) != 1) {
      /* read error */
      fprintf(stderr, "fread: %s: ", sFileName);
      perror("");
    }
  }
    
  fclose(fp);

  /* This simple detection code has a 1 in 16777216
   * chance of misrecognizing or deleting the last 128
   * bytes of your mp3 if it isn't tagged. ID3 ain't
   * world peace, live with it.
   */
  if (!strncmp(id3v1tag.tag, "TAG", 3))
  {
    puts("{");
    print_const("title"   , trim_and_escape(id3v1tag.title, 30));
    print_const("artist"  , trim_and_escape(id3v1tag.artist, 30));
    print_const("album"   , trim_and_escape(id3v1tag.album, 30));
    print_const("filename", trim_and_escape(sFileName));
    puts("}");
    return 0;
  } 

  return 1;     
}

int main (int argc, char *argv[])
{
  for (int i = 1; i < argc; ++i)
  {
    if (PrintID3v2Tag(argv[i]) == 0) continue;
    if (PrintID3v1Tag(argv[i]) == 0) continue;

    fprintf(stderr, "Failed to locate tags in %s\n", argv[i]);
  }

  return 0;
}

