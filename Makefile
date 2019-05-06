# Axel Zuchovicki A01022875

CFLAGS = -Wall -pedantic
LDFLAGS = -fopenmp -lm 

LIBRARIES = tools.h FileOpertaions.h sockets/fatal_error.h sockets/FileTransmission.h sockets/sockets.h
SOURCES = main.c FileOperations.c sockets/fatal_error.c sockets/FileTransmission.c sockets/sockets.c
MAIN = error_correction.out

$(MAIN): $(SOURCES)
	gcc $^ -o $@ $(CFLAGS) $(LDFLAGS) -g

clean:
	rm -f $(MAIN) *.o *.zip

zip: clean
	zip -r $(MAIN).zip $(SOURCES) $(LIBRARIES) Makefile

.PHONY: clean zip
