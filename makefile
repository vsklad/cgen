#
#  CGen makefile
#  https://cgen.sophisticatedways.net
#  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
#  Published under terms of MIT license.
#

PATH_ACL = ./acl
PATH_PLE_BASE = ./ple/base
PATH_PLE_IO = ./ple/io
PATH_PLE_VARIABLES = ./ple/variables
PATH_PLE_LIBRARY = ./ple/library
PATH_PLE_ANF = ./ple/anf
PATH_PLE_CNF = ./ple/cnf

BIN_NAME = cgen
CPLUS_FLAGS = -I. -I$(PATH_ACL) -I$(PATH_PLE_BASE) -I$(PATH_PLE_IO) -I$(PATH_PLE_VARIABLES) -I$(PATH_PLE_LIBRARY) -I$(PATH_PLE_ANF) -I$(PATH_PLE_CNF) -std=c++11

all:
	g++ $(CPLUS_FLAGS) -c *.cpp
	g++ $(CPLUS_FLAGS) -c $(PATH_PLE_VARIABLES)/*.cpp
	g++ $(CPLUS_FLAGS) -c $(PATH_PLE_LIBRARY)/*.cpp
	g++ $(CPLUS_FLAGS) -c $(PATH_PLE_ANF)/*.cpp
	g++ *.o -o ${BIN_NAME}
clean:
	rm -rf *.o
	rm ${BIN_NAME}