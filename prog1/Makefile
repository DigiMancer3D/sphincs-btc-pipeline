CC = gcc
CFLAGS = -O3 -march=native -std=c99 -Wall -Wextra -fomit-frame-pointer -I. \
         -Wno-unused-parameter -Wno-unused-variable -Wno-implicit-function-declaration \
         -fno-stack-protector

all: prog1

prog1: main.c address.c fips202.c fors.c merkle.c sign.c utils.c utilsx1.c wots.c wotsx1.c hash_shake.c thash_shake_simple.c randombytes.c
	$(CC) $(CFLAGS) -o prog1 main.c address.c fips202.c fors.c merkle.c sign.c utils.c utilsx1.c wots.c wotsx1.c hash_shake.c thash_shake_simple.c randombytes.c

clean:
	rm -f prog1
