CC = gcc

prog = download
SRC := src
INCLUDES := ./
OBJ := bin

SOURCES := $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

flags = -g -Wall -O3 -funroll-loops 
libs =

download: bin data download_inner

download_inner: $(OBJECTS)
	$(CC) $(flags)  $^ -o download $(libs) -I$(INCLUDES)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -I$(SRC) $(flags)  -c $< -o $@ $(libs) -I$(INCLUDES) 

bin: 
	mkdir -p bin

data: 
	mkdir -p data

clean:
	rm -f $(OBJ)/*.o download
