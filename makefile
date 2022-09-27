CPP      = cl
CPPFLAGS = /EHsc /std:c++20
SOURCES  = main.cpp aes.cpp
OBJS     = $(SOURCES:.cpp=.obj)

all: aes.exe

aes.exe: $(OBJS)
	$(CPP) $(CPPFLAGS) $(OBJS) /link /out:aes.exe

main.obj: aes.h
aes.obj: aes.h

clean:
	del aes.exe *.obj *.txt *.tmp
