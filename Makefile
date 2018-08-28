# Makefile

CC       = gcc $(PROFILE)
#PROFILE  = -pg
DEBUG    = -DDEBUG
GDEBUG   = -g
OPTIMIZE = -O2 # -msse4 # -march=native -mtune=native #-msse4 -ffast-math
#ANDOR    = -DANDOR
#PVCAM    = -DPVCAM
#VSLIB	 = -DVSLIB
INCLUDES = -I.
THREADED = -D_REENTRANT -D_GNU_SOURCE -D_XOPEN_SOURCE -D_POSIX_C_SOURCE=200809L
FLAGS    = -Wall -fms-extensions -fno-strict-aliasing
CFLAGS   = $(FLAGS) $(THREADED) $(GDEBUG) $(DEBUG) $(OPTIMIZE) $(INCLUDES) $(ANDOR) $(PVCAM) $(VSLIB)
LDLIBS  = -L. -lm -lcfitsio -lpthread -ljpeg

# GLib part
CFLAGS += `pkg-config --cflags glib-2.0`
LDLIBS += `pkg-config --libs glib-2.0`

ifeq ($(shell uname), Darwin)
FLAGS += -fPIC -D_DARWIN_C_SOURCE
endif

ifeq ($(findstring MINGW, $(shell uname)), MINGW)
LDLIBS += -lws2_32
LDFLAGS += -static
endif

TARGETS = \
	photometer \
	coadd \

COMMON_OBJS = \
	utils.o \
	time_str.o \
	coords.o \
	image.o \
		image_keywords.o \
		image_fits.o \
		image_jpeg.o \
	server.o \
	queue.o \
	command.o \
	command_queue.o \
	random.o \

ifeq ($(shell uname), Linux)
COMMON_OBJS += popen_noshell.o
endif

FAST_OBJS = \
	fast.o \
		fast_grabber.o \
		fast_storage.o \

# ANDOR part
ifdef ANDOR
ifeq ($(shell uname), Linux)
LDLIBS  += -latcore
TARGETS += test_simple fast
COMMON_OBJS += grabber_andor.o
else
COMMON_OBJS += grabber_andor_fake.o
endif
endif

# PVCAM
ifdef PVCAM
ifeq ($(shell uname), Linux)
LDLIBS += -lpvcam -ldl -lraw1394
else
LDLIBS += -lPvcam32
endif
TARGETS += test_pvcam fast
COMMON_OBJS += grabber_pvcam.o
endif

# VSLIB
ifdef VSLIB
ifeq ($(findstring MINGW, $(shell uname)), MINGW)
INCLUDES += -I/c/Program\ Files/vs-lib3/sdk/include/
LDLIBS += -L/c/Program\ Files/vs-lib3/sdk/lib/i386/ -lvslib3
TARGETS += test_vslib fast
COMMON_OBJS += grabber_vslib.o
endif
endif

all: depend $(TARGETS)

$(TARGETS): $(COMMON_OBJS)

fast: $(FAST_OBJS)

clean:
	rm -f *~ *.o */*.o Makefile.depend* $(TARGETS) $(SCHEDULER_OBJS)

depend:
	@echo "# DO NOT DELETE THIS LINE -- make depend depends on it." >Makefile.depend
	@makedepend *.c -fMakefile.depend -I/usr/local/include 2>/dev/null

-include Makefile.depend # DO NOT DELETE
# DO NOT DELETE
