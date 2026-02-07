#!/usr/bin/env python3
"""
Micro-Hypervisor CLI
Interface de linha de comando para gerenciar o projeto
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path
from typing import List, Tuple

class Colors:
    """Cores para terminal"""
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    PURPLE = '\033[0;35m'
    CYAN = '\033[0;36m'
    NC = '\033[0m'  # No Color

def print_banner():
    """Imprime banner do projeto"""
    banner = f"""
    {Colors.CYAN}╔════════════════════════════════════════╗
    ║     Micro-Hypervisor v1.0           ║
    ║  Trusted Execution Environment       ║
    ║                                      ║
    ║  Interface de Gerenciamento          ║
    ╚════════════════════════════════════════╝{Colors.NC}
    """
    print(banner)

def log_info(msg: str):
    print(f"{Colors.BLUE}[*]{Colors.NC} {msg}")

def log_success(msg: str):
    print(f"{Colors.GREEN}[+]{Colors.NC} {msg}")

def log_error(msg: str):
    print(f"{Colors.RED}[-]{Colors.NC} {msg}")

def log_warning(msg: str):
    print(f"{Colors.YELLOW}[!]{Colors.NC} {msg}")

def run_command(cmd: List[str], show_output: bool = True) -> Tuple[int, str, str]:
    """Executa comando e retorna status"""
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            cwd=get_project_root()
        )

        if show_output and result.stdout:
            print(result.stdout)

        return result.returncode, result.stdout, result.stderr
    except Exception as e:
        log_error(f"Erro ao executar comando: {e}")
        return 1, "", str(e)

def get_project_root() -> Path:
    """Retorna diretório raiz do projeto"""
    return Path(__file__).parent.parent

def check_dependencies() -> bool:
    """Verifica se dependências estão instaladas"""
    log_info("Verificando dependências...")

    required = ['gcc', 'g++', 'nasm', 'cmake', 'make']
    missing = []

    for cmd in required:
        try:
            subprocess.run(['which', cmd], capture_output=True, check=True)
        except subprocess.CalledProcessError:
            missing.append(cmd)

    if missing:
        log_warning(f"Faltando: {', '.join(missing)}")
        return False

    log_success("Todas as dependências instaladas")
    return True

def build_project(clean: bool = False) -> bool:
    """Compila o projeto"""
    log_info("Compilando projeto...")

    if clean:
        log_info("Limpando build anterior...")
        run_command(['rm', '-rf', 'build'])

    # Criar diretório build
    build_dir = get_project_root() / 'build'
    build_dir.mkdir(exist_ok=True)

    # Configurar CMake
    log_info("Configurando CMake...")
    ret, _, _ = run_command(['cmake', '..'], show_output=False)
    if ret != 0:
        log_error("CMake configure failed")
        return False

    # Compilar
    log_info("Compilando...")
    ret, _, _ = run_command(['make', '-j4'], show_output=False)
    if ret != 0:
        log_error("Compilação falhou")
        return False

    log_success("Build completo!")
    return True

def run_tests() -> bool:
    """Roda testes"""
    log_info("Rodando testes...")

    test_script = get_project_root() / 'scripts' / 'test.sh'
    if not test_script.exists():
        log_error("Script de testes não encontrado")
        return False

    ret, _, _ = run_command(['bash', str(test_script)])
    return ret == 0

def run_qemu(debug: bool = False) -> bool:
    """Roda no QEMU"""
    log_info("Iniciando QEMU...")

    binary = get_project_root() / 'build' / 'MicroHypervisor'
    if not binary.exists():
        log_error("Binário não encontrado. Execute: python3 interface/cli.py build")
        return False

    cmd = ['qemu-system-x86_64', '-m', '2G', '-nographic', '-kernel', str(binary)]

    if sys.platform == 'linux':
        # Tentar usar KVM
        cmd.insert(1, '-enable-kvm')

    if debug:
        cmd.extend(['-s', '-S'])
        log_info("Modo debug: Conecte com gdb na porta 1234")

    log_success(f"Comando: {' '.join(cmd)}")

    try:
        subprocess.run(cmd)
    except KeyboardInterrupt:
        log_info("QEMU interrompido")

    return True

def run_docker_build() -> bool:
    """Build Docker image"""
    log_info "Building Docker image..."

    ret, _, _ = run_command([
        'docker', 'build', '-t', 'microhypervisor:latest', '.'
    ])

    return ret == 0

def run_docker_run(service: str = 'builder') -> bool:
    """Roda serviço Docker"""
    log_info(f"Iniciando Docker service: {service}")

    ret, _, _ = run_command([
        'docker-compose', 'run', '--rm', service
    ])

    return ret == 0

def show_status():
    """Mostra status do projeto"""
    log_info("Status do projeto:")
    print()

    root = get_project_root()

    # Verificar binário
    binary = root / 'build' / 'MicroHypervisor'
    if binary.exists():
        size = binary.stat().st_size
        log_success(f"✓ Binário: {size:,} bytes")
    else:
        log_warning("✗ Binário não encontrado")

    # Verificar dependências
    deps_ok = check_dependencies()

    # Verificar VT-x
    if sys.platform == 'linux':
        try:
            with open('/proc/cpuinfo', 'r') as f:
                if 'vmx' in f.read():
                    log_success("✓ VT-x suportado")
                else:
                    log_warning("✗ VT-x não disponível")
        except:
            pass

    print()

def interactive_menu():
    """Menu interativo"""
    while True:
        print(f"""
{Colors.PURPLE}Menu Principal:{Colors.NC}
  1. Build do projeto
  2. Build limpo (rebuild)
  3. Rodar testes
  4. Rodar no QEMU
  5. Rodar no QEMU (debug mode)
  6. Verificar status
  7. Docker: Build
  8. Docker: Run shell
  9. Docker: Run tests
  0. Sair
        """)

        choice = input(f"{Colors.CYAN}Escolha: {Colors.NC}").strip()

        if choice == '1':
            build_project()
        elif choice == '2':
            build_project(clean=True)
        elif choice == '3':
            run_tests()
        elif choice == '4':
            run_qemu()
        elif choice == '5':
            run_qemu(debug=True)
        elif choice == '6':
            show_status()
        elif choice == '7':
            run_docker_build()
        elif choice == '8':
            run_docker_run('builder')
        elif choice == '9':
            run_docker_run('tester')
        elif choice == '0':
            log_info("Saindo...")
            break
        else:
            log_error("Opção inválida")

def main():
    """Função principal"""
    parser = argparse.ArgumentParser(
        description='Micro-Hypervisor CLI',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Exemplos:
  python3 interface/cli.py build           # Compila projeto
  python3 interface/cli.py test            # Roda testes
  python3 interface/cli.py run             # Roda no QEMU
  python3 interface/cli.py status          # Mostra status
  python3 interface/cli.py                 # Menu interativo
        """
    )

    parser.add_argument('command', nargs='?',
                       choices=['build', 'test', 'run', 'status', 'clean'],
                       help='Comando a executar')
    parser.add_argument('--clean', '-c',
                       action='store_true',
                       help='Limpa antes de build')
    parser.add_argument('--debug', '-d',
                       action='store_true',
                       help='Modo debug')
    parser.add_argument('--interactive', '-i',
                       action='store_true',
                       help='Menu interativo')

    args = parser.parse_args()

    print_banner()

    # Se nenhum comando ou --interactive, mostra menu
    if not args.command or args.interactive:
        interactive_menu()
        return 0

    # Executar comando
    if args.command == 'build':
        success = build_project(clean=args.clean)
        return 0 if success else 1

    elif args.command == 'test':
        success = run_tests()
        return 0 if success else 1

    elif args.command == 'run':
        success = run_qemu(debug=args.debug)
        return 0 if success else 1

    elif args.command == 'status':
        show_status()
        return 0

    elif args.command == 'clean':
        log_info("Limpando...")
        run_command(['rm', '-rf', 'build'])
        log_success("Limpeza completa")
        return 0

if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        log_info("\nInterrompido pelo usuário")
        sys.exit(130)
    except Exception as e:
        log_error(f"Erro: {e}")
        sys.exit(1)
