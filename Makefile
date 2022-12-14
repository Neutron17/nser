CC = cc

PNAME = c

SUBPROJ = binary

SRC  = $(shell find src/$(SUBPROJ) -name "*.c")
OBJ  = $(SRC:.c=.o)
BIN = build

EXEC = $(BIN)/$(PNAME)
INCFLAGS  = -Isrc/binary

CCFLAGS += $(INCFLAGS)
CCFLAGS += -O0
CCFLAGS += -Wall
CCFLAGS += -pedantic
CCFLAGS += -ggdb

LDFLAGS  = $(INCFLAGS)
LDFLAGS += -lpthread
LDFLAGS += -lrt

INSTALL_PATH = /usr/local/bin

all: build

run: build
	$(BIN)/$(PNAME) $*

build: $(OBJ)
	$(CC) $(CCFLAGS) -ggdb -o $(BIN)/$(PNAME) $(filter %.o,$^) $(LDFLAGS)

clean:
	rm $(BIN)/* $(OBJ)

install: build
	cp $(BIN)/$(PNAME) $(INSTALL_PATH)

%.o: %.c
	$(CC) -ggdb -o $@ -c $< $(CCFLAGS)

