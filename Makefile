#
# Build CScout and its prerequisites
#
# (C) Copyright 2016 Diomidis Spinellis
#
# This file is part of CScout.
#
# CScout is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# CScout is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with CScout.  If not, see <http://www.gnu.org/licenses/>.
#

export CSCOUT_DIR=$(shell pwd)
HSQLDB_VERSION=2.4.1
HSQLDB_URL=http://downloads.sourceforge.net/project/hsqldb/hsqldb/hsqldb_2_4/hsqldb-$(HSQLDB_VERSION).zip
DEFAULT_HSQLDB_DIR=$(CSCOUT_DIR)/hsqldb-$(HSQLDB_VERSION)/hsqldb
export HSQLDB_DIR?=$(DEFAULT_HSQLDB_DIR)

.PHONY: src/build/cscout cpprest btyacc/btyacc

src/build/cscout: casablanca/cpprest btyacc/btyacc
	cd src && $(MAKE)

cpprest: casablanca/build.debug ninja
	cd casablanca/build.debug && sudo ninja install

casablanca/build.debug: casablanca
	cd casablanca && mkdir build.debug
	
casablanca:
	git clone https://github.com/Microsoft/cpprestsdk.git casablanca
	
ninja: build.ninja	
	cd casablanca/build.debug && ninja

build.ninja: 
	cd casablanca/build.debug && cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release

btyacc/btyacc: btyacc
	cd btyacc && $(MAKE)

btyacc:
	git clone https://github.com/dspinellis/btyacc

# Default installation of HSQLDB
$(DEFAULT_HSQLDB_DIR):  hsqldb-$(HSQLDB_VERSION).zip
	unzip $< && touch $@

hsqldb-$(HSQLDB_VERSION).zip:
	wget $(HSQLDB_URL) || curl -O $(HSQLDB_URL)

test: src/build/cscout $(HSQLDB_DIR)
	cd src && $(MAKE) test

clean:
	cd src && $(MAKE) clean

install: src/build/cscout
	cd src && $(MAKE) install

uninstall:
	cd src && $(MAKE) uninstall

example: src/build/cscout
	cd src && $(MAKE) example