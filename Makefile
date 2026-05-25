.PHONY: all clean install uninstall example example_x11 example_win debug

include config.mk

SRC_DIR := src
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)

SRCS := $(SRC_DIR)/simpleui.c

ifeq ($(OS),Windows_NT)
    SRCS += $(SRC_DIR)/simpleui_win32.c
else
    SRCS += $(SRC_DIR)/simpleui_x11.c
endif

OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET := $(BUILD_DIR)/libsimpleui.a

all: $(TARGET) example

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

example: $(TARGET)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/simpleui_example $(SRC_DIR)/simpleui_example.c $(TARGET) -lX11 -lm

example_win: $(TARGET)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/simpleui_example_win32.exe $(SRC_DIR)/simpleui_example_win32.c $(TARGET) -luser32 -lgdi32 -lwinmm

debug:
	$(MAKE) -Bj DEBUG=1

clean:
	rm -rf $(BUILD_DIR)

install: $(TARGET)
	cp $(TARGET) $(INSTALL_LIB)/libsimpleui.a
	cp include/simpleui.h $(INSTALL_INC)/simpleui.h

uninstall:
	rm -f $(INSTALL_LIB)/libsimpleui.a
	rm -f $(INSTALL_INC)/simpleui.h
