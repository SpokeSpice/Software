INPUT_DIR ?= ../../Artworks/playlist
OUTPUT_DIR ?= ../Firmware/spiffs_data

INPUT_FILES := $(wildcard $(INPUT_DIR)/*.gif)
OUTPUT_FILES := $(patsubst $(INPUT_DIR)/%.gif,$(OUTPUT_DIR)/%.rgif,$(INPUT_FILES))

all: $(OUTPUT_FILES)

clean:
	rm -f $(OUTPUT_DIR)/*.rgif

$(OUTPUT_DIR)%.rgif: $(INPUT_DIR)%.gif
	@mkdir -p $(OUTPUT_DIR)
	@echo "Processing $<..."
	@python3 convert.py $< $(patsubst %.rgif,%.gif,$@)
	@mv $(patsubst %.rgif,%.gif,$@) $@
