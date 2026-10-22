.PHONY: run check clean build-c run-c build-cpp run-cpp run-ai

PYTHON ?= python3
CC ?= cc
CXX ?= c++
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c11
CXXFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c++17
PY_GAME := inventory/bayeshuui.py
AI_PLAYER := source/bayeishuui_ai.py
C_GAME := build/bayeishuui-c
CPP_GAME := build/bayeishuui-cpp

# Preserve the Python prototype.
run:
	$(PYTHON) $(PY_GAME)

# Generate a Player 1 AI run config in the current working directory.
run-ai:
	$(PYTHON) $(AI_PLAYER)

# Check Python syntax and compile both native implementations.
check:
	$(PYTHON) -m py_compile $(PY_GAME) $(AI_PLAYER)
	$(MAKE) build-c
	$(MAKE) build-cpp

build:
	mkdir -p build

build-c: build source/bayeishuui_sprites.h
	$(CC) $(CFLAGS) source/bayeishuui.c -o $(C_GAME) -lm

build-cpp: build source/bayeishuui_sprites.h
	$(CXX) $(CXXFLAGS) source/bayeishuui.cpp -o $(CPP_GAME)

run-c: build-c
	./$(C_GAME)

run-cpp: build-cpp
	./$(CPP_GAME)

clean:
	rm -rf build
	find . -type d -name __pycache__ -prune -exec rm -rf {} +
