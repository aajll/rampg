# Makefile for generating rampg documentation using Doxygen and Sphinx/Breathe
#
# Python dependencies are managed via uv using docs/pyproject.toml.
# Run from the repository root or from docs/sphinx/.
#
#   make -C docs/sphinx html

# Project root
PROJECT_ROOT := $(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST))))../..)

# Directories
DOCS_DIR := $(PROJECT_ROOT)/docs
SPHINX_DIR := $(DOCS_DIR)/sphinx
BUILD_DIR := $(DOCS_DIR)/_build
DOXYGEN_OUTPUT := $(BUILD_DIR)/doxygen
SPHINX_OUTPUT := $(BUILD_DIR)/html

# Configuration files
DOXYFILE := $(SPHINX_DIR)/Doxyfile

# uv runs sphinx-build inside the venv defined by docs/pyproject.toml
SPHINX_BUILD := uv run --project $(DOCS_DIR) sphinx-build

# Colors for terminal output
BLUE := \033[1;34m
GREEN := \033[1;32m
YELLOW := \033[1;33m
RESET := \033[0m

# Default target
.PHONY: all
all: html

# Clean build directory
.PHONY: clean
clean:
	@echo "$(YELLOW)Cleaning build directory...$(RESET)"
	rm -rf $(BUILD_DIR)
	@echo "$(GREEN)Done!$(RESET)"

# Generate Doxygen XML output
.PHONY: doxygen
doxygen:
	@echo "$(BLUE)Generating Doxygen XML output...$(RESET)"
	mkdir -p $(DOXYGEN_OUTPUT)
	doxygen $(DOXYFILE)
	@echo "$(GREEN)Doxygen XML generated in $(DOXYGEN_OUTPUT)/xml$(RESET)"

# Generate Sphinx HTML documentation
.PHONY: html
html: doxygen
	@echo "$(BLUE)Generating Sphinx HTML documentation...$(RESET)"
	mkdir -p $(SPHINX_OUTPUT)
	$(SPHINX_BUILD) -b html -D breathe_projects.rampg="$(DOXYGEN_OUTPUT)/xml" $(SPHINX_DIR) $(SPHINX_OUTPUT)
	@echo "$(GREEN)HTML documentation generated in $(SPHINX_OUTPUT)$(RESET)"

# Generate Sphinx LaTeX documentation
.PHONY: latex
latex: doxygen
	@echo "$(BLUE)Generating Sphinx LaTeX documentation...$(RESET)"
	mkdir -p $(BUILD_DIR)/latex
	$(SPHINX_BUILD) -b latex -D breathe_projects.rampg="$(DOXYGEN_OUTPUT)/xml" $(SPHINX_DIR) $(BUILD_DIR)/latex
	@echo "$(GREEN)LaTeX files generated in $(BUILD_DIR)/latex$(RESET)"

# Build PDF from LaTeX
.PHONY: pdf
pdf: latex
	@echo "$(BLUE)Building PDF from LaTeX...$(RESET)"
	cd $(BUILD_DIR)/latex && make
	@echo "$(GREEN)PDF generated in $(BUILD_DIR)/latex$(RESET)"

# View the generated HTML documentation
.PHONY: view
view: html
	@echo "$(GREEN)Opening HTML documentation in browser...$(RESET)"
	xdg-open $(SPHINX_OUTPUT)/index.html

# Check if dependencies are installed
.PHONY: check-deps
check-deps:
	@echo "$(BLUE)Checking dependencies...$(RESET)"
	@command -v doxygen >/dev/null || (echo "$(YELLOW)Doxygen not found. Install with: sudo pacman -S doxygen$(RESET)" && exit 1)
	@command -v uv >/dev/null || (echo "$(YELLOW)uv not found. See: https://docs.astral.sh/uv/$(RESET)" && exit 1)
	@echo "$(GREEN)All dependencies satisfied!$(RESET)"

# Help target
.PHONY: help
help:
	@echo "$(GREEN)rampg Documentation Makefile$(RESET)"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Generate HTML documentation (default)"
	@echo "  doxygen    - Generate Doxygen XML output"
	@echo "  html       - Generate Sphinx HTML documentation"
	@echo "  latex      - Generate Sphinx LaTeX documentation"
	@echo "  pdf        - Build PDF from LaTeX"
	@echo "  view       - Generate HTML and open in browser"
	@echo "  clean      - Remove build directory"
	@echo "  check-deps - Check if all dependencies are installed"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Python dependencies (sphinx, breathe) are managed via uv."
	@echo "Run 'uv sync --project docs' to install them manually."
