SRC:=src/*.c
LIBS:=-largparse -lm
LIBS_DIR:=-L"lib/"
EXE:=colorconv

all:
	$(CC) $(SRC) -o $(EXE) $(LIBS_DIR) $(LIBS)

clean:
	rm -fr ./colorconv
