# config.mk - SimpleUI build configuration

CC := gcc
AR := ar

CFLAGS := -Wall -Wextra -Iinclude

PREFIX ?= /usr/local
INSTALL_LIB := $(PREFIX)/lib
INSTALL_INC := $(PREFIX)/include