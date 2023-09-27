.phony all:
all: test

test: test.c
	gcc test.c -lreadline -lhistory -o test

.PHONY clean:
clean:
	-rm -rf *.o *.exe
