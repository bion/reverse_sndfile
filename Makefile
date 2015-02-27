CFLAGS=-g -Wall -Wextra -Isrc -lsndfile

all: build

build: reverse_sndfile

clean:
	rm -f reverse_sndfile
