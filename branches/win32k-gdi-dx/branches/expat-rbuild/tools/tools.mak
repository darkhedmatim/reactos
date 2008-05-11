TOOLS_BASE = tools
TOOLS_BASE_ = $(TOOLS_BASE)$(SEP)
TOOLS_INT = $(INTERMEDIATE_)$(TOOLS_BASE)
TOOLS_INT_ = $(TOOLS_INT)$(SEP)
TOOLS_OUT = $(OUTPUT_)$(TOOLS_BASE)
TOOLS_OUT_ = $(TOOLS_OUT)$(SEP)

EXPAT_BASE_ = $(TOOLS_BASE_)expat$(SEP)
EXPAT_INT = $(TOOLS_INT_)expat
EXPAT_INT_ = $(EXPAT_INT)$(SEP)

TOOLS_CFLAGS = $(CFLAGS) -Os -Wall -Wpointer-arith -Wno-strict-aliasing \
  -DXMLNODE_LOCATION -DHAVE_EXPAT_CONFIG_H -DXMLCALL="" -DXMLIMPORT=""

TOOLS_CXXFLAGS = $(TOOLS_CFLAGS) $(CPPFLAGS) \
  -ftracer -momit-leaf-frame-pointer -mpreferred-stack-boundary=2 \
  -Itools

TOOLS_LFLAGS = $(LFLAGS)

$(TOOLS_INT) $(EXPAT_INT): | $(INTERMEDIATE)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(TOOLS_OUT): | $(OUTPUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

TOOLS_SOURCES = $(addprefix $(TOOLS_BASE_), \
	ssprintf.cpp \
	xmlstorage.cpp \
	xml.cpp \
	)

TOOLS_HEADERS = $(addprefix $(TOOLS_BASE_), \
	ssprintf.h \
	xmlstorage.h \
	xml.h \
	)

TOOLS_OBJECTS = \
	$(addprefix $(INTERMEDIATE_), $(TOOLS_SOURCES:.cpp=.o))

  
EXPAT_SOURCES = $(addprefix $(EXPAT_BASE_), \
	xmlparse.c \
	xmlrole.c \
	xmltok.c \
	)

EXPAT_OBJECTS = \
	$(addprefix $(INTERMEDIATE_), $(EXPAT_SOURCES:.c=.o))


$(TOOLS_INT_)ssprintf.o: $(TOOLS_BASE_)ssprintf.cpp $(TOOLS_HEADERS) | $(TOOLS_INT)
	$(ECHO_CC)
	${host_gcc} $(TOOLS_CXXFLAGS) -c $< -o $@

$(TOOLS_INT_)xml.o: $(TOOLS_BASE_)xml.cpp $(TOOLS_HEADERS) | $(TOOLS_INT)
	$(ECHO_CC)
	${host_gcc} $(TOOLS_CXXFLAGS) -c $< -o $@

$(TOOLS_INT_)xmlstorage.o: $(TOOLS_BASE_)xmlstorage.cpp $(TOOLS_HEADERS) | $(TOOLS_INT)
	$(ECHO_CC)
	${host_gcc} $(TOOLS_CXXFLAGS) -c $< -o $@


$(EXPAT_INT_)xmlparse.o: $(EXPAT_BASE_)xmlparse.c $(EXPAT_HEADERS) | $(EXPAT_INT)
	$(ECHO_CC)
	${host_gcc} $(TOOLS_CXXFLAGS) -c $< -o $@

$(EXPAT_INT_)xmlrole.o: $(EXPAT_BASE_)xmlrole.c $(EXPAT_HEADERS) | $(EXPAT_INT)
	$(ECHO_CC)
	${host_gcc} $(TOOLS_CXXFLAGS) -c $< -o $@

$(EXPAT_INT_)xmltok.o: $(EXPAT_BASE_)xmltok.c $(EXPAT_HEADERS) | $(EXPAT_INT)
	$(ECHO_CC)
	${host_gcc} $(TOOLS_CXXFLAGS) -c $< -o $@


include tools/bin2c.mak
include tools/rsym.mak
include tools/raddr2line.mak
include tools/pefixup.mak
include tools/bin2res/bin2res.mak
include tools/buildno/buildno.mak
include tools/cabman/cabman.mak
include tools/cdmake/cdmake.mak
include tools/gendib/gendib.mak
include tools/mkhive/mkhive.mak
include tools/nci/nci.mak
include tools/rbuild/rbuild.mak
include tools/unicode/unicode.mak
include tools/widl/widl.mak
include tools/winebuild/winebuild.mak
include tools/wmc/wmc.mak
include tools/wpp/wpp.mak
include tools/wrc/wrc.mak
