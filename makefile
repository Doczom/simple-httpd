OUTPUT_DIR=bin
OUTPUT_MODULES=$(OUTPUT_DIR)/modules

examples=$(wildcard example/*.asm)
examples_obj=$(patsubst %.asm,%.obj,$(examples))
 

all: build-main build-utils $(examples_obj) copy_docs
	

build-main:
	fasm httpd.asm $(OUTPUT_DIR)/httpd

build-utils:
	fasm utils/mime_types.asm $(OUTPUT_DIR)/mime_types.bin
	fasm utils/redirect.asm $(OUTPUT_MODULES)/redirect.obj
	fasm utils/block_access.asm $(OUTPUT_MODULES)/block_access.obj

example/%.obj: example/%.asm
	fasm $< $(OUTPUT_MODULES)/$(notdir $(basename $@)).obj

copy_docs:
	cp -R doc/* $(OUTPUT_DIR)/server_data/docs