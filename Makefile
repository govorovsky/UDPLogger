PREFIX=/usr/local
BINDIR= $(PREFIX)/bin
LIBS = 
PROGRAM1 = logger
PROGRAM2 = sender
PROGRAM3 = reader
CC=gcc
CFLAGS= 
COMPILE = $(CC) $(CFLAGS) -c
LINK = $(CC) $(CFLAGS)


all: $(PROGRAM1) $(PROGRAM2) $(PROGRAM3)

$(PROGRAM1): logger.o logger.h 
	$(LINK) logger.o -o $(PROGRAM1)
$(PROGRAM2): sender.o 
	$(LINK) sender.o -o $(PROGRAM2)
$(PROGRAM3): readlog.o 
	$(LINK) readlog.o -o $(PROGRAM3)
logger.o: logger.c
	$(COMPILE) logger.c
sender.o: sender.c
	$(COMPILE) sender.c
reader.o: readlog.c logger.h
	$(COMPILE) readlog.c
uninstall:
	cd $(BINDIR) && if [ -f "./$(PROGRAM1)" ];then rm $(PROGRAM1);fi
install:
	cp -f $(PROGRAM1) $(BINDIR)
clean:
	rm -rf *.o $(PROGRAM1) $(PROGRAM2) $(PROGRAM3)
	
