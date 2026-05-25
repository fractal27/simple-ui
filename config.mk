# config.mk - SimpleUI build configuration

CC := gcc
AR := ar

CFLAGS := -Wall -Wextra -Iinclude
DEBUG_CFLAGS := -ggdb -fPIC -fsanitize=address
OPTIMIZE_CFLAGS := -O2 -march=native -mtune=native

ifeq ($(DEBUG),1)
CFLAGS += $(DEBUG_CFLAGS)
else
CFLAGS += $(OPTIMIZE_CFLAGS)
endif

PREFIX ?= /usr/local
INSTALL_LIB := $(PREFIX)/lib
INSTALL_INC := $(PREFIX)/include
