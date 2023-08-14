CC = gcc
CFLAGS = -Wall -pedantic -pipe -g

SRC = src
OBJ = build
LINK = $(patsubst %,-l %,ssl crypto pq pthread)

OUTNAME = app

.PHONY: all clean

all: $(OBJ) $(OUTNAME)

$(OBJ):
	@mkdir $(OBJ)

$(OUTNAME): $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(wildcard $(SRC)/*.c))
	$(CC) $(CFLAGS) -o $@ $^ $(LINK)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	@rm -f $(OUTNAME) $(OBJ)/* $(SRC)/.*.swp
