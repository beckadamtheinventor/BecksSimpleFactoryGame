#
# 'make'        build executable file 'main'
# 'make clean'  removes all .o and executable files
#

# define the C++ compiler to use
CXX = g++
# define the C compiler to use
CC = gcc

# define any compile-time flags
CXXFLAGS	:= -std=c++17 -Wall -Wextra -g

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
LFLAGS =

# define output directory
OUTPUT	:= output

# define source directory
SRC		:= src

# define include directory
INCLUDE	:= include

# define lib directory
LIB		:= lib

ifeq ($(OS),Windows_NT)
CXXFLAGS += -DPLATFORM_WINDOWS
MAIN	:= main.exe
SOURCEDIRS	:= $(SRC) $(SRC)\\engine $(SRC)\\engine\\components $(SRC)\\engine\\components\\opensimplex
INCLUDEDIRS	:= $(INCLUDE)
LIBDIRS		:= $(LIB)
FIXPATH = $(subst /,\,$1)
RM  := helpers/deletefile.exe
CP  := helpers/copyfile.exe
MD  := mkdir
else
MAIN	:= main
SOURCEDIRS	:= $(shell find $(SRC) -type d)
INCLUDEDIRS	:= $(shell find $(INCLUDE) -type d)
LIBDIRS		:= $(shell find $(LIB) -type d)
FIXPATH = $1
RM = rm -f
CP  := cp
MD	:= mkdir -p
endif

# define any directories containing header files other than /usr/include
INCLUDES	:= $(patsubst %,-I%, $(INCLUDEDIRS:%/=%))

# define the C libs
LIBS		:= $(patsubst %,-L%, $(LIBDIRS:%/=%)) -lraylib 
ifeq ($(OS),Windows_NT)
LIBS += -lopengl32 -lgdi32 -lwinmm
else
LIBS += -lm -lpthread -ldl -lrt
endif

RAYLIB_OBJECTS := $(wildcard lib/raylib/*.o)

# define the C source files
SOURCES		:= $(wildcard $(patsubst %,%/*.cpp, $(SOURCEDIRS)))

# define the C object files
OBJECTS		:= $(SOURCES:.cpp=.o)

# define the dependency output files
DEPS		:= $(OBJECTS:.o=.d)

#
# The following part of the makefile is generic; it can be used to
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

OUTPUTMAIN	:= $(call FIXPATH,$(OUTPUT)/$(MAIN))

all: $(OUTPUT) removebin $(MAIN)
	@echo Executing 'all' complete!

zip:
	$(CP) output/main.exe bsfg.exe
	7z a -mx8 -y bsfg.zip assets bsfg.exe

removebin:
ifeq ($(OS),Windows_NT)
	$(CC) helpers/deletefile.c -o helpers/deletefile.exe
	$(CC) helpers/copyfile.c -o helpers/copyfile.exe
endif
	$(RM) src/main.o src/main.d src/engine/bsfg.o src/engine/bsfg.d src/engine/components/ScriptInterface.o src/engine/components/ScriptInterface.d src/engine/components/World.o src/engine/components/World.d src/engine/components/ScriptAssemblyCompiler.o src/engine/components/ScriptAssemblyCompiler.d src/engine/components/opensimplex/OpenSimplex.o src/engine/components/opensimplex/OpenSimplex.d

$(OUTPUT):
	$(MD) $(OUTPUT)

$(MAIN): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(OUTPUTMAIN) $(OBJECTS) lib/zlib-1.3.1/libz.dll.a $(LFLAGS) $(LIBS)

# include all .d files
-include $(DEPS)

# this is a suffix replacement rule for building .o's and .d's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file)
# -MMD generates dependency output files same name as the .o file
# (see the gnu make manual section about automatic variables)
.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -MMD $<  -o $@

.PHONY: clean
clean:
	$(RM) $(OUTPUTMAIN)
	$(RM) $(call FIXPATH,$(OBJECTS))

run: all
	./$(OUTPUTMAIN)
	@echo Executing 'run: all' complete!
