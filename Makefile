## EPITECH PROJECT, 2025
## Makefile
## File description:
## it makes files
##

BUILD_TYPE ?= Debug
BUILD_DIR := build/$(BUILD_TYPE)
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

# Règles principales
all: $(BUILD_DIR)/Makefile
	@$(MAKE) -C $(BUILD_DIR)

client: $(BUILD_DIR)/Makefile
	@$(MAKE) -C $(BUILD_DIR) jetpack_client

server: $(BUILD_DIR)/Makefile
	@$(MAKE) -C $(BUILD_DIR) jetpack_server

$(BUILD_DIR)/Makefile: CMakeLists.txt | $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) $(CURDIR)

clean:
	@$(MAKE) -C $(BUILD_DIR) clean 2>/dev/null || true

fclean: clean
	@rm -rf build bin

re: fclean all

# Cibles pour reconfigurer en mode Debug ou Release
debug:
	$(MAKE) BUILD_TYPE=Debug re

release:
	$(MAKE) BUILD_TYPE=Release re

.PHONY: all client server clean fclean re debug release

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
