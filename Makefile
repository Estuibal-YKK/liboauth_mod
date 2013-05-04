release: all
	cp $(TARGET_LIB) release_psp/$(TARGET_LIB)
	cp $(TARGET_LIB_HEADER) release_psp/$(TARGET_LIB_HEADER)
	rm *.o
	rm $(TARGET_LIB)

TARGET_LIB = liboauth.a
TARGET_LIB_HEADER = oauth.h
OBJS = oauth.o
OBJS += oauth_http.o
OBJS += new_socket.o
OBJS += sha1.o
OBJS += hash.o
OBJS += xmalloc.o

INCDIR =
CFLAGS = -O3 -G0 -Wall -DPSP -fshort-wchar
# -DDEBUG
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lpspgu

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
