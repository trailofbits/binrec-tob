CC := $(CLANG)
DYNINST_INSTALL := $(S2EDIR)/build/dyninst_install
DYNINST_INCLUDE := $(DYNINST_INSTALL)/include
DYNINST_LIB := $(DYNINST_INSTALL)/lib
LD_LIBRARY_PATH += $(DYNINST_LIB)
CFLAGS += -std=c++11
LD_FLAGS += -Wl,-v,-rpath,'$(DYNINST_LIB)' -L$(DYNINST_LIB) -ldyninstAPI -linstructionAPI -std=c++11 -lstdc++ 
#-ldynDwarf -ldynElf

.PHONY: clean

all: replaceRecovFuncs

replaceRecovFuncs.o: replaceRecovFuncs.cpp 
	$(CC) -c replaceRecovFuncs.cpp  $(CFLAGS) -I$(DYNINST_INCLUDE)  

replaceRecovFuncs: replaceRecovFuncs.o 
	$(CC) replaceRecovFuncs.o  $(LD_FLAGS) -o replaceRecovFuncs 

