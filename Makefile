CC = gcc
OFLAGS = -g -O
CFLAGS = $(OFLAGS) \
		 -Wextra -Wall \
		 -Wno-unused-parameter -Wno-missing-field-initializers \
		 -fsanitize=undefined -fsanitize=address \
		 -pthread

INCLUDES = -I./src
LIBS = -lGL -lm -lncurses -lglfw -lGLEW


SRC = $(shell find ./src -type f -name "*.c")
HDR = $(shell find ./src -type f -name "*.h")
OBJ = $(SRC:.c=.o)

all: clean compile

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

compile: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIBS) -o $@ 

run: compile
	./compile

clean:
	rm -f $(OBJ) compile

format: $(SRC) $(HDR)
	clang-format -i $(SRC) $(HDR)

.PHONY: clean
