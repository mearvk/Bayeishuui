.PHONY: run check clean build-c run-c build-cpp run-cpp run-ai build-sheet-c build-sheet-cpp run-sheet-c run-sheet-cpp

PYTHON ?= python3
CC ?= cc
CXX ?= c++
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c11
CXXFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c++17
PY_GAME := inventory/bayeshuui.py
AI_PLAYER := source/bayeishuui_ai.py
C_GAME := build/bayeishuui-c
CPP_GAME := build/bayeishuui-cpp
SHEET_C := build/bayeishuui-sheet-c
SHEET_CPP := build/bayeishuui-sheet-cpp
SAMPLE_SHEET := fighters/the_first_fang.sleela fighters/milo_quickstep.sleela

# Preserve the Python prototype.
run:
	$(PYTHON) $(PY_GAME)

# Generate a Player 1 AI run config in the current working directory.
run-ai: run-cpp

# Check Python syntax and compile both native implementations.
check:
	$(PYTHON) -m py_compile $(PY_GAME) $(AI_PLAYER)
	$(MAKE) build-c
	$(MAKE) build-cpp
	$(MAKE) build-sheet-c
	$(MAKE) build-sheet-cpp

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

# SLeeLa character sheet reader/interpreter/disposer demos.
build-sheet-c: build source/fighter_sheet.h
	$(CC) $(CFLAGS) source/fighter_sheet_demo.c -o $(SHEET_C)

build-sheet-cpp: build source/fighter_sheet.h
	$(CXX) $(CXXFLAGS) source/fighter_sheet_demo.cpp -o $(SHEET_CPP)

run-sheet-c: build-sheet-c
	./$(SHEET_C) $(SAMPLE_SHEET)

run-sheet-cpp: build-sheet-cpp
	./$(SHEET_CPP) $(SAMPLE_SHEET)

clean:
	rm -rf build
	find . -type d -name __pycache__ -prune -exec rm -rf {} +
