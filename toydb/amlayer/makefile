a.out : am.o amfns.o amsearch.o aminsert.o amstack.o amglobals.o ../pflayer/pflayer.o main.o amscan.o amprint.o
	cc am.o amfns.o amsearch.o aminsert.o  amstack.o amglobals.o ../pflayer/pflayer.o main.o amscan.o amprint.o 

amlayer.o : am.o amfns.o amsearch.o aminsert.o amstack.o amglobals.o amscan.o amprint.o
	ld -r am.o amfns.o amsearch.o aminsert.o  amstack.o amglobals.o amscan.o amprint.o  -o amlayer.o

am.o : am.c am.h pf.h
	cc -m64 -c am.c

amfns.o : amfns.c am.h pf.h
	cc -m64 -c amfns.c

amsearch.o : amsearch.c am.h pf.h
	cc -m64 -c amsearch.c

aminsert.o : aminsert.c am.h pf.h
	cc -m64 -c aminsert.c

amscan.o : amscan.c am.h pf.h
	cc -m64 -c amscan.c

amstack.o : amstack.c am.h pf.h
	cc -m64 -c amstack.c

amglobals.o : amglobals.c am.h
	cc -m64 -c amglobals.c

amprint.o : amprint.c am.h pf.h 
	cc -m64 -c amprint.c
	
main.o : main.c am.h pf.h 
	cc -m64 -c main.c


