#########################################################################
#
# plain 'make' builds 3 files: a shared .so, a static .a
# and, from the objects compiled with -fPIC, a static *pic.a
#
# 'make test' compiles and runs an additional test file
#
# a compiler with c++11 support is required
# tested with gcc-4.7+ and clang-3.2

debug    = 0
libname  = srl
out      = bin
cache    = cache

CXXFLAGS = -std=c++11 -Wfatal-errors

ifeq ($(debug), 1)
	CXXFLAGS += -g -O0 -DDEBUG -pedantic -Wall -Wextra -Wshadow -ftrapv -Wcast-align
	out   := $(out)/debug
	cache := $(cache)/debug
else
	CXXFLAGS += -O3 -DNDEBUG
endif

header     = -Iinclude
libsrcdir  = src/lib
testsrcdir = src/tests

testfile   = tests

########################################################################

lib         = $(out)/lib$(libname)
libsrcs     = $(wildcard $(libsrcdir)/*.cpp)
libobjs_pic = $(patsubst %.cpp,$(cache)/pic/%.o, $(libsrcs))
libobjs_pdc = $(patsubst %.cpp,$(cache)/pdc/%.o, $(libsrcs))

testsrcs = $(wildcard $(testsrcdir)/*.cpp)
testobjs = $(patsubst %.cpp,$(cache)/pdc/%.o, $(testsrcs))

allobjects = $(libobjs_pic) $(libobjs_pdc) $(testobjs)

.PHONY: clean distclean print

all: print $(lib).so $(lib)pic.a $(lib).a
	@echo "...complete."

install:
	@echo "No install routine provided.\nYou can find the compiled libs in $(out)/ after running 'make'."
	@echo "'cat makefile' for further info."

print:
	@echo "building $(out)/$(libname) with...\n\t$(CXX) $(CXXFLAGS)"

# shared lib
$(lib).so: $(libobjs_pic)
	@echo "\tlinking $(lib).so"
	@$(CXX) $(CXXFLAGS) -shared -o $(lib).so $(libobjs_pic)

# static lib w/ -fPIC
$(lib)pic.a: $(libobjs_pic)
	@echo "\tpacking $(lib)pic.a"
	@$(AR) rcs $(lib)pic.a $(libobjs_pic)

# static lib w/o -fPIC
$(lib).a: $(libobjs_pdc)
	@echo "\tpacking $(lib).a"
	@$(AR) rcs $(lib).a $(libobjs_pdc)

# compile files
$(cache)/pic/%.o: %.cpp
	@$(call compile,$@,$<,-fPIC)

$(cache)/pdc/%.o: %.cpp
	@$(call compile,$@,$<)
	
# build and run test
test: print run
	@echo "...complete."

run: $(out)/$(testfile)
	@echo "\tRunning $(out)/$(testfile)"
	@./$(out)/$(testfile)

$(out)/$(testfile): $(lib).a $(testobjs)
	@echo "\tlinking $(out)/$(testfile)"
	@$(CXX) $(CXXFLAGS) -o $(out)/$(testfile) $(testobjs) $(lib).a

# $1 -> output object-file $2 -> source-file $3 -> additional flags
define compile
	mkdir -p $(out)
	mkdir -p $(dir $1)
	echo "\tcompiling $1"
	#compile and generate dependencies
	$(CXX) $(CXXFLAGS) $3 $(header) -MMD -MP -c -o $1 $2
endef

# include dependencies
-include $(allobjects:%.o=%.d)

clean:
	@rm -r -f $(cache)
	@echo "clean"

distclean: clean
	@rm -r -f $(out)
	@echo "distclean"
