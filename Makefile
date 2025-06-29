SRC:=src/*.c
LIBS:= -lm -mavx2
SRC_LIBS:=lib/argparse.c
INCLUDE_DIR:=-I"include/"
EXE:=iif

all:
	$(CC) $(SRC) $(SRC_LIBS) -o $(EXE) $(LIBS) $(INCLUDE_DIR)

clean:
	rm -fr ./iif
