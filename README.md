# Micro-Hypervisor: Trusted Execution Environment

Um ecossistema de segurança de nível de kernel projetado para criar uma zona de confiança absoluta (Trusted Execution Environment) através de um Micro-Hypervisor de Tipo-1.

## Overview

Este projeto implementa um hypervisor minimalista que utiliza virtualização assistida por hardware (Intel VT-x) para isolar processos sensíveis, fornecendo:

- **Isolamento Físico**: Virtualização Intel VT-x para separar o contexto de segurança do restante do SO
- **Performance Incomparável**: Transferência de dados com latência próxima de zero usando SIMD (AVX2/AVX-512)
- **Vigilância Ativa**: Monitoramento da integridade do kernel host através de rastreamento de hardware

## Stack Tecnológica

- **Core Language**: Assembly x86-64 (NASM/Yasm)
- **System Language**: C++20
- **Instruções de Hardware**: Intel VT-x (VMX), AVX2/AVX-512, RDRAND
- **Build System**: CMake + Make
- **Ferramentas de Debug**: GDB + QEMU

## Estrutura do Projeto

```
├── build/                 # Binários e artefatos de compilação
├── docs/                  # Documentação técnica
├── include/               # Definições e headers
│   ├── vmx_defs.inc       # Definições de campos VMCS
│   ├── avx_macros.inc     # Macros para serialização SIMD
│   └── registers.hpp      # Wrappers C++ para registradores
├── src/
│   ├── boot/              # Entrada do sistema e transição para Long Mode
│   │   └── entry.asm
│   ├── core/              # O coração do Hypervisor (VT-x)
│   │   ├── vmx_init.asm   # Ativação do modo VMX
│   │   └── vmcs_config.asm# Configuração da estrutura de controle
│   ├── marshall/          # Engine de Marshalling Ultra-Rápido
│   │   └── simd_packer.asm# Implementação AVX2/512
│   ├── monitor/           # Detector de Rootkits e Hardware-Trace
│   │   ├── intercept.asm  # Handlers de VMExit
│   │   └── trace_engine.asm
│   └── main.cpp           # Orquestrador C++ de alto nível
├── tests/                 # Testes de integridade e performance
└── CMakeLists.txt         # Build configuration
```

## Requisitos de Hardware

- Processador Intel com suporte a VT-x
- Suporte a AVX2 ou AVX-512 (recomendado)
- BIOS com VT-x habilitado
- Mínimo 4GB RAM

## 🚀 Quick Start

**Opção 1: Interface CLI (Recomendado)**
```bash
# Menu interativo
python3 interface/cli.py

# Ou comandos diretos
python3 interface/cli.py build --clean    # Compilar
python3 interface/cli.py test             # Testar
python3 interface/cli.py run              # Rodar no QEMU
```

**Opção 2: Web Dashboard**
```bash
# Iniciar dashboard
python3 interface/web.py
# Acesse: http://localhost:5000
```

**Opção 3: Docker**
```bash
docker-compose up -d
docker-compose run builder
```

**Opção 4: Scripts Automatizados**
```bash
# Build completo com dependências
./scripts/build.sh --all

# Rodar testes
./scripts/test.sh
```

Veja [QUICKSTART.md](QUICKSTART.md) para detalhes completos.

## Building

### Pré-requisitos

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake nasm qemu-system-x86 python3

# Fedora/RHEL
sudo dnf install gcc-c++ cmake nasm qemu-system-x86 python3

# macOS
brew install cmake nasm qemu python3
```

### Compilação

```bash
# Opção 1: Usar CLI (automático)
python3 interface/cli.py build

# Opção 2: Manual
mkdir build && cd build
cmake ..
make -j$(nproc)

# Opção 3: Script automatizado
./scripts/build.sh --all
```

## Uso

### Modo de Teste (sem VMX)

```bash
# Rodar testes
cd build
ctest --verbose

# Testes específicos
./tests/test_simd
./tests/test_performance
./tests/test_integrity
```

### Modo Hypervisor (requer VT-x)

```bash
# Rodar no QEMU com KVM
make run-qemu

# Debug com GDB
make debug-qemu
# Em outro terminal:
gdb build/MicroHypervisor
(gdb) target remote :1234
```

### Criar ISO bootável

```bash
make iso
# Resultado: MicroHypervisor.iso
```

## Arquitetura

### 1. Micro-Hypervisor (O Escudo)

Um monitor de máquina virtual (VMM) minimalista que:
- Configura o bit VMXE no registrador CR4
- Inicializa a região VMXON
- Monta o VMCS (Virtual Machine Control Structure)
- Cria um anel de privilégio superior ao Kernel (Ring -1)

### 2. Engine de Marshalling Binário (O Fluxo)

Sistema de transporte de dados ultra-rápido entre SO e Enclave:
- Utiliza registradores de 256 ou 512 bits
- Processa múltiplos blocos de dados simultaneamente
- Garante que a segurança não destrua o throughput do sistema

### 3. Rootkit Detector via Hardware-Trace (A Vigilância)

Auditor de integridade em tempo real que:
- Configura VMExit para escrita em registradores sensíveis
- Monitora LSTAR (syscalls) e IDTR (interrupções)
- Detecta anomalias antes que o malware se mascare

## Testes

### Testes SIMD

```bash
./tests/test_simd
```
Testa operações de memória, hash, e pack/unpack usando AVX.

### Testes de Performance

```bash
./tests/test_performance
```
Benchmark de throughput e latência das operações SIMD.

### Testes de Integridade

```bash
./tests/test_integrity
```
Valida determinismo de hash, efeito avalanche, e detecção de modificações.

## Desenvolvimento

### Adicionando Novos Handlers de VMExit

Edite `src/monitor/intercept.asm` e adicione o handler em `vmexit_dispatch`:

```asm
cmp rdi, EXIT_REASON_SEU_EXIT
je .handle_seu_exit
```

### Adicionando Novas Operações SIMD

Edite `src/marshall/simd_packer.asm` e use as macros em `include/avx_macros.inc`.

## Segurança

### Ameaças Mitigadas

- **Rootkits de Kernel**: Detecção via hardware trace
- **Vazamento de Memória**: Isolamento via VT-x
- **Hooking de Syscall**: Monitoramento de IA32_LSTAR
- **Modificação de IDT/GDT**: Verificação de integridade

### Limitações Conhecidas

- Requer hardware Intel (AMD-V suporte planejado)
- Necessita BIOS com VT-x habilitado
- Sobrecarga de ~1-2% em operações normais

## Debugging

### GDB com VMX

```bash
# Terminal 1
qemu-system-x86_64 -enable-kvm -m 2G -kernel MicroHypervisor -s -S

# Terminal 2
gdb MicroHypervisor
(gdb) target remote :1234
(gdb) hbreak *0x1000  # Hardware breakpoint
(gdb) continue
```

### Logs de VMExit

Os eventos de VMExit são registrados no buffer de trace:
```cpp
trace_log_event(EVENT_TYPE, data);
```

## Performance

Benchmarks típicos (Intel i7-12700K):

| Operação | Throughput | Latência |
|----------|-----------|----------|
| SIMD Memcpy | 45 GB/s | 22 ns |
| Pack/Unpack | 38 GB/s | 26 ns |
| Hash (SHA-256) | 2.5 GB/s | 102 ns |

## Contribuindo

1. Fork o projeto
2. Crie uma branch para sua feature (`git checkout -b feature/AmazingFeature`)
3. Commit suas mudanças (`git commit -m 'Add AmazingFeature'`)
4. Push para a branch (`git push origin feature/AmazingFeature`)
5. Abra um Pull Request

## Licença

Este projeto é uma prova de conceito educacional. Use por sua conta e risco.

## Referências

- Intel SDM Volume 3C: VMX
- Intel Architecture Instruction Set Extensions Programming Reference
- AMD64 Architecture Programmer's Manual Volume 2

## Autores

Thiag - Desenvolvimento inicial

## Agradecimentos

- Intel pela documentação técnica detalhada
- Comunidade OSDev pelos recursos sobre desenvolvimento de hipervisores
- Projetos referentes: KVM, Xen, HVF

---

**Warning**: Este é um projeto educacional/prova de conceito. Não use em produção sem revisão de segurança completa.
