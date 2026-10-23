# Bayeishuui — native C11 and C++17 terminal martial-arts duel.
# Best-effort, FHS-respecting build and install for both implementations.

.PHONY: all run check clean install uninstall \
        build-c run-c build-cpp run-cpp run-ai demo run-demo \
        install-c install-cpp uninstall-c uninstall-cpp help \
        build-sheet-c build-sheet-cpp run-sheet-c run-sheet-cpp

# ---- Toolchain and flags (all overridable) --------------------------------
PYTHON  ?= python3
CC      ?= cc
CXX     ?= c++
CFLAGS  ?= -O2 -Wall -Wextra -Wpedantic -std=c11
CXXFLAGS?= -O2 -Wall -Wextra -Wpedantic -std=c++17
LDLIBS_C ?= -lm

# ---- Project layout -------------------------------------------------------
VERSION   := 1.0.0
SRCDIR    := source
BUILDDIR  := build
MANDIR_SRC:= man
PY_GAME   := inventory/bayeshuui.py
AI_PLAYER := $(SRCDIR)/bayeishuui_ai.py
HEADERS   := $(SRCDIR)/bayeishuui_sprites.h $(SRCDIR)/bayeishuui_ai.h \
             $(SRCDIR)/bayeishuui_pixels.h
C_GAME    := $(BUILDDIR)/bayeishuui-c
CPP_GAME  := $(BUILDDIR)/bayeishuui-cpp
DEMO      := $(BUILDDIR)/bayeishuui-pixeldemo
SHEET_C   := $(BUILDDIR)/bayeishuui-sheet-c
SHEET_CPP := $(BUILDDIR)/bayeishuui-sheet-cpp
SAMPLE_SHEET := fighters/the_first_fang.sleela fighters/milo_quickstep.sleela
C_BIN     := bayeishuui-c
CPP_BIN   := bayeishuui-cpp
C_MAN     := $(MANDIR_SRC)/bayeishuui-c.6
CPP_MAN   := $(MANDIR_SRC)/bayeishuui-cpp.6
DOCS      := README.md LICENSE CONTROLS_AND_COMBAT.md MARTIAL_ARTS_DESIGN.md

# ---- Install locations (GNU / FHS conventions, overridable) ---------------
PREFIX    ?= /usr/local
DESTDIR   ?=
BINDIR    ?= $(PREFIX)/bin
DATAROOTDIR ?= $(PREFIX)/share
MANDIR    ?= $(DATAROOTDIR)/man
MAN6DIR   ?= $(MANDIR)/man6
DOCDIR    ?= $(DATAROOTDIR)/doc/bayeishuui
INSTALL        ?= install
INSTALL_PROGRAM?= $(INSTALL) -m 0755
INSTALL_DATA   ?= $(INSTALL) -m 0644

# ---- Default: build both native implementations ---------------------------
all: build-c build-cpp

# ---- Build ----------------------------------------------------------------
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

build-c: $(C_GAME)
$(C_GAME): $(SRCDIR)/bayeishuui.c $(HEADERS) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(SRCDIR)/bayeishuui.c -o $@ $(LDLIBS_C)

build-cpp: $(CPP_GAME)
$(CPP_GAME): $(SRCDIR)/bayeishuui.cpp $(HEADERS) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(SRCDIR)/bayeishuui.cpp -o $@

# Pixel-accurate renderer demo (Kitty / Sixel / glyph).
demo: $(DEMO)
$(DEMO): $(SRCDIR)/bayeishuui_pixeldemo.c $(HEADERS) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(SRCDIR)/bayeishuui_pixeldemo.c -o $@ $(LDLIBS_C)

run-demo: demo
	./$(DEMO)

# SLeeLa character sheet reader/interpreter/disposer demos.
build-sheet-c: $(SHEET_C)
$(SHEET_C): $(SRCDIR)/fighter_sheet_demo.c $(SRCDIR)/fighter_sheet.h | $(BUILDDIR)
	$(CC) $(CFLAGS) $(SRCDIR)/fighter_sheet_demo.c -o $@

build-sheet-cpp: $(SHEET_CPP)
$(SHEET_CPP): $(SRCDIR)/fighter_sheet_demo.cpp $(SRCDIR)/fighter_sheet.h | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(SRCDIR)/fighter_sheet_demo.cpp -o $@

run-sheet-c: build-sheet-c
	./$(SHEET_C) $(SAMPLE_SHEET)

run-sheet-cpp: build-sheet-cpp
	./$(SHEET_CPP) $(SAMPLE_SHEET)

# ---- Run ------------------------------------------------------------------
run:
	$(PYTHON) $(PY_GAME)

run-c: build-c
	./$(C_GAME)

run-cpp: build-cpp
	./$(CPP_GAME)

# Generate a Player 1 AI run config in the current working directory.
run-ai: run-cpp

# ---- Checks ---------------------------------------------------------------
check:
	$(PYTHON) -m py_compile $(PY_GAME) $(AI_PLAYER)
	$(MAKE) build-c
	$(MAKE) build-cpp
	$(MAKE) demo
	$(MAKE) build-sheet-c
	$(MAKE) build-sheet-cpp

# ---- Install --------------------------------------------------------------
# `install` does both; install-c / install-cpp do one implementation each.
install: install-c install-cpp
	$(INSTALL) -d $(DESTDIR)$(DOCDIR)
	$(INSTALL_DATA) $(DOCS) $(DESTDIR)$(DOCDIR)

install-c: build-c
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL_PROGRAM) $(C_GAME) $(DESTDIR)$(BINDIR)/$(C_BIN)
	$(INSTALL) -d $(DESTDIR)$(MAN6DIR)
	$(INSTALL_DATA) $(C_MAN) $(DESTDIR)$(MAN6DIR)/$(C_BIN).6

install-cpp: build-cpp
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL_PROGRAM) $(CPP_GAME) $(DESTDIR)$(BINDIR)/$(CPP_BIN)
	$(INSTALL) -d $(DESTDIR)$(MAN6DIR)
	$(INSTALL_DATA) $(CPP_MAN) $(DESTDIR)$(MAN6DIR)/$(CPP_BIN).6

# ---- Uninstall ------------------------------------------------------------
uninstall: uninstall-c uninstall-cpp
	rm -f  $(addprefix $(DESTDIR)$(DOCDIR)/,$(notdir $(DOCS)))
	-rmdir $(DESTDIR)$(DOCDIR) 2>/dev/null || true

uninstall-c:
	rm -f $(DESTDIR)$(BINDIR)/$(C_BIN)
	rm -f $(DESTDIR)$(MAN6DIR)/$(C_BIN).6

uninstall-cpp:
	rm -f $(DESTDIR)$(BINDIR)/$(CPP_BIN)
	rm -f $(DESTDIR)$(MAN6DIR)/$(CPP_BIN).6

# ---- Clean ----------------------------------------------------------------
clean:
	rm -rf $(BUILDDIR)
	find . -type d -name __pycache__ -prune -exec rm -rf {} +

# ---- Help -----------------------------------------------------------------
help:
	@echo "Bayeishuui $(VERSION) — targets:"
	@echo "  make / make all        Build both native binaries"
	@echo "  build-c build-cpp      Build one implementation"
	@echo "  run run-c run-cpp      Run Python / C / C++ games"
	@echo "  demo run-demo          Build/run the pixel-accurate renderer demo"
	@echo "  build-sheet-c/-cpp     Build the SLeeLa character-sheet demos"
	@echo "  run-sheet-c/-cpp       Run the sheet reader on the sample fighters"
	@echo "  check                  py_compile + build games + demo + sheets"
	@echo "  install                Install both binaries, man pages, and docs"
	@echo "  install-c/-cpp         Install a single implementation + its man page"
	@echo "  uninstall[-c/-cpp]     Remove installed files"
	@echo "  clean                  Remove build/ and __pycache__"
	@echo "Vars: PREFIX=$(PREFIX) BINDIR=$(BINDIR) MANDIR=$(MANDIR) DOCDIR=$(DOCDIR) DESTDIR="
