CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGET = mmu
SRC = mmu.c
OBJ = mmu.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC)

clean:
	rm -f $(OBJ) $(TARGET) output256.csv output128.csv

.PHONY: all clean
