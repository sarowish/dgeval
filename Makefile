CXX = clang++
CXXFLAGS = -std=c++23 -g -O2
CPPFLAGS = -MMD -MP

SRC_DIR = src
BUILD_DIR = build

EXE = $(BUILD_DIR)/project4
SRC = $(wildcard $(SRC_DIR)/*.cpp) $(SRC_DIR)/parser.cpp $(SRC_DIR)/scanner.cpp
OBJ = $(SRC:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

JOBS ?= $(shell nproc)
MAKEFLAGS += -j $(JOBS) -l $(JOBS)

.PHONY: clean clean_output

$(EXE): $(OBJ) | $(BUILD_DIR)
	$(CXX) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(SRC_DIR)/parser.cpp $(SRC_DIR)/parser.hpp $(SRC_DIR)/location.hpp: $(SRC_DIR)/parser.yy
	bison -o $(SRC_DIR)/parser.cpp $^

$(SRC_DIR)/scanner.cpp: $(SRC_DIR)/scanner.ll $(SRC_DIR)/scanner.hpp
	flex -o $@ $<

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -r $(BUILD_DIR) $(SRC_DIR)/{scanner.cpp,parser.{cpp,hpp},location.hpp}

clean_output:
	rm *.json *-IC.txt

-include $(OBJ:.o=.d)
