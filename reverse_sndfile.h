#ifndef __reverse_sndfile_h__
#define __reverse_sndfile_h__

#define BUFFER_LEN 1024
#define MAX_CHANNELS 6
#define MAX_FILENAME_LEN 256

extern int num_procs = 2;

typedef struct {
  SNDFILE *inputfile;
  SNDFILE *outputfile;
  SF_INFO *file_info;
  int start_frame;
  int chunk_size;
} CopySectionArgs;

#endif
