CC = cc
CFLAGS = -I../pflayer -I. -O2
PFLAYER = ../pflayer/pflayer.o
AMLAYER = ../amlayer/amlayer.o

TOOLS = pf_benchmark student_store index_benchmark

all: $(TOOLS)

pf_benchmark: pf_benchmark.o $(PFLAYER)
	$(CC) -o $@ pf_benchmark.o $(PFLAYER)

student_store: student_store.o slot_page.o $(PFLAYER)
	$(CC) -o $@ student_store.o slot_page.o $(PFLAYER)

index_benchmark: index_benchmark.o slot_page.o $(PFLAYER) $(AMLAYER)
	$(CC) -o $@ index_benchmark.o slot_page.o $(PFLAYER) $(AMLAYER)

slot_page.o: slot_page.c slot_page.h
	$(CC) $(CFLAGS) -c slot_page.c

pf_benchmark.o: pf_benchmark.c
	$(CC) $(CFLAGS) -c pf_benchmark.c

student_store.o: student_store.c slot_page.h
	$(CC) $(CFLAGS) -c student_store.c

index_benchmark.o: index_benchmark.c slot_page.h
	$(CC) $(CFLAGS) -c index_benchmark.c

$(PFLAYER):
	$(MAKE) -C ../pflayer pflayer.o

$(AMLAYER):
	$(MAKE) -C ../amlayer amlayer.o

clean:
	rm -f *.o $(TOOLS)

