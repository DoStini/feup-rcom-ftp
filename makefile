CC = gcc

prog = download
SRC := src
OBJ := bin

SOURCES := $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

flags = -g -Wall
libs =

download: bin download_inner

download_inner: $(OBJECTS)
	$(CC) $(flags)  $^ -o download

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -I$(SRC) $(flags)  -c $< -o $@

bin: 
	mkdir bin

clean:
	rm -f $(OBJ)/*.o download 
