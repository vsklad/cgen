#
#  CNFGen makefile
#  https://cnfgen.sophisticatedways.net
#  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
#  Published under terms of MIT license.
#

PATH_ACL = ./acl
PATH_PLE_CNF = ./ple/cnf
PATH_PLE_COMMON = ./ple/common
PATH_PLE_IO = ./ple/io

BIN_NAME = cnfgen
CPLUS_FLAGS = -I. -I$(PATH_ACL) -I$(PATH_PLE_CNF) -I$(PATH_PLE_COMMON) -I$(PATH_PLE_IO) -std=c++11

all: main.o commandline.o commands.o variablesio.o
	g++ main.o commandline.o commands.o variablesio.o -o ${BIN_NAME}
main.o: main.cpp
	g++ $(CPLUS_FLAGS) -c main.cpp
commandline.o: commandline.cpp
	g++ $(CPLUS_FLAGS) -c commandline.cpp
commands.o: commands.cpp
	g++ $(CPLUS_FLAGS) -c commands.cpp
variablesio.o: $(PATH_PLE_COMMON)/variablesio.cpp
	g++ $(CPLUS_FLAGS) -c $(PATH_PLE_COMMON)/variablesio.cpp
clean:
	rm -rf *.o
	rm ${BIN_NAME}