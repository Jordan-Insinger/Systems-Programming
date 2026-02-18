CC	= gcc
CFLAGS	= -o
RM	= rm -f


default: all

all: magic_transformer

magic_transformer: src/magic_transformer.c
	$(CC) $(CFLAGS) magic_transformer src/magic_transformer.c

clean veryclean:
	$(RM) magic_transformer
