# Makefile for Atom Simulation - macOS, Linux and Windows (MSYS2/MinGW)
UNAME := $(shell uname -s)
SRC    = atomsimu.c
CFLAGS = -Wall

ifeq ($(UNAME),Darwin)
  # macOS: Apple's built-in GLUT/OpenGL frameworks
  CC      = clang
  TARGET  = atomsimu
  CFLAGS += -Wno-deprecated-declarations
  LDFLAGS = -framework GLUT -framework OpenGL -lm
else ifneq (,$(findstring MINGW,$(UNAME)))
  # Windows: MSYS2 MinGW + freeglut
  CC      = gcc
  TARGET  = atomsimu.exe
  LDFLAGS = -lfreeglut -lopengl32 -lglu32 -lm
else
  # Linux
  CC      = gcc
  TARGET  = atomsimu
  LDFLAGS = -lGL -lGLU -lglut -lm
endif

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f atomsimu atomsimu.exe