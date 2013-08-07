TARGETS:=data.feed bin/feedd
DESTDIR:=/usr/local

# programmes
SQLITE3:=sqlite3
INSTALL:=install
CXX:=clang++
CC:=clang
PKGCONFIG:=pkg-config

# paths, etc
INCLUDES:=include
LIBRARIES:=sqlite3 libcurl libxml-2.0

# boost libraries
BOOSTLIBDIR:=
BOOSTLIBRARIES:=regex

# flags
CCFLAGS:=$(addprefix -I,$(INCLUDES)) $(addprefix -I,$(BOOSTLIBDIR))
PCCFLAGS:=$(shell $(PKGCONFIG) --cflags $(LIBRARIES))
PCLDFLAGS:=$(shell $(PKGCONFIG) --libs $(LIBRARIES)) $(addprefix -lboost_,$(BOOSTLIBRARIES))
CXXFLAGS:=-O3
CFLAGS:=
LDFLAGS:=
CXXR:=$(CXX) $(CCFLAGS) $(PCCFLAGS) $(CXXFLAGS) $(LDFLAGS) $(PCLDFLAGS)
CR:=$(CC) $(CCFLAGS) $(PCCFLAGS) $(CFLAGS) $(LDFLAGS) $(PCLDFLAGS)

# meta rules
all: $(TARGETS)
clean:
	rm -f $(TARGETS)

data.feed: src/feed.sql
	rm -f $@
	$(SQLITE3) $@ < $<

bin/.volatile:
	mkdir $(dir $@); true
	touch $@

bin/%: src/%.c++ bin/.volatile
	$(CXXR) $(CXXFLAGS) $< -o $@
