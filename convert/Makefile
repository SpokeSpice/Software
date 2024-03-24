INPUT_DIR ?= ../../Artworks/playlist
OUTPUT_DIR ?= ../Firmware/spiffs_data

INPUT_FILES := $(wildcard $(INPUT_DIR)/*.gif)
OUTPUT_FILES := $(patsubst $(INPUT_DIR)/%.gif,$(OUTPUT_DIR)/%.rgif,$(INPUT_FILES))

BINARY=target/debug/rgif-convert

all: binary $(OUTPUT_FILES)

clean:
	rm -f $(OUTPUT_DIR)/*.rgif

binary: $(BINARY)
	cargo build

$(OUTPUT_DIR)%.rgif: $(INPUT_DIR)%.gif
	@mkdir -p $(OUTPUT_DIR)
	@echo "Processing $<..."
	$(BINARY) $< $(patsubst %.rgif,%.gif,$@)
	@mv $(patsubst %.rgif,%.gif,$@) $@
