TARGET = populate.elf

CC = gcc
CFLAGS = -I../../../src/include

TOOLS_DIR = ../../../build/tools
OUT_DIR = $(TOOLS_DIR)/populate/

$(TARGET): populate.c
	@mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) $^ -o $(OUT_DIR)/$@

run: $(TARGET)
	$(OUT_DIR)/$^  ../../../build/img/kernel.img -g
