project_files := $(shell aegis -l pf -terse -p $(project) -c $(change))
change_files  := $(shell aegis -l cf -terse -p $(project) -c $(change))
source_files  := $(sort $(project_files) $(change_files))
ccobject_files:= $(patsubst %.cc,%.o,$(filter %.cc,$(source_files)))
cobject_files := $(patsubst %.c,%.o,$(filter %.c,$(source_files)))
object_files  := $(cobject_files) $(ccobject_files)

CC= gcc
CCPP= g++
INCDIRS=-I/usr/local/openmail/include
DEFINES=-DIBM_PORT -DPROTOTYPES_AVAIL -DSTDARG_AVAIL
OPTS=-g
CFLAGS=$(INCDIRS) $(DEFINES) $(OPTS) -DNO_INLINES -DDEBUG=1
#CFLAGS=$(INCDIRS) $(DEFINES) $(OPTS)
LFLAGS=$(LIBDIRS) $(OPTS) -L.
OMD=/usr/local/openmail
OMDL=$(OMD)/lib
OMDI=$(OMD)/include

MLIBS=$(OMDL)/exuak.a $(OMDL)/extf.a $(OMDL)/exual.a $(OMDL)/mdc.a\
	 $(OMDL)/nls.a
SLIBS=-lcurses
OMSLIB=-loms
OBJS=agent.o mail.o omutil.o screens0.o screens1.o screens2.o

all: $(object_files) liboms.a agent browse omshc advmail.idx tar

agent: $(OBJS) liboms.a
	-rm -f agent core
	-strip -r ansicurs.o
	-strip -r config.o
	$(CCPP) $(LFLAGS) -o agent $(OBJS) $(OMSLIB) $(MLIBS) $(SLIBS)

browse: hypertxt.cc liboms.a
	rm -f browse
	$(CC) $(LFLAGS) -DSTANDALONE -o browse hypertxt.cc $(OMSLIB) $(SLIBS)

omshc: omshc.o
	$(CC) $(LFLAGS) -o omshc omshc.o

advmail.idx: advmail.txt omshc
	./omshc advmail.txt

liboms.a: screen.o ed.o hypertxt.o config.o ansicurs.o
	-rm -f liboms.a
	ar r liboms.a screen.o ed.o hypertxt.o config.o ansicurs.o
	-ranlib liboms.a

%.o: %.cc
	rm -f $*.o
	$(CC) $(CFLAGS) -c $*.cc

%.o: %.c
	rm -f $*.o
	$(CC) $(CFLAGS) -c $*.c

%.d: %.cc
	rm -f $*.d
	$(CC) $(CFLAGS) -M $*.cc | sed 's/^\(.*\).o :/\1.o \1.d :/' > $*.d

%.d: %.c
	rm -f $*.d
	$(CC) $(CFLAGS) -M $*.c | sed 's/^\(.*\).o :/\1.o \1.d :/' > $*.d

tar:
	-tar cvhf /home/gram/ommail.tar $(source_files)

include $(patsubst %.o,%.d,$(object_files))



