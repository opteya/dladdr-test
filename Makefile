make_CFLAGS += -Wall
make_CFLAGS += -Wextra

make_CFLAGS += -Og
make_CFLAGS += -g
make_CFLAGS += -fstack-protector-strong
make_CFLAGS += -fstack-clash-protection
make_CFLAGS += -fasynchronous-unwind-tables
make_CFLAGS += -fexceptions

make_CPPFLAGS += -D_FORTIFY_SOURCE=2

make_LDFLAGS += -Wl,--hash-style,both
make_LDFLAGS += -Wl,-O1
make_LDFLAGS += -Wl,-z,defs
make_LDFLAGS += -Wl,-z,now
make_LDFLAGS += -Wl,-z,relro
make_LDFLAGS += -Wl,-z,noexecstack
make_LDFLAGS += -Wl,-z,separate-code

program_CFLAGS += -fPIE
program_CPPFLAGS += -DPIE
program_LDFLAGS += -pie
program_LDFLAGS += -rdynamic

shlib_CFLAGS += -fPIC
shlib_CPPFLAGS += -DPIC
shlib_LDFLAGS += -shared

PROGRAMS := dladdr-test-multiple
PROGRAMS += dladdr-test-single

SIZEMAX  := $(guile (expt 2 29))

define gensize
(define (power2 v m)
 (if (<= m 1)
  v
  (cons v (power2
   (* v 2)
   (/ m 2)))))
(power2 1 $1)
endef

SIZES := $(guile $(call gensize,$(SIZEMAX)))

define make_program_object
	$(CC) $(make_CFLAGS) $(program_CFLAGS) $2 $(CFLAGS) \
		$(make_CPPFLAGS) $(program_CPPFLAGS) $1 $(CPPFLAGS) \
		-c -o $@ $<
endef

define make_program
	$(CC) $(make_CFLAGS) $(program_CFLAGS) $2 $(CFLAGS) \
		$(make_LDFLAGS) $(program_LDFLAGS) $3 $(LDFLAGS) \
		-o $@ $^ \
		$(make_LIBS) $(program_LIBS) $4 $(LIBS)
endef

define make_shlib_object
	$(CC) $(make_CFLAGS) $(shlib_CFLAGS) $2 $(CFLAGS) \
		$(make_CPPFLAGS) $(shlib_CPPFLAGS) $1 $(CPPFLAGS) \
		-c -o $@ $<
endef

define make_shlib
	$(CC) $(make_CFLAGS) $(shlib_CFLAGS) $2 $(CFLAGS) \
		$(make_LDFLAGS) $(shlib_LDFLAGS) $3 $(LDFLAGS) \
		-o $@ $^ \
		$(make_LIBS) $(shlib_LIBS) $4 $(LIBS)
endef

all: $(PROGRAMS)

## disable most automatic / implicit rules
.SUFFIXES: ;

## disable all automatic, implicit rules
MAKEFLAGS+=-r

## disable makefile rebuilding
$(MAKEFILE_LIST): ;

clean:
	$(RM) $(PROGRAMS) $(libsymbol_LIBS) $(libsymbol_SOURCES) libsymbols.so generator *.o

define genlib
libsymbol$(size).so: libsymbol$(size).o
	$$(call make_shlib)

libsymbol$(size).o: libsymbol$(size).s
	$$(call make_shlib_object)

libsymbol$(size).s: generator
	./generator symbol $(size) > $$@

libsymbol_OBJS += libsymbol$(size).o
libsymbol_LIBS += libsymbol$(size).so
libsymbol_SOURCES += libsymbol$(size).s
endef

# build one library for each size
$(foreach size,$(SIZES),$(eval $(call genlib,$(size))))

# gather everything in a single library
libsymbols.so: $(libsymbol_OBJS)
	$(call make_shlib)

dladdr-test_LDFLAGS += -Wl,-rpath,\$$ORIGIN

dladdr-test-multiple: $(libsymbol_LIBS)
dladdr-test-multiple: dladdr-test.o
	$(call make_program,,,$(dladdr-test_LDFLAGS),-ldl)

dladdr-test-single: libsymbols.so
dladdr-test-single: dladdr-test.o
	$(call make_program,,,$(dladdr-test_LDFLAGS),-ldl)

dladdr-test.o: dladdr-test.c
	$(call make_program_object,-DSIZEMAX=$(SIZEMAX))

generator: generator.o
	$(call make_program)

generator.o: generator.c
	$(call make_program_object)
