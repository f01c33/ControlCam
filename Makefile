#OBJS specifies which files to compile as part of the project
# OBJS = exemplo.cpp
OBJ = main
OUT = ctrlcam
EXT = .c

OBJ2 = replace
OUT2 = replace
EXT2 = .c

DEPS = './deps/flag/flag.c' './deps/b64/encode.c' './deps/jsmn/jsmn.c' -I './deps/jsmn'
DEPS = 
# DEPS2 = './deps/str-replace/str-replace.c' './deps/str-replace/occurrences.c' './deps/str-replace/strdup.c'
# './deps/flag/flag.c' './deps/b64/encode.c'
#'./deps/b64/decode.c'
#code.c

#CC specifies which compiler we're using
# CC = g++
CC = clang
CC2 = clang

#COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
COMPILER_FLAGS = -Wall -g -std=c99
COMPILER_FLAGS2 = -Wall -g -std=c99

#-Wpedantic 
#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = -lcurl -lm -L './deps/jsmn' -ljsmn
LINKER_FLAGS2 = 

# -lglut -lGL -lGLU -lXmu -lXext -lXi -lX11 -lm
# -lcurl
# -lm -lpthread -lSDL2
# -lgmpxx -lgmp

all: objects
# routine to run
objects: 
	$(CC) $(OBJ)$(EXT) $(DEPS) $(COMPILER_FLAGS) -o $(OUT) $(LINKER_FLAGS)
	$(CC2) $(OBJ2)$(EXT2) $(DEPS2) $(COMPILER_FLAGS2) $(LINKER_FLAGS2) -o $(OUT2)

# example code to clean stuff
clean:
	rm $(OBJ) $(OBJ2)