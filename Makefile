.PHONY: run check clean

PYTHON ?= python3
GAME := inventory/bayeshuui.py

# Run the preserved terminal-adventure prototype.
run:
	$(PYTHON) $(GAME)

# Validate Python syntax without launching the interactive game.
check:
	$(PYTHON) -m py_compile $(GAME)

clean:
	find . -type d -name __pycache__ -prune -exec rm -rf {} +
