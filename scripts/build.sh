#!/bin/bash
# =============================================================================
# Kernova-TEE - Automated Build Script
# =============================================================================
# Instala dependências e compila o projeto automaticamente
# =============================================================================

set -e

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Funções de logging
log_info() {
    echo -e "${BLUE}[*]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[+]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

log_error() {
    echo -e "${RED}[-]${NC} $1"
}

# Detectar SO
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        OS="linux"
        if [ -f /etc/debian_version ]; then
            DISTRO="debian"
        elif [ -f /etc/redhat-release ]; then
            DISTRO="redhat"
        else
            DISTRO="unknown"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"
        DISTRO="macos"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        OS="windows"
        DISTRO="windows"
    else
        OS="unknown"
        DISTRO="unknown"
    fi

    log_info "Detectado: $OS ($DISTRO)"
}

# Verificar dependências
check_dependencies() {
    log_info "Verificando dependências..."

    local missing_deps=()

    # Verificar comandos essenciais
    for cmd in gcc g++ nasm cmake make; do
        if ! command -v $cmd &> /dev/null; then
            missing_deps+=($cmd)
        fi
    done

    if [ ${#missing_deps[@]} -gt 0 ]; then
        log_warning "Dependências faltando: ${missing_deps[*]}"
        return 1
    fi

    log_success "Todas as dependências estão instaladas"
    return 0
}

# Instalar dependências automaticamente
install_dependencies() {
    log_info "Instalando dependências..."

    if [ "$OS" == "linux" ]; then
        if [ "$DISTRO" == "debian" ]; then
            sudo apt-get update
            sudo apt-get install -y build-essential cmake nasm qemu-system-x86 gdb python3
        elif [ "$DISTRO" == "redhat" ]; then
            sudo dnf install -y gcc-c++ cmake nasm qemu-system-x86 gdb python3
        fi
    elif [ "$OS" == "macos" ]; then
        if ! command -v brew &> /dev/null; then
            log_error "Homebrew não encontrado. Instale em https://brew.sh"
            exit 1
        fi
        brew install cmake nasm qemu
    elif [ "$OS" == "windows" ]; then
        log_warning "No Windows: Instale MSYS2 ou use WSL"
        log_info "Recomendado: Execute este script no WSL (Ubuntu)"
        return 1
    fi

    log_success "Dependências instaladas"
}

# Limpar build anterior
clean_build() {
    log_info "Limpando build anterior..."

    if [ -d "build" ]; then
        rm -rf build
        log_success "Build limpo"
    fi
}

# Compilar com CMake
build_cmake() {
    log_info "Compilando com CMake..."

    mkdir -p build
    cd build

    # Configurar
    log_info "Configurando CMake..."
    cmake .. -DCMAKE_BUILD_TYPE=Release

    # Compilar
    log_info "Compilando..."
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

    cd ..
    log_success "Build completo com CMake"
}

# Compilar com Make (alternativo)
build_make() {
    log_info "Compilando com Make..."

    make clean 2>/dev/null || true
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

    log_success "Build completo com Make"
}

# Verificar resultado da compilação
verify_build() {
    log_info "Verificando build..."

    if [ -f "build/Kernova" ]; then
        local size=$(du -h build/Kernova | cut -f1)
        log_success "Binário criado: $size"
        return 0
    elif [ -f "Kernova" ]; then
        local size=$(du -h Kernova | cut -f1)
        log_success "Binário criado: $size"
        return 0
    else
        log_error "Binário não encontrado"
        return 1
    fi
}

# Rodar testes
run_tests() {
    log_info "Rodando testes..."

    if [ -f "scripts/test.sh" ]; then
        ./scripts/test.sh
    else
        log_warning "Script de testes não encontrado"
    fi
}

# Gerar documentação
generate_docs() {
    log_info "Gerando documentação..."

    if command -v doxygen &> /dev/null; then
        doxygen docs/Doxyfile 2>/dev/null || log_warning "Doxyfile não encontrado"
    else
        log_warning "Doxygen não instalado"
    fi
}

# Criar package
create_package() {
    log_info "Criando package..."

    local version=$(grep "project(Kernova VERSION" CMakeLists.txt | grep -oP '\d+\.\d+\.\d+')
    local package_name="Kernova-${version}-$(uname -m)"

    mkdir -p dist
    tar -czf "dist/${package_name}.tar.gz" \
        --exclude='build' \
        --exclude='.git' \
        --exclude='dist' \
        . 2>/dev/null || true

    log_success "Package criado: dist/${package_name}.tar.gz"
}

# Menu de help
show_help() {
    cat << EOF
Kernova-TEE - Build Script

Uso: $0 [OPÇÕES]

Opções:
    -h, --help          Mostra esta mensagem
    -c, --clean         Limpa build anterior
    -d, --deps          Instala dependências
    -b, --build         Compila o projeto
    -t, --test          Roda testes após build
    -p, --package       Cria package tar.gz
    -a, --all           Faz tudo (clean + deps + build + test)
    --docker            Build dentro do Docker

Exemplos:
    $0 --all                    # Build completo
    $0 --build --test           # Build e testes
    $0 --clean --build          # Rebuild

EOF
}

# Main
main() {
    echo ""
    echo "========================================"
    echo "  Kernova-TEE Build Script"
    echo "========================================"
    echo ""

    # Default flags
    DO_CLEAN=false
    DO_DEPS=false
    DO_BUILD=false
    DO_TEST=false
    DO_PACKAGE=false
    DO_ALL=false

    # Parse argumentos
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                DO_CLEAN=true
                shift
                ;;
            -d|--deps)
                DO_DEPS=true
                shift
                ;;
            -b|--build)
                DO_BUILD=true
                shift
                ;;
            -t|--test)
                DO_TEST=true
                shift
                ;;
            -p|--package)
                DO_PACKAGE=true
                shift
                ;;
            -a|--all)
                DO_ALL=true
                shift
                ;;
            *)
                log_error "Opção desconhecida: $1"
                show_help
                exit 1
                ;;
        esac
    done

    # Se nenhum argumento, mostra help
    if [ "$DO_ALL" = false ] && [ "$DO_CLEAN" = false ] && [ "$DO_DEPS" = false ] && [ "$DO_BUILD" = false ] && [ "$DO_TEST" = false ] && [ "$DO_PACKAGE" = false ]; then
        show_help
        exit 0
    fi

    # Se --all, ativar tudo
    if [ "$DO_ALL" = true ]; then
        DO_CLEAN=true
        DO_DEPS=true
        DO_BUILD=true
        DO_TEST=true
        DO_PACKAGE=true
    fi

    # Detectar SO
    detect_os

    # Executar passos
    if [ "$DO_CLEAN" = true ]; then
        clean_build
    fi

    if [ "$DO_DEPS" = true ]; then
        if ! check_dependencies; then
            install_dependencies
        fi
    fi

    if [ "$DO_BUILD" = true ]; then
        # Preferir CMake se disponível
        if command -v cmake &> /dev/null; then
            build_cmake
        else
            build_make
        fi

        if ! verify_build; then
            log_error "Build falhou"
            exit 1
        fi
    fi

    if [ "$DO_TEST" = true ]; then
        run_tests
    fi

    if [ "$DO_PACKAGE" = true ]; then
        create_package
    fi

    echo ""
    echo "========================================"
    log_success "Build concluído com sucesso!"
    echo "========================================"
    echo ""
}

# Executar main
main "$@"
