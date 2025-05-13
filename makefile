CC = gcc
SRC_DIR = src
INC_DIR = includes
OBJ_DIR = obj
BIN = c-gtk-sql-server

CFLAGS = -Wall -g -I$(INC_DIR) `pkg-config --cflags gtk4`
LDFLAGS = `pkg-config --libs gtk4 jansson` -lsqlite3 -lssl -lcrypto

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN)
