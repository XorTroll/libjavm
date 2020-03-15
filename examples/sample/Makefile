CXX      := g++
CXXFLAGS := -std=gnu++17
LDFLAGS  := -L/usr/lib -lstdc++ -lm -pthread
BUILD    := ./build
OBJ_DIR  := $(BUILD)/obj
OUT_DIR  := $(BUILD)/bin
TARGET   := $(notdir $(CURDIR))
INCLUDE  := -I$(CURDIR)/../../libjavm/include/
SRC      :=                      \
   $(wildcard src/*.cpp)         \

OBJECTS  := $(SRC:%.cpp=$(OBJ_DIR)/%.o)

all: build $(OUT_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	@echo $<
	@$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@ $(LDFLAGS)

$(OUT_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) -o $(OUT_DIR)/$(TARGET) $^ $(LDFLAGS)
	@echo built -> $(OUT_DIR)/$(TARGET)

.PHONY: all build clean debug release

build:
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OBJ_DIR)

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -O2
release: all

clean:
	@rm -rvf $(OBJ_DIR)/*
	@rm -rvf $(OUT_DIR)/*