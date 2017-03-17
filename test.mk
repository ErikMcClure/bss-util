TARGET := test
SRCDIR := test
BUILDDIR := bin
OBJDIR := bin/obj
C_SRCS := $(wildcard $(SRCDIR)/*.c)
CXX_SRCS := $(wildcard $(SRCDIR)/*.cpp)
INCLUDE_DIRS := include/bss-util
LIBRARY_DIRS := 
LIBRARIES := bss-util rt pthread

CPPFLAGS += -w -std=gnu++11 -pthread
LDFLAGS += -L./bin/

include base.mk

distclean:
	@- $(RM) $(OBJS)
	@- $(RM) -r $(OBJDIR)
	@- $(RM) bin/*.txt
	@- $(RM) bin/*.ini
	@- $(RM) bin/*.json
	@- $(RM) bin/*.ubj
	@- $(RM) bin/*.xml

