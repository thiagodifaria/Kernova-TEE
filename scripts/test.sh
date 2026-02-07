#!/bin/bash
# =============================================================================
# Micro-Hypervisor - Automated Test Script
# =============================================================================
# Roda todos os testes e gera relatório
# =============================================================================

set -e

# Cores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[*]${NC} $1"; }
log_success() { echo -e "${GREEN}[+]${NC} $1"; }
log_error() { echo -e "${RED}[-]${NC} $1"; }

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Array para armazenar resultados
declare -a TEST_RESULTS

# Executar teste e registrar resultado
run_test() {
    local test_name="$1"
    local test_command="$2"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    log_info "Rodando: $test_name"

    if eval "$test_command" > /tmp/test_output.log 2>&1; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
        log_success "PASS: $test_name"
        TEST_RESULTS+=("✓ $test_name")
        return 0
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
        log_error "FAIL: $test_name"
        TEST_RESULTS+=("✗ $test_name")
        cat /tmp/test_output.log | head -20
        return 1
    fi
}

# Teste 1: Verificar se o binário existe
test_binary_exists() {
    [ -f "build/MicroHypervisor" ] || [ -f "MicroHypervisor" ]
}

# Teste 2: Verificar dependências
test_dependencies() {
    command -v gcc && command -v nasm && command -v cmake
}

# Teste 3: Verificar estrutura do projeto
test_project_structure() {
    [ -d "src" ] && [ -d "include" ] && [ -d "tests" ]
}

# Teste 4: Verificar arquivos de código
test_source_files() {
    [ -f "src/main.cpp" ] && \
    [ -f "src/core/vmx_init.asm" ] && \
    [ -f "include/registers.hpp" ]
}

# Teste 5: Compilar teste SIMD
test_build_simd_test() {
    cd build
    if [ -f "tests/test_simd" ]; then
        return 0
    fi

    g++ -std=c++20 -O2 -I../include \
        ../tests/test_simd.cpp \
        -o tests/test_simd \
        -nostdlib 2>/dev/null || \
    g++ -std=c++20 -O2 -I../include \
        ../tests/test_simd.cpp \
        -o tests/test_simd 2>/dev/null
}

# Teste 6: Rodar teste SIMD (se compilado)
test_run_simd() {
    [ -f "build/tests/test_simd" ] && build/tests/test_simd
}

# Teste 7: Verificar sintaxe Assembly
test_assembly_syntax() {
    nasm -f elf64 src/core/vmx_init.asm -o /tmp/test.o 2>&1
    rm -f /tmp/test.o
}

# Teste 8: Verificar sintaxe C++
test_cpp_syntax() {
    g++ -std=c++20 -fsyntax-only -Iinclude src/main.cpp 2>&1
}

# Teste 9: Verificar arquitetura
test_architecture() {
    uname -m | grep -q "x86_64"
}

# Teste 10: Verificar VT-x (apenas detecta, não requer)
test_vtx_support() {
    if [ -f /proc/cpuinfo ]; then
        grep -q "vmx" /proc/cpuinfo 2>/dev/null
    else
        # No macOS ou outros, assume x86_64
        return 0
    fi
}

# Teste 11: Verificar AVX support
test_avx_support() {
    if [ -f /proc/cpuinfo ]; then
        grep -q "avx2" /proc/cpuinfo 2>/dev/null || grep -q "avx" /proc/cpuinfo
    else
        # Assume support se x86_64
        return 0
    fi
}

# Teste 12: Verificar integridade dos arquivos
test_file_integrity() {
    # Verificar se os arquivos principais não estão vazios
    [ -s "src/main.cpp" ] && [ -s "src/core/vmx_init.asm" ] && [ -s "CMakeLists.txt" ]
}

# Main
main() {
    echo ""
    echo "========================================"
    echo "  Micro-Hypervisor Test Suite"
    echo "========================================"
    echo ""

    cd "$(dirname "$0")/.."

    log_info "Iniciando testes..."
    echo ""

    # Rodar todos os testes
    run_test "Binary exists" "test_binary_exists"
    run_test "Dependencies installed" "test_dependencies"
    run_test "Project structure" "test_project_structure"
    run_test "Source files present" "test_source_files"
    run_test "Assembly syntax" "test_assembly_syntax"
    run_test "C++ syntax" "test_cpp_syntax"
    run_test "Architecture check" "test_architecture"
    run_test "VT-x support" "test_vtx_support"
    run_test "AVX support" "test_avx_support"
    run_test "File integrity" "test_file_integrity"

    # Testes opcionais (requer build completo)
    if [ -d "build" ]; then
        run_test "Build SIMD test" "test_build_simd_test"

        if [ -f "build/tests/test_simd" ]; then
            run_test "Run SIMD test" "test_run_simd"
        fi
    fi

    # Relatório final
    echo ""
    echo "========================================"
    echo "  Test Results Summary"
    echo "========================================"
    echo ""

    for result in "${TEST_RESULTS[@]}"; do
        echo "  $result"
    done

    echo ""
    echo "  Total:  $TOTAL_TESTS"
    echo -e "  ${GREEN}Passed:${NC} $PASSED_TESTS"
    echo -e "  ${RED}Failed:${NC} $FAILED_TESTS"
    echo ""

    if [ $FAILED_TESTS -eq 0 ]; then
        log_success "All tests passed!"
        echo ""
        return 0
    else
        log_error "Some tests failed!"
        echo ""
        return 1
    fi
}

main "$@"
