# cc -g -Wall -Wextra reverse_sndfile.c -o reverse_sndfile -ldl -lsndfile /usr/lib/x86_64-linux-gnu/libsndfile.a
CFLAGS=-g -Wall -Wextra -ldl -lsndfile /usr/lib/x86_64-linux-gnu/libsndfile.a

all: build

build: reverse_sndfile

clean:
	rm -f reverse_sndfile
