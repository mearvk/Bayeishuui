.PHONY: run check clean

PYTHON ?= python3

run:
	$(PYTHON) bayeshuui.py

check:
	$(PYTHON) -m py_compile bayeshuui.py

clean:
	find . -type d -name __pycache__ -prune -exec rm -rf {} +
