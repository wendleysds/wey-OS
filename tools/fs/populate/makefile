TARGET = populate.elf

CC = gcc
CFLAGS =

TOOLS_DIR = ../../../build/tools
OUT_DIR = $(TOOLS_DIR)/

$(TARGET): populate.c
	@mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) $^ -o $(OUT_DIR)/$@

run: $(TARGET)
	$(OUT_DIR)/$^  ../../../build/img/kernel.img -g
