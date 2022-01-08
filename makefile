CC = gcc

prog = download
SRC := src
INCLUDES := ../include
OBJ := bin

SOURCES := $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

flags = -g -Wall 
libs =

download: bin download_inner

download_inner: $(OBJECTS)
	$(CC) $(flags)  $^ -o download $(libs) -I$(INCLUDES)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -I$(SRC) $(flags)  -c $< -o $@ $(libs) -I$(INCLUDES) 

bin: 
	mkdir -p bin

clean:
	rm -f $(OBJ)/*.o download
