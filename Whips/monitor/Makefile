# Makefile for FlappyRender shared library
# Used by Python visualizer via ctypes

UNAME := $(shell uname)

SRC_DIR = ../src
BUILD_DIR = build

ifeq ($(UNAME), Darwin)
    # macOS
    TARGET = $(BUILD_DIR)/libFlappyRender.dylib
    CXXFLAGS = -dynamiclib -fPIC
else
    # Linux
    TARGET = $(BUILD_DIR)/libFlappyRender.so
    CXXFLAGS = -shared -fPIC
endif

CXXFLAGS += -O2 -std=c++11

SOURCES = $(SRC_DIR)/FlappyRender.cpp
HEADERS = $(SRC_DIR)/FlappyRender.h

.PHONY: all clean

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(SOURCES) $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES)

clean:
	rm -rf $(BUILD_DIR)
