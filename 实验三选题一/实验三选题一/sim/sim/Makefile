CC                   = g++
AS                   = g++
LD                   = g++
CFLAGS              += -D_GNU_SOURCE -O0 -g -static
INCLUDE              = -I.
LDFLAGS             += $(SUBDIRS:%=-L %) -lpthread

#SOURCES              = $(shell ls *.c)
SOURCES              = main.cpp
OBJECTS              = $(SOURCES:.cpp=.o) lsq.o tlb.o memory.o
OUTPUT               = main

BUILDDIRS            = $(SUBDIRS:%=build-%)
CLEANDIRS            = $(SUBDIRS:%=clean-%)

# .SILENT:
all: $(OUTPUT)

$(OUTPUT): $(BUILDDIRS) $(OBJECTS)
	$(LD) $(OBJECTS) -no-pie  $(LDFLAGS) -o $(OUTPUT) 

%.o : %.cpp agu.h controller.h define.h
	$(CC) $(CFLAGS) $(INCLUDE) -c $<


clean: $(CLEANDIRS)
	rm -f $(OBJECTS) $(OUTPUT)

