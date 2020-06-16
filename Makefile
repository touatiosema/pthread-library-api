CFLAGS=-Wall -Wextra -Iinclude

TESTS=  01-main \
	02-switch \
	11-join \
	12-join-main \
	21-create-many	\
	22-create-many-recursive	\
	23-create-many-once	\
	31-switch-many	\
	32-switch-many-join	\
	33-switch-many-cascade \
	51-fibonacci	\
	61-mutex	\
	62-mutex

FILES=$(TESTS:%=build/%)
FILES_PTHREAD=$(TESTS:%=build/%-pthread)

all: $(FILES) $(FILES_PTHREAD)

src/thread.c : include/thread.h

build/thread.o: src/thread.c
	gcc $(CFLAGS) -fPIC -o $@ -c $<

build/libthread.so: build/thread.o
	gcc $(CFLAGS) -fPIC -shared -o $@ $^ $(LDFLAGS)

build/%-pthread.o: test/%.c
	gcc $(CFLAGS) -DUSE_PTHREAD -o $@ -c $<

build/%-pthread: build/%-pthread.o
	gcc -o $@ $^ -lpthread

build/%.o: test/%.c
	gcc $(CFLAGS) -o $@ -c $<

build/%: build/%.o build/libthread.so
	gcc -o $@ $^ -lrt

test:
	
install:
	mkdir -p install/bin
	mkdir -p install/lib
	cp ./build/libthread.so ./install/lib/
	cp ./build/* ./install/bin/
	rm ./install/bin/*.o ./install/bin/*.so	

valgrind:
	valgrind $(FILES)

MAX_THREADS ?= 500
graphs: ./src/graphs.sh ./src/graphs.py $(FILES) $(FILES_PTHREAD)
	bash ./src/graphs.sh
	python3 ./src/graphs.py

clean :
	rm -rf ./install build/*.o build/*.so $(FILES) $(FILES_PTHREAD) graph*
