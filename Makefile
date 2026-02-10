# =============================================================================
# Kernova-TEE Makefile
# =============================================================================
# Simplificado para compilação rápida sem CMake
# =============================================================================

# Configurações
ASSEMBLER = nasm
CXX = g++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra -ffreestanding -nostdlib -fno-exceptions -fno-rtti
ASFLAGS = -f elf64
LDFLAGS = -nostdlib -static -T linker.ld

# Diretórios
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include
TEST_DIR = tests

# Arquivos Assembly
ASM_SOURCES = $(SRC_DIR)/boot/entry.asm \
              $(SRC_DIR)/core/vmx_init.asm \
              $(SRC_DIR)/core/vmcs_config.asm \
              $(SRC_DIR)/marshall/simd_packer.asm \
              $(SRC_DIR)/monitor/intercept.asm \
              $(SRC_DIR)/monitor/trace_engine.asm

# Arquivos C++
CPP_SOURCES = $(SRC_DIR)/main.cpp

# Arquivos objeto
ASM_OBJECTS = $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
CPP_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))

# Executável
TARGET = $(BUILD_DIR)/Kernova

# Phony targets
.PHONY: all clean run test dirs

# Default target
all: dirs $(TARGET)

# Criar diretórios
dirs:
	@mkdir -p $(BUILD_DIR)/boot
	@mkdir -p $(BUILD_DIR)/core
	@mkdir -p $(BUILD_DIR)/marshall
	@mkdir -p $(BUILD_DIR)/monitor
	@mkdir -p $(BUILD_DIR)/tests

# Compilar arquivos Assembly
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@echo "[NASM] $<"
	@$(ASSEMBLER) $(ASFLAGS) -I $(INCLUDE_DIR)/ $< -o $@

# Compilar arquivos C++
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) -I $(INCLUDE_DIR) -c $< -o $@

# Linkar
$(TARGET): $(ASM_OBJECTS) $(CPP_OBJECTS)
	@echo "[LD] $@"
	@$(CXX) $(LDFLAGS) -o $@ $^
	@echo "[+] Build completo: $@"

# Limpar
clean:
	@echo "[*] Limpando arquivos de build..."
	@rm -rf $(BUILD_DIR)
	@echo "[+] Limpeza completa"

# Rodar no QEMU
run: $(TARGET)
	@echo "[*] Rodando no QEMU..."
	qemu-system-x86_64 -enable-kvm -m 2G -nographic -kernel $(TARGET) -serial mon:stdio || \
	qemu-system-x86_64 -m 2G -nographic -kernel $(TARGET) -serial mon:stdio

# Debug no QEMU
debug: $(TARGET)
	@echo "[*] Rodando no QEMU com GDB..."
	qemu-system-x86_64 -enable-kvm -m 2G -nographic -kernel $(TARGET) -s -S

# Compilar testes
test: dirs
	@echo "[*] Compilando testes..."
	@$(CXX) $(CXXFLAGS) -I $(INCLUDE_DIR) -o $(BUILD_DIR)/tests/test_simd $(TEST_DIR)/test_simd.cpp $(SRC_DIR)/marshall/simd_packer.asm $(ASFLAGS)
	@echo "[+] Testes compilados"

# Rodar testes
test-run: test
	@echo "[*] Rodando testes..."
	@$(BUILD_DIR)/tests/test_simd

# Ajuda
help:
	@echo "Kernova-TEE Makefile"
	@echo ""
	@echo "Targets disponíveis:"
	@echo "  all       - Compila o projeto (default)"
	@echo "  clean     - Remove arquivos de build"
	@echo "  run       - Roda no QEMU"
	@echo "  debug     - Roda no QEMU com GDB"
	@echo "  test      - Compila testes"
	@echo "  test-run  - Roda os testes"
	@echo "  help      - Mostra esta mensagem"
