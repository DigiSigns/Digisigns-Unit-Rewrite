CC = gcc
CFLAGS = -Wall -pedantic -pipe -g

SRC = src
OBJ = build
DATADIR = resources
LINK = $(patsubst %,-l %,ssl crypto pq pthread avcodec avformat)

OUTNAME = app

.PHONY: all clean

all: $(OBJ) $(DATADIR) $(OUTNAME)

$(OBJ):
	@mkdir $(OBJ)

$(DATADIR):
	@mkdir $(DATADIR)

$(OUTNAME): $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(wildcard $(SRC)/*.c))
	$(CC) $(CFLAGS) -o $@ $^ $(LINK)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^ -I /usr/include/x86_64-linux-gnu/

clean:
	@rm -f $(OUTNAME) $(OBJ)/* $(DATADIR)/* $(SRC)/.*.swp
	@rmdir $(OBJ)
	@rmdir $(DATADIR)
