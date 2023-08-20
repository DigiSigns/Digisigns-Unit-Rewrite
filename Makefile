CC = gcc
CFLAGS = -Wall -pedantic -pipe -g

SRC = src
OBJ = build
DATADIR = resources
LINK = $(patsubst %,-l %, curl pq pthread avcodec avformat)

OUTNAME = app

.PHONY: all clean

all: $(OBJ) $(DATADIR) $(DATADIR)/video_inputs $(DATADIR)/video_outputs $(OUTNAME)

$(OBJ):
	@mkdir $(OBJ)

$(DATADIR):
	@mkdir $(DATADIR)

$(DATADIR)/video_inputs:
	@mkdir $(DATADIR)/video_inputs

$(DATADIR)/video_outputs:
	@mkdir $(DATADIR)/video_outputs

$(OUTNAME): $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(wildcard $(SRC)/*.c))
	$(CC) $(CFLAGS) -o $@ $^ $(LINK)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^ -I /usr/include/x86_64-linux-gnu/

clean:
	@rm -rf $(OUTNAME) $(OBJ)/* $(DATADIR)/* $(SRC)/.*.swp
	@rmdir $(OBJ)
	@rmdir $(DATADIR)
