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

int main(int argc, char *argv[])
{
  char *filename;
  char *reversed_filename;
  SNDFILE *inputfile;
  SF_INFO inputfile_info;

  if (argc != 2) {
    printf("usage: reverse_sndfile FILENAME\n");
    return 0;
  }

  filename = argv[1];

  reversed_filename = (char *)calloc(MAX_FILENAME_LEN, sizeof(char));
  copy_up_to_char_or_max(reversed_filename,
                         filename,
                         '.',
                         strnlen(filename, MAX_FILENAME_LEN));

  strcat(reversed_filename, "_reversed");

  inputfile = sf_open(filename, SFM_READ, &inputfile_info);
  check(inputfile != NULL, "unable to open file");

  sf_close(inputfile);

  return 0;

 error:
  sf_close(inputfile);
  return 1;
}
