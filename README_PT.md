# Kernova-TEE

![Kernova-TEE](https://img.shields.io/badge/Kernova--TEE-Trusted%20Execution%20Environment-111827?style=for-the-badge)

**Kernova-TEE e um micro-hypervisor Type-1 orientado a pesquisa para sistemas x86-64, implementado com Assembly NASM e uma camada de orquestracao em C++20. O projeto explora isolamento baseado em hardware, monitoramento em modo raiz VMX, movimentacao segura de dados com SIMD e verificacoes de integridade de baixo nivel para cenarios de trusted execution.**

[![Versao](https://img.shields.io/badge/Version-1.0.0-2563EB?style=flat)](README.md)
[![Assembly](https://img.shields.io/badge/x86--64-NASM-525252?style=flat)](service-api/service-cpp/src)
[![C++](https://img.shields.io/badge/C++-20-00599C?style=flat&logo=cplusplus&logoColor=white)](service-api/service-cpp)
[![Intel VT-x](https://img.shields.io/badge/Intel%20VT--x-VMX-0071C5?style=flat&logo=intel&logoColor=white)](service-api/service-cpp/src/core)
[![SIMD](https://img.shields.io/badge/SIMD-AVX2%20%7C%20AVX--512-6B7280?style=flat)](service-api/service-cpp/src/marshall)
[![Runtime](https://img.shields.io/badge/Runtime-Docker%20Compose-2496ED?style=flat&logo=docker&logoColor=white)](infra)
[![Licenca](https://img.shields.io/badge/License-MIT-success?style=flat)](LICENSE)

---

## Documentacao

**README principal:** [README.md](README.md)  
**README em ingles:** [README_EN.md](README_EN.md)  
**Arquitetura:** [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)

---

## Visao Geral

Kernova-TEE modela uma fronteira de trusted execution baseada em Intel VT-x. O projeto posiciona um monitor de hypervisor em modo raiz VMX e trata o sistema operacional host como um guest em modo nao-raiz VMX. O objetivo e explorar como um monitor controlado por hardware pode isolar memoria sensivel, interceptar eventos relevantes para integridade e mover dados por caminhos controlados de marshalling.

O projeto fica propositalmente proximo do hardware. A maior parte das operacoes VMX e SIMD e implementada em Assembly x86-64, enquanto C++20 fornece a camada de orquestracao para estado, ciclo de vida, testes e execucao em modo de desenvolvimento.

Kernova-TEE deve ser entendido como um projeto de pesquisa em seguranca de baixo nivel e programacao de sistemas. Ele nao e uma implementacao de TEE pronta para producao e nao pretende substituir hypervisors maduros, TEEs de hardware ou frameworks de seguranca de kernel.

---

## Problema

Ferramentas tradicionais de seguranca de endpoint rodam no mesmo nivel de privilegio do kernel ou abaixo de mecanismos de virtualizacao de hardware. Quando codigo malicioso opera em Ring 0, ele pode modificar pontos de entrada de syscall, alterar codigo de kernel, manipular tabelas de descritores, esconder regioes de memoria e interferir em verificacoes de integridade realizadas pelo proprio sistema operacional.

Kernova explora um plano de controle diferente:

- O monitor roda em modo raiz VMX, frequentemente chamado de Ring -1.
- O sistema operacional e movido para modo nao-raiz VMX.
- Regioes sensiveis do monitor e do enclave ficam fora do controle normal do guest.
- Escritas em registradores, MSRs e estado relacionado a descritores podem ser roteadas por VMExit.

A premissa central e que um monitor menor e controlado por hardware pode observar e restringir comportamentos selecionados do sistema operacional a partir de um nivel de privilegio que o kernel guest nao consegue sobrescrever diretamente.

---

## Capacidades Principais

- Estrutura de micro-hypervisor Type-1 mirando Ring -1 / operacao em modo raiz VMX.
- Caminho de bootstrap VMX com `VMXON`, preparacao de VMCS, `VMLAUNCH`, `VMRESUME`, `VMREAD`, `VMWRITE`, `VMXOFF` e `INVEPT`.
- Modelagem de estado guest/host em VMCS e controles de execucao baseados em processador.
- Monitoramento orientado a VMExit para escritas em registradores de controle, alteracoes de IA32_LSTAR, modificacoes em tabelas de descritores e operacoes `VMCALL` do guest.
- Rotinas de marshalling SIMD usando Assembly orientado a AVX2 e AVX-512.
- Copia segura, limpeza segura, pack/unpack e hash por blocos expostos como simbolos chamaveis por C.
- Orquestracao em C++20 para inicializacao VMX, buffers de enclave, estado do monitor e auto-testes em modo de desenvolvimento.
- Suite de testes nativos cobrindo comportamento SIMD, desempenho, validacao de campos VMCS e rotinas de integridade.
- Ambiente Docker Compose para builds Linux reproduziveis a partir de hosts Windows, macOS ou Linux.
- CLI e dashboard Flask para fluxos locais de operacao.

---

## Estrutura do Repositorio

```text
client-cli/
  main.py                         Cliente de gerenciamento por terminal

client-web/
  app.py                          Dashboard Flask para build/test/status
  requirements.txt

service-api/
  service-cpp/
    src/
      boot/                       Entrada de boot, long mode e setup do processador
      core/                       Inicializacao VMX e configuracao VMCS
      marshall/                   Rotinas SIMD de pack/unpack/copy/hash
      monitor/                    Interceptacao VMExit e trace de integridade
      main.cpp                    Camada de orquestracao C++20
      vmx_stubs.cpp               Stubs VMX seguros para builds em userspace
    include/
      vmx_defs.inc                Encodings VMCS e constantes VMX
      avx_macros.inc              Biblioteca de macros SIMD
      registers.hpp               Wrappers de CPUID, CR e MSR
    tests/                        Suites nativas via CTest
    CMakeLists.txt                Sistema de build principal
    Makefile                      Caminho alternativo de build baixo nivel
    linker.ld                     Layout de memoria bare-metal

infra/
  Dockerfile                      Imagem Linux de build e teste
  compose.yaml                    Servicos builder, tester e web

scripts/
  build.sh                        Entrypoint de build, clean, package e pipeline
  test.sh                         Entrypoint CMake + CTest
  docker-entrypoint.sh            Dispatcher de comandos do container

docs/
  ARCHITECTURE.md                 Fronteiras do repositorio e subsistemas
```

Essa organizacao segue a mesma convencao dos outros projetos service-oriented do workspace: clientes ficam em `client-*`, backends ficam em `service-api/service-*`, infraestrutura fica em `infra`, comandos operacionais ficam em `scripts` e material arquitetural fica em `docs`.

---

## Arquitetura

### Modelo de Privilegio

```text
Ring -1 / modo raiz VMX
  Monitor Kernova
  Gerenciamento de estado VMX
  Regiao de memoria estilo enclave
  Handlers de VMExit
  Trace de integridade

Ring 0 / modo nao-raiz VMX
  Sistema operacional guest
  Drivers de kernel
  Pontos de entrada de syscall
  Tabelas de descritores

Ring 3
  Aplicacoes de usuario
```

O monitor e projetado para operar acima do kernel guest. O guest continua executando normalmente, mas eventos privilegiados selecionados sao modelados como superficies de VMExit que podem ser registradas, inspecionadas ou restringidas pelo monitor.

### Fluxo de Inicializacao

```text
_start (entry.asm)
  -> desabilitar interrupcoes
  -> inicializar stack e estado de segmentos
  -> verificar long mode
  -> verificar suporte VMX via CPUID e IA32_FEATURE_CONTROL
  -> preparar regioes identity-mapped para estruturas VMX
  -> inicializar estado SIMD
  -> chamar inicializacao C++ do hypervisor

hypervisor_init (main.cpp)
  -> inicializar monitor de trace
  -> alocar buffer de enclave
  -> inicializar estado VMX
  -> preparar regiao VMXON
  -> configurar estado guest/host da VMCS
  -> executar auto-testes em modo de desenvolvimento
  -> entrar no loop idle de prova de conceito
```

### Nucleo VMX do Hypervisor

`service-api/service-cpp/src/core` contem as rotinas VMX de baixo nivel.

| Operacao | Instrucao / Interface | Proposito |
|----------|-----------------------|-----------|
| Habilitar VMX | `CR4.VMXE` | Permitir operacao VMX no processador |
| Entrar em modo raiz VMX | `VMXON` | Ativar VMX usando uma regiao VMXON alinhada a 4KB |
| Carregar VMCS | `VMPTRLD` | Selecionar a Virtual Machine Control Structure ativa |
| Limpar VMCS | `VMCLEAR` | Resetar estado de launch da VMCS |
| Ler VMCS | `VMREAD` | Inspecionar campos da VMCS |
| Escrever VMCS | `VMWRITE` | Configurar campos de guest, host e controle |
| Lancar guest | `VMLAUNCH` | Entrar na execucao guest nao-raiz VMX |
| Retomar guest | `VMRESUME` | Continuar execucao guest apos VMExit |
| Sair de VMX | `VMXOFF` | Desativar operacao VMX |
| Invalidar traducoes EPT | `INVEPT` | Limpar traducoes de Extended Page Table |

MSRs importantes:

| MSR | Endereco | Proposito |
|-----|----------|-----------|
| `IA32_FEATURE_CONTROL` | `0x3A` | Politica de habilitacao e lock de VMX |
| `IA32_VMX_BASIC` | `0x480` | Revision ID VMX e metadados da VMCS |
| `IA32_VMX_PROCBASED_CTLS` | `0x482` | Controles de execucao baseados em processador |
| `IA32_EFER` | `0xC0000080` | Estado de long mode e recursos relacionados |

### Engine de Marshalling SIMD

`service-api/service-cpp/src/marshall` implementa rotinas de memoria de alto throughput pensadas para movimentacao de dados entre host e enclave.

| Funcao | Descricao |
|--------|-----------|
| `simd_pack_data` | Empacota dados de origem em um buffer de destino com operacoes vetoriais |
| `simd_unpack_data` | Extrai dados empacotados para um buffer legivel pelo host |
| `simd_secure_memcpy` | Realiza copia alinhada vetorizada |
| `simd_zero_memory` | Limpa memoria usando operacoes vetoriais de zeroing |
| `simd_hash_blocks` | Fornece o primitivo atual de hash por blocos usado pelos testes |

Familias de instrucoes representativas:

```nasm
VMOVDQA    ; load/store vetorial alinhado
VPXOR      ; XOR vetorial e primitivo de zeroing
VZEROUPPER ; evita penalidades de transicao AVX/SSE
VMOVDQA64  ; acesso vetorial alinhado de 64 bytes com AVX-512
VPXORQ     ; XOR de quadwords com AVX-512
```

### Monitor de Hardware Trace

`service-api/service-cpp/src/monitor` modela logging de eventos no monitor e verificacoes de integridade.

| Superficie | Relevancia de seguranca |
|------------|--------------------------|
| Escritas em CR0/CR3/CR4 | Paging, write-protection e estado relacionado a VMX podem ser alterados aqui |
| Escritas em IA32_LSTAR | Hooks de entrada de syscall frequentemente passam por essa MSR |
| Alteracoes em IDTR/GDTR | Relocacao de tabelas de descritores pode indicar interceptacao de interrupcoes ou excecoes |
| `VMCALL` | Interface de requisicao guest-to-hypervisor |
| Configuracao de watchpoints | Rastreia regioes de memoria sensiveis a integridade |

As funcoes chamaveis por C incluem `trace_init`, `trace_log_event`, `trace_check_integrity` e `trace_set_watchpoint`.

---

## Quebra por Codigo-Fonte

| Modulo | Linguagem | Proposito |
|--------|-----------|-----------|
| `src/boot/entry.asm` | NASM | Entrada de boot, long mode, verificacao VMX e setup SIMD |
| `src/core/vmx_init.asm` | NASM | Setup da regiao VMXON, ativacao VMX e helpers de transicao |
| `src/core/vmcs_config.asm` | NASM | Estado guest/host da VMCS e campos de controle |
| `src/marshall/simd_packer.asm` | NASM | Rotinas SIMD de copy, zero, pack, unpack e hash por blocos |
| `src/monitor/intercept.asm` | NASM | Dispatch de VMExit e stubs de handlers |
| `src/monitor/trace_engine.asm` | NASM | Logging de eventos, verificacoes de integridade e watchpoints |
| `src/main.cpp` | C++20 | Orquestracao de hypervisor, enclave, VMX, monitor e testes |
| `src/vmx_stubs.cpp` | C++20 | Stubs VMX para builds de desenvolvimento em userspace |
| `include/registers.hpp` | C++20 | Helpers de CPUID, registradores de controle e MSR |
| `include/vmx_defs.inc` | NASM | Constantes VMCS e definicoes VMX |
| `include/avx_macros.inc` | NASM | Macros auxiliares AVX |

---

## Requisitos

### Requisitos do Host

| Requisito | Detalhe |
|-----------|---------|
| CPU | Processador Intel com suporte VT-x para execucao em hardware |
| Firmware | Intel Virtualization Technology habilitado na BIOS/UEFI |
| SIMD | AVX2 recomendado; caminhos AVX-512 sao modelados quando disponiveis |
| SO | Linux para validacao nativa direta; Docker recomendado a partir de Windows/macOS |
| Toolchain | CMake, NASM, GCC/G++, QEMU e Docker para o fluxo padrao |

### Pacotes Linux

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake nasm qemu-system-x86 gdb python3 docker.io
```

---

## Build e Execucao

### Build via Docker

```bash
docker compose -f infra/compose.yaml run --rm builder
```

### Testes via Docker

```bash
docker compose -f infra/compose.yaml run --rm tester
```

### CLI

```bash
python3 client-cli/main.py
python3 client-cli/main.py build --clean
python3 client-cli/main.py test
python3 client-cli/main.py status
python3 client-cli/main.py run
python3 client-cli/main.py run --debug
```

### Dashboard Web

```bash
docker compose -f infra/compose.yaml up web
```

O dashboard fica exposto em `http://localhost:5000`.

### Build Manual com CMake

```bash
cmake -S service-api/service-cpp -B service-api/service-cpp/build -DCMAKE_BUILD_TYPE=Release
cmake --build service-api/service-cpp/build --parallel
ctest --test-dir service-api/service-cpp/build --output-on-failure
```

### QEMU

```bash
qemu-system-x86_64 -enable-kvm -m 2G -kernel service-api/service-cpp/build/Kernova -nographic
```

Modo debug:

```bash
qemu-system-x86_64 -enable-kvm -m 2G -kernel service-api/service-cpp/build/Kernova -s -S -nographic
gdb service-api/service-cpp/build/Kernova
```

---

## Testes

O projeto usa CTest a partir do diretorio de build do servico C++.

```bash
./scripts/test.sh
```

Suites:

| Teste | Escopo | Requer VT-x |
|-------|--------|-------------|
| `test_simd` | Copia SIMD, zeroing, pack/unpack e comportamento de hash | Nao |
| `test_vmcs` | Encoding de campos VMCS e invariantes de configuracao | Sem execucao VMX direta |
| `test_performance` | Medicoes de throughput e latencia SIMD | Nao |
| `test_integrity` | Determinismo de hash, avalanche, colisoes e rotinas de monitor | Nao |

Status atual de validacao:

- Build Docker/Linux concluido com sucesso.
- `test_simd`, `test_vmcs` e `test_performance` passam.
- `test_integrity` falha em assertions conhecidas de qualidade de hash relacionadas a avalanche e colisoes em `simd_hash_blocks`.

---

## Layout de Memoria

O linker script modela a organizacao bare-metal pretendida:

```text
Endereco    Regiao                  Tamanho  Proposito
0x100000    Codigo do hypervisor     256KB    .text e dados somente leitura
0x200000    Regiao VMXON              4KB     Estrutura VMXON com revision ID VMX
0x201000    Regiao VMCS               4KB     Virtual Machine Control Structure ativa
0x202000    Stack do hypervisor        16KB    Stack em Ring -1
0x210000    Regiao de enclave          1MB     Memoria de trabalho protegida
0x310000    Buffer de dados            64KB    Buffer de staging SIMD
0x410000    Buffer de trace            64KB    Armazenamento de eventos e integridade
```

---

## Modelo de Seguranca

Kernova-TEE se concentra em um conjunto estreito de superficies de ameaca de baixo nivel.

| Superficie de ameaca | Caminho de deteccao ou controle |
|----------------------|----------------------------------|
| Rootkits de kernel alterando paging ou estado de controle | VMExit em operacoes selecionadas de registradores de controle |
| Hooking de syscall ou tabela de syscall | Monitoramento de IA32_LSTAR |
| Manipulacao de tabelas de descritores | Verificacoes de integridade em IDTR/GDTR |
| Adulteracao de memoria em regioes monitoradas | Watchpoints e verificacoes de buffers de integridade |
| Comunicacao guest-to-monitor | Interface restrita via `VMCALL` |

Limitacoes:

- E uma prova de conceito, nao uma TEE formalmente verificada.
- O primitivo atual de hash nao e criptograficamente suficiente, como indicado por `test_integrity`.
- Execucao em modo raiz VMX nao pode ser validada dentro de containers Docker padrao.
- O comportamento em hardware depende do modelo de CPU, configuracao de firmware e politica de virtualizacao do host.
- O projeto ainda nao implementa uma politica completa de isolamento baseada em EPT adequada para producao.

---

## Benchmarks

O projeto inclui testes orientados a benchmark para throughput e latencia SIMD. Os valores historicos documentados pelo projeto incluem:

| Operacao | Classe alvo / observada | Abordagem de implementacao |
|----------|--------------------------|-----------------------------|
| Memcpy SIMD | Dezenas de GB/s em CPUs desktop modernas | `VMOVDQA` / copia vetorizada |
| Pack/unpack | Caminho de staging de alto throughput | XOR e load/store vetorial alinhado |
| Zeroing de memoria | Limpeza na largura do vetor | `VPXOR` mais stores vetoriais |
| Hash por blocos | Primitivo SIMD experimental | Rotina Assembly por blocos |
| Overhead de VMExit | Dependente de hardware | Caminho de transicao VMX |

Os resultados devem ser interpretados como medicoes de desenvolvimento, nao como garantias portaveis.

---

## Notas Operacionais

- Use Docker Compose para o caminho de build mais confiavel a partir do Windows.
- A stack Compose armazena artefatos Linux de build em um volume Docker para evitar conflitos com caches CMake do Windows.
- Builds nativos Windows/MinGW podem falhar em APIs POSIX de alocacao e em suposicoes de Assembly orientadas a ELF.
- Execucao em QEMU requer uma imagem de kernel adequada e suporte de virtualizacao do host.
- Docker e adequado para compilacao, testes e dashboard, mas nao para execucao direta em VT-x.

---

## Licenca

Este projeto e licenciado sob a MIT License. Consulte [LICENSE](LICENSE).

## Contato

Thiago Di Faria - [thiagodifaria@gmail.com](mailto:thiagodifaria@gmail.com)
