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

int copy_section_in_reverse(SNDFILE *inputfile, SNDFILE *outputfile,
                            const SF_INFO *file_info,
                            const int start_frame, const int chunk_size)
{
  // iterate backwards through inputfile, writing forwards into outputfile

  sf_count_t inputfile_offset_from_end = -1 - start_frame;
  sf_count_t outputfile_offset = start_frame;
  sf_count_t end_frame = start_frame + chunk_size;
  float *copy_array;

  if ((copy_array = malloc(file_info->channels * sizeof(float))) == NULL) {
    fprintf(stderr, "memory allocation failed\n");
    return 1;
  }

  while (outputfile_offset <= end_frame) {
    sf_seek(inputfile, inputfile_offset_from_end--, SEEK_END);
    sf_read_float(inputfile, copy_array, 1);

    sf_seek(outputfile, outputfile_offset++, SEEK_SET);
    sf_write_float(outputfile, copy_array, 1);
  }

  free(copy_array);

  return 0;
}

void *pcopy_section_in_reverse(void *copySectionArgsStruct)
{
  CopySectionArgs *args = (CopySectionArgs *)copySectionArgsStruct;
  int response_code = 0;

  // iterate backwards through inputfile, writing forwards into outputfile

  sf_count_t inputfile_offset_from_end = -1 - args->start_frame;
  sf_count_t outputfile_offset = args->start_frame;
  sf_count_t end_frame = args->start_frame + args->chunk_size;
  float *copy_array;

  if ((copy_array = malloc(args->file_info->channels * sizeof(float))) == NULL) {
    fprintf(stderr, "memory allocation failed\n");
    response_code = 1;
    pthread_exit(&response_code);
  }

  while (outputfile_offset <= end_frame) {
    sf_seek(args->inputfile, inputfile_offset_from_end--, SEEK_END);
    sf_read_float(args->inputfile, copy_array, 1);

    sf_seek(args->outputfile, outputfile_offset++, SEEK_SET);
    sf_write_float(args->outputfile, copy_array, 1);
  }

  free(copy_array);

  pthread_exit(&response_code);
}

int copy_file_in_reverse(SNDFILE *inputfile, SNDFILE *outputfile, SF_INFO *file_info)
{
  int chunk_size = (int)file_info->frames / num_procs;
  int i = 0;
  int response_code = 0;
  pthread_t workers[num_procs];
  CopySectionArgs copySectionArgs[num_procs];
  int *worker_return_val = 0;

  for (i = 0; i < num_procs; i++) {
    copySectionArgs[num_procs].inputfile        = inputfile;
    copySectionArgs[num_procs].outputfile       = outputfile;
    copySectionArgs[num_procs].file_info        = file_info;
    copySectionArgs[num_procs].start_frame      = chunk_size * i;
    copySectionArgs[num_procs].chunk_size       = chunk_size;

    if (pthread_create(&workers[i], NULL, pcopy_section_in_reverse, &copySectionArgs)) {
      fprintf(stderr, "Error creating thread\n");
      return 1;
    }
  }

  for (i = 0; i < num_procs; i++) {
    if (pthread_join(workers[i], (void *)worker_return_val)) {
      fprintf(stderr, "Error joining thread\n");
      return 1;
    }

    response_code += *worker_return_val;
  }

  return response_code;
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

  if ((copy_file_in_reverse(inputfile, outputfile, &inputfile_info))) {
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
