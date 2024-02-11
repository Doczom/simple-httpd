OUTPUT_DIR=bin
OUTPUT_MODULES=$(OUTPUT_DIR)/modules

examples=$(wildcard example/*.asm)
examples_obj=$(patsubst %.asm,%.obj,$(examples))
 

all: build-main build-mime $(examples_obj)
	

build-main:
	fasm httpd.asm $(OUTPUT_DIR)/httpd

build-mime:
	fasm utils/mime_types.asm $(OUTPUT_DIR)/mime_types.bin

example/%.obj: example/%.asm
	fasm $< $(OUTPUT_MODULES)/$(notdir $(basename $@)).obj

