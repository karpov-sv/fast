# Makefile

CC       = gcc $(PROFILE)
#PROFILE  = -pg
DEBUG    = -DDEBUG
GDEBUG   = -g
OPTIMIZE = -O2 # -msse4 # -march=native -mtune=native #-msse4 -ffast-math
#ANDOR    = -DANDOR
#ANDOR2    = -DANDOR2
#PVCAM    = -DPVCAM
#VSLIB	 = -DVSLIB
#PVCAM    = -DPVCAM
#VSLIB	 = -DVSLIB
#CSDU     = -DCSDU
GXCCD    = -DGXCCD
# QHY	 = -DQHY
# FAKE	 = -DFAKE
INCLUDES = -I.
THREADED = -D_REENTRANT -D_GNU_SOURCE -D_XOPEN_SOURCE -D_POSIX_C_SOURCE=200809L
FLAGS    = -Wall -fms-extensions -fno-strict-aliasing
CFLAGS   = $(FLAGS) $(THREADED) $(GDEBUG) $(DEBUG) $(OPTIMIZE) $(INCLUDES) $(ANDOR) $(ANDOR2) $(PVCAM) $(VSLIB) $(CSDU) $(QHY) $(GXCCD) $(FAKE)
CXXFLAGS = $(CFLAGS)
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
	coadd \
	test \

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
endif
endif

ifdef ANDOR2
LDLIBS  += -landor
TARGETS += test_andor2 fast
COMMON_OBJS += grabber_andor2.o
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

# CSDU
ifdef CSDU
LDLIBS += -lstt_cam
TARGETS += test_csdu fast
COMMON_OBJS += grabber_csdu.o
endif

# QHY
ifdef QHY
LDLIBS += -lqhyccd
TARGETS += test_qhy fast
COMMON_OBJS += grabber_qhy.o
endif

# Moravian Instruments GXCCD
ifdef GXCCD
#LDLIBS += -L../libgxccd/lib -lgxccd
LDLIBS +=  ../libgxccd/lib/libgxccd.a -lusb-1.0
INCLUDES += -I../libgxccd/include
TARGETS += fast
COMMON_OBJS += grabber_gxccd.o
endif

# Fake grabber
ifdef FAKE
TARGETS += fast
COMMON_OBJS += grabber_fake.o
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
