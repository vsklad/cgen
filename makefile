#
#  CGen makefile
#  https://cgen.sophisticatedways.net
#  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
#  Published under terms of MIT license.
#

BIN_NAME = cgen
BIN_NAME_OPTIMIZED = cgeno

CXX = g++
CXX_FLAGS = -I. -std=c++11

PATH_ACL = ./acl
CXX_FLAGS += -I$(PATH_ACL)
PATH_BAL_ANF = ./bal/anf
CXX_FLAGS += -I$(PATH_BAL_ANF)
PATH_BAL_BAL = ./bal/bal
CXX_FLAGS += -I$(PATH_BAL_BAL)
PATH_BAL_CNF_CNF = ./bal/cnf/cnf
CXX_FLAGS += -I$(PATH_BAL_CNF_CNF)
PATH_BAL_CNF_IO = ./bal/cnf/io
CXX_FLAGS += -I$(PATH_BAL_CNF_IO)
PATH_BAL_CNF_ENCODING = ./bal/cnf/encoding
CXX_FLAGS += -I$(PATH_BAL_CNF_ENCODING)
PATH_BAL_CNF_PROCESSOR = ./bal/cnf/processor
CXX_FLAGS += -I$(PATH_BAL_CNF_PROCESSOR)
PATH_BAL_CNF_TRACER = ./bal/cnf/tracer
CXX_FLAGS += -I$(PATH_BAL_CNF_TRACER)
PATH_BAL_ENCODING = ./bal/encoding
CXX_FLAGS += -I$(PATH_BAL_ENCODING)
PATH_BAL_FORMULA = ./bal/formula
CXX_FLAGS += -I$(PATH_BAL_FORMULA)
PATH_BAL_IO = ./bal/io
CXX_FLAGS += -I$(PATH_BAL_IO)
PATH_BAL_VARIABLES = ./bal/variables
CXX_FLAGS += -I$(PATH_BAL_VARIABLES)
PATH_BAL_UTILS = ./bal/utils
CXX_FLAGS += -I$(PATH_BAL_UTILS)

all: cgen cgen_optimized

cgen:
	$(CXX) $(CXX_FLAGS) -DCNF_TRACE -c *.cpp
	$(CXX) $(CXX_FLAGS) -DCNF_TRACE -c $(PATH_BAL_ANF)/*.cpp
	$(CXX) $(CXX_FLAGS) -DCNF_TRACE -c $(PATH_BAL_CNF_CNF)/*.cpp
	$(CXX) $(CXX_FLAGS) -DCNF_TRACE -c $(PATH_BAL_CNF_ENCODING)/*.cpp
	$(CXX) $(CXX_FLAGS) -DCNF_TRACE -c $(PATH_BAL_CNF_PROCESSOR)/*.cpp
	$(CXX) $(CXX_FLAGS) -DCNF_TRACE -c $(PATH_BAL_FORMULA)/*.cpp
	$(CXX) $(CXX_FLAGS) -DCNF_TRACE -c $(PATH_BAL_VARIABLES)/*.cpp
	$(CXX) *.o -o ${BIN_NAME}
	
cgen_optimized:
	$(CXX) $(CXX_FLAGS) -c *.cpp
	$(CXX) $(CXX_FLAGS) -c $(PATH_BAL_ANF)/*.cpp
	$(CXX) $(CXX_FLAGS) -c $(PATH_BAL_CNF_CNF)/*.cpp
	$(CXX) $(CXX_FLAGS) -c $(PATH_BAL_CNF_ENCODING)/*.cpp
	$(CXX) $(CXX_FLAGS) -c $(PATH_BAL_CNF_PROCESSOR)/*.cpp
	$(CXX) $(CXX_FLAGS) -c $(PATH_BAL_FORMULA)/*.cpp
	$(CXX) $(CXX_FLAGS) -c $(PATH_BAL_VARIABLES)/*.cpp
	$(CXX) *.o -o ${BIN_NAME_OPTIMIZED}

clean:
	rm -rf *.o
	rm -f ${BIN_NAME}
	rm -f ${BIN_NAME_OPTIMIZED}
