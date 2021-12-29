CXX := g++
CXX_FLAGS := -std=gnu++17 -O3
LD_FLAGS := -lm -pthread
BUILD := $(CURDIR)/build
OBJ_DIR := $(BUILD)/obj
OUT_DIR := $(BUILD)/bin
TARGET := $(notdir $(CURDIR))
INCLUDE := -I$(CURDIR)/../../libjavm/include/
SRC :=	$(shell find $(CURDIR)/src/ -type f -name '*.cpp') $(shell find $(CURDIR)/../../libjavm/src/ -type f -name '*.cpp')

OBJECTS := $(SRC:%.cpp=$(OBJ_DIR)/%.o)

all: build $(OUT_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	@echo $<
	@$(CXX) $(CXX_FLAGS) $(INCLUDE) -c $< -o $@

$(OUT_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) -o $(OUT_DIR)/$(TARGET) $^ $(LD_FLAGS)
	@echo built - $(OUT_DIR)/$(TARGET)

.PHONY: all build clean

build:
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OBJ_DIR)

clean:
	@rm -rf $(BUILD)/