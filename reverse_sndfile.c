#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <string.h>
#include "dbg.h"
#include "reverse_sndfile.h"

int copy_up_to_char_or_max(char *dest, const char *input, const char upto, const int max)
{
  int i = 0;
  char current_char = input[i];

  while (i < max && current_char != '\0' && current_char != upto) {
    dest[i] = current_char;
    i++;
    current_char = input[i];
  }

  dest[i] = '\0';

  return 1;
}

void resolve_filename_extension(char** format_name, const SF_INFO sf_info)
{
  int format_masks[] = {SF_FORMAT_AIFF, SF_FORMAT_WAV, SF_FORMAT_FLAC, SF_FORMAT_OGG};
  char *format_extensions[] = {".aiff", ".wav", ".flac", ".ogg"};
  int i;

  while (i < 4) {
    if (sf_info.format & format_masks[i]) break;
    i++;
  }

  strcpy(*format_name, format_extensions[i]);
}

int main(int argc, char *argv[])
{
  char *filename;
  char *reversed_filename;
  char *format_name;
  SNDFILE *inputfile;
  SF_INFO inputfile_info;

  if (argc != 2) {
    printf("usage: reverse_sndfile FILENAME\n");
    return 0;
  }

  filename = argv[1];

  // open file

  inputfile = sf_open(filename, SFM_READ, &inputfile_info);
  check(inputfile != NULL, "unable to open file");

  // format output filename

  reversed_filename = (char *)calloc(MAX_FILENAME_LEN, sizeof(char));
  copy_up_to_char_or_max(reversed_filename,
                         filename,
                         '.',
                         strnlen(filename, MAX_FILENAME_LEN));

  strcat(reversed_filename, "_reversed");
  format_name = (char *)calloc(5, sizeof(char));
  resolve_filename_extension(&format_name, inputfile_info);
  strcat(reversed_filename, format_name);
  printf("writing output file to: %s\n", reversed_filename);

  sf_close(inputfile);
  return 0;

 error:
  sf_close(inputfile);
  return 1;
}
