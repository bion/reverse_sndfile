#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <string.h>
#include <pthread.h>
#include "reverse_sndfile.h"

void copy_up_to_char_or_max(char *dest, const char *input, const char upto, const int max)
{
  int i = 0;
  char current_char = input[i];

  while (i < max && current_char != '\0' && current_char != upto) {
    dest[i] = current_char;
    i++;
    current_char = input[i];
  }

  dest[i] = '\0';
}

void resolve_filename_extension(char** format_extension, const SF_INFO sf_info)
{
  int format_masks[] = {SF_FORMAT_AIFF, SF_FORMAT_WAV, SF_FORMAT_FLAC, SF_FORMAT_OGG};
  char *format_extensions[] = {".aiff", ".wav", ".flac", ".ogg"};
  int i = 0;

  while (i < 4) {
    if (sf_info.format & format_masks[i]) break;
    i++;
  }

  strncpy(*format_extension, format_extensions[i], sizeof(char) * 5);
}

SF_INFO* create_output_file_info(const SF_INFO inputfile_info)
{
  SF_INFO *reversed_file_info = malloc(sizeof(SF_INFO));
  if (reversed_file_info == NULL) {
    fprintf(stderr, "memory allocation failed\n");
    exit(1);
  }

  memcpy(reversed_file_info, &inputfile_info, sizeof(SF_INFO));

  return reversed_file_info;
}

int copy_samples_in_reverse(SNDFILE *inputfile, SNDFILE *outputfile, SF_INFO *file_info)
{
  // iterate backwards through inputfile, writing forwards into outputfile

  sf_count_t inputfile_offset_from_end = -1;
  sf_count_t outputfile_offset = 0;
  float *copy_array;
  int num_frames = file_info->frames;

  if ((copy_array = malloc(file_info->channels * sizeof(float))) == NULL) {
    fprintf(stderr, "memory allocation failed\n");
    return 0;
  }

  while (outputfile_offset <= num_frames) {
    sf_seek(inputfile, inputfile_offset_from_end--, SEEK_END);
    sf_read_float(inputfile, copy_array, 1);

    sf_seek(outputfile, outputfile_offset++, SEEK_SET);
    sf_write_float(outputfile, copy_array, 1);
  }

  free(copy_array);

  return 1;
}

int main(int argc, char *argv[])
{
  char *filename;
  char *reversed_filename;
  char *format_extension;
  SNDFILE *inputfile;
  SNDFILE *outputfile;
  SF_INFO inputfile_info;
  SF_INFO *outputfile_info;

  if (argc != 2) {
    printf("usage: reverse_sndfile FILENAME\n");
    return 0;
  }

  filename = argv[1];

  // open input file

  if ((inputfile = sf_open(filename, SFM_READ, &inputfile_info)) == NULL) {
    printf("unable to open input file\n");
    return 1;
  };

  // format output filename

  if ((reversed_filename = malloc(MAX_FILENAME_LEN * sizeof(char))) == NULL) {
    fprintf(stderr, "memory allocation failed\n");
    goto error;
  }

  copy_up_to_char_or_max(reversed_filename,
                         filename,
                         '.',
                         strnlen(filename, MAX_FILENAME_LEN));

  strncat(reversed_filename, "_reversed", sizeof(char) * 9);

  if ((format_extension = malloc(10 * sizeof(char))) == NULL) {
    fprintf(stderr, "memory allocation failed\n");
    goto error;
  }

  resolve_filename_extension(&format_extension, inputfile_info);
  strncat(reversed_filename, format_extension, sizeof(char) * 10);

  printf("writing output file to: %s\n", reversed_filename);

  // open outputfile

  outputfile_info = create_output_file_info(inputfile_info);
  if ((outputfile = sf_open(reversed_filename, SFM_WRITE, outputfile_info)) == NULL) {
    fprintf(stderr, "unable to open output file\n");
    goto error;
  }

  if(!(copy_samples_in_reverse(inputfile, outputfile, &inputfile_info))) {
    fprintf(stderr, "error copying samples\n");
    goto error;
  }

  // tidy up

  free(format_extension);
  free(reversed_filename);
  free(outputfile_info);

  sf_close(inputfile);
  sf_close(outputfile);
  return 0;

 error:
  sf_close(inputfile);
  sf_close(outputfile);
  return 1;
}
