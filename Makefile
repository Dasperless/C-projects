BUILD_DIR ?= build
CMAKE ?= cmake
CTEST ?= ctest

.PHONY: all build debug asan test clean help

all: build

build:
	@mkdir -p $(BUILD_DIR)
	@$(CMAKE) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	@$(CMAKE) --build $(BUILD_DIR)

debug:
	@mkdir -p $(BUILD_DIR)
	@$(CMAKE) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	@$(CMAKE) --build $(BUILD_DIR)

asan:
	@mkdir -p $(BUILD_DIR)
	@$(CMAKE) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -DENABLE_UBSAN=ON
	@$(CMAKE) --build $(BUILD_DIR)

test: build
	@cd $(BUILD_DIR) && $(CTEST) --output-on-failure

clean:
	@rm -rf $(BUILD_DIR) bin lib

help:
	@echo "Available targets:"
	@echo "  all    : Build the project in Release mode (default)"
	@echo "  build  : Build the project in Release mode"
	@echo "  debug  : Build the project in Debug mode"
	@echo "  asan   : Build the project with AddressSanitizer and UBSan"
	@echo "  test   : Run CTest test suite"
	@echo "  clean  : Remove build artifacts and directories"
	@echo "  help   : Display this help message"
