.PHONY: run run-cpp-default run-py run-ai-reference check check-py clean \
        build-c run-c build-cpp run-cpp run-ai \
        build-sheet-c build-sheet-cpp run-sheet-c run-sheet-cpp

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

# Recorded SHA-256 of the optional Python companions. check-py greets each file
# when its hash matches; update these if you intentionally edit the .py files.
AI_PLAYER_SHA256 := ab62c9226c8d0bc4daf0035ec793b176ff2cbcbd69668c8d93d12e1113406f33
PY_GAME_SHA256   := 4d9928d35575d97f0600bb9eaba551deb7bf23f8db3da88d6f2ff464ff639106

# ------------------------------------------------------------------------
# The native C and C++ builds drive Bayeishuui. `make run` launches the game.
# The Python files are preserved as optional companions, reachable on their own
# (run-py / run-ai-reference) and never required by the native build.
# ------------------------------------------------------------------------

# Default: launch the native game (C++ build drives Player vs Fighter Guild).
run: run-cpp

# Generate a Player 1 AI run config in the current working directory.
run-ai: run-cpp

# Optional Python companions, kept working on their own terms ----------------

# The original Python terminal prototype.
run-py:
	$(PYTHON) $(PY_GAME)

# The engine-neutral Player 1 AI reference module (prints its own guidance).
run-ai-reference:
	$(PYTHON) $(AI_PLAYER)

# ------------------------------------------------------------------------
# check: the native build gate. Compiles/links C, C++, and the sheet demos.
# No Python interpreter required. This is the source of truth for CI.
# ------------------------------------------------------------------------
check:
	$(MAKE) build-c
	$(MAKE) build-cpp
	$(MAKE) build-sheet-c
	$(MAKE) build-sheet-cpp

# ------------------------------------------------------------------------
# check-py: a friendly, always-successful nod to the optional Python files.
# For each file: if sha256 matches the recorded hash, greet it and let the
# module speak in its own voice; if it changed, say so gently; if the tools
# aren't present, skip quietly. Never fails the build.
# ------------------------------------------------------------------------
check-py:
	@echo "Saying hello to the optional Python companions..."
	@$(call greet_python,$(AI_PLAYER),$(AI_PLAYER_SHA256),Player 1 AI reference)
	@$(call greet_python,$(PY_GAME),$(PY_GAME_SHA256),Python game prototype)

# greet_python(file, expected_sha, label): verify + greet, always exit 0.
define greet_python
	if ! command -v sha256sum >/dev/null 2>&1; then \
	  echo "  - $(3): sha256sum not available, skipping (that's OK)."; \
	elif [ ! -f "$(1)" ]; then \
	  echo "  - $(3): $(1) not found, skipping (that's OK)."; \
	else \
	  got=$$(sha256sum "$(1)" | cut -d' ' -f1); \
	  if [ "$$got" = "$(2)" ]; then \
	    echo "  + $(3): checksum matches -- hello, $(1)!"; \
	    if command -v $(PYTHON) >/dev/null 2>&1; then \
	      $(PYTHON) "$(1)" 2>/dev/null | sed 's/^/      /' || true; \
	    else \
	      echo "      ($(PYTHON) not installed; greeting only.)"; \
	    fi; \
	  else \
	    echo "  ~ $(3): $(1) has changed since its recorded checksum."; \
	    echo "      If that was intentional, update its *_SHA256 in the Makefile."; \
	  fi; \
	fi
endef

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
