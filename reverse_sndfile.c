#include <stdio.h>
#include <sndfile.h>
#include "dbg.h"

int main(int argc, char *argv[])
{
  char *filename;
  SNDFILE *inputfile;
  SF_INFO inputfile_info;

  if (argc != 2)
    {
      printf("usage: reverse_sndfile FILENAME\n");
      return 0;
    }

  filename = argv[1];
  printf("reversing %s\n", filename);

  inputfile = sf_open(filename, SFM_READ, &inputfile_info);
  check(inputfile != NULL, "unable to open file");

  sf_close(inputfile);

  return 0;

 error:
  return 1;
}
