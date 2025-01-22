include .env

CFLAGS = -std=c++17 -I. -I$(VULKAN_SDK_PATH)/include -I$(TINYOBJ_PATH) -I./systems
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan

# create list of all spv files and set as dependency
vertSources = $(shell find ./shaders -type f -name "*.vert")
vertObjFiles = $(patsubst %.vert, %.vert.spv, $(vertSources))
fragSources = $(shell find ./shaders -type f -name "*.frag")
fragObjFiles = $(patsubst %.frag, %.frag.spv, $(fragSources))

# Find all .cpp files in the root and systems directory
SOURCES = $(wildcard *.cpp systems/*.cpp)

TARGET = a.out

# Build target executable in one go
$(TARGET): $(SOURCES) $(vertObjFiles) $(fragObjFiles)
	g++ $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

# Rule for compiling shader files into .spv
%.spv: %
	${GLSLC} $< -o $@

.PHONY: test clean

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
	rm -f *.o
	rm -f shaders/*.spv
