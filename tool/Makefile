CC=g++
CFLAGS=-I./ -I../extern/ -I../extern/backward/include -ggdb -DDEBUG
LDFLAGS=-ldw
EXE=bin/linux_x86_64/Exporter

all: $(EXE) 
	$(EXE) ../src/shaders/candles.vert.spv ../src/shaders/candles.frag.spv
	
$(EXE): Exporter.o backward.o
	$(CC) $^ -o $@ $(LDFLAGS)

Exporter.o: Exporter.cpp Exporter.h
	$(CC) -c $< -o $@ $(CFLAGS)

backward.o: ../extern/backward/src/backward.cpp
	$(CC) -c $< -o $@ $(CFLAGS)



# CC=clang++
#  
# CFLAGS=-Wall -std=c++17 -Iinc -Iinc/imgui -Iinc/vma -Itracy/public/tracy
# LFLAGS=-Wall -std=c++17 -Llib -lglfw3dll -LC:/VulkanSDK/1.3.268.0/Lib -lvulkan-1 -Llib -lcpptrace
# 
# SRC_FILES=$(wildcard src/*.cpp)
# OBJ_FILES=$(patsubst src/%, obj/%, $(patsubst %.cpp, %.o, $(SRC_FILES)))
# IMGUI_SRC=$(wildcard src/imgui/*.cpp)
# IMGUI_OBJ=$(patsubst src/imgui/%, obj/imgui/%, $(patsubst %.cpp, %.o, $(IMGUI_SRC)))
# TRACY_OBJ=obj/tracy/TracyClient.o
# TRACY_SRC=tracy/public/TracyClient.cpp
# SHADER_SRC=$(wildcard src/shaders/*.glsl)
# SHADER_BIN=$(patsubst %.glsl, %.spv, $(SHADER_SRC))
# HEADER_ONLY_FILES=inc/vma/vk_mem_alloc.h
# #
# EXE=bin/main
# run:
# 	./$(EXE)
# 
# debug: CFLAGS += -DDEBUG -DTRACY_ENABLE -O0 -g -gcodeview -D_WIN32_WINNT=0x0602 -DWINVER=0x0602 -DTRACY_VK_USE_SYMBOL_TABLE
# debug: LFLAGS += -DDEBUG -DTRACY_ENABLE -O0 -fuse-ld=lld -g -Wl,--pdb= -lws2_32 -limagehlp
# debug:$(EXE)
# 	./$(EXE)
# 
# $(EXE): $(OBJ_FILES) $(IMGUI_OBJ) $(TRACY_OBJ)
# 	$(CC) $^ -o $@ $(LFLAGS)
# 
# obj/%.o: src/%.cpp inc/%.hpp
# 	$(CC) -c $< -o $@ $(CFLAGS)
# 
# obj/imgui/%.o: src/imgui/%.cpp
# 	$(CC) -c $< -o $@ $(CFLAGS)
# 
# $(TRACY_OBJ): $(TRACY_SRC) tracy/public/tracy/Tracy.hpp
# 	$(CC) -c $< -o $@ $(CFLAGS)
# 
# src/shaders/model.vert.spv: src/shaders/model.vert 
# 	glslc -fshader-stage=vertex $< -o $@
# 
# src/shaders/model.frag.spv: src/shaders/model.frag.spv
# 	glslc -fshader-stage=fragment $< -o $@
# 
# .PHONY: debug clean
