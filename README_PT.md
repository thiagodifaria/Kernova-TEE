# Kernova-TEE

![Kernova-TEE](https://img.shields.io/badge/Kernova--TEE-Trusted%20Execution%20Environment-111827?style=for-the-badge)

**Kernova-TEE e um projeto de trusted execution x86-64 orientado a pesquisa, implementado com Assembly NASM, C++20 e um driver Linux privilegiado de validacao. O projeto explora isolamento baseado em hardware por Intel VMX e AMD SVM, movimentacao segura de dados com SIMD e verificacoes de integridade de baixo nivel para cenarios de trusted execution.**

[![Versao](https://img.shields.io/badge/Version-2.0.0-2563EB?style=flat)](README.md)
[![Assembly](https://img.shields.io/badge/x86--64-NASM-525252?style=flat)](service-api/service-cpp/src)
[![C++](https://img.shields.io/badge/C++-20-00599C?style=flat&logo=cplusplus&logoColor=white)](service-api/service-cpp)
[![Intel VT-x](https://img.shields.io/badge/Intel%20VT--x-VMX-0071C5?style=flat&logo=intel&logoColor=white)](service-api/service-cpp/src/virtualization/intel_vmx/core)
[![AMD SVM](https://img.shields.io/badge/AMD--V-SVM-ED1C24?style=flat)](service-api/service-cpp/src/virtualization/amd_svm)
[![Kernel Driver](https://img.shields.io/badge/Linux%20Driver-/dev/kernova-111827?style=flat&logo=linux&logoColor=white)](service-kernel/linux)
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

Kernova-TEE modela uma fronteira de trusted execution baseada em virtualizacao assistida por hardware em x86-64. O projeto preserva o caminho Intel VMX, adiciona um caminho AMD SVM para hosts AMD e introduz um driver kernel Linux para levar a validacao privilegiada do modelo em userspace para `/dev/kernova`. O objetivo e explorar como um monitor controlado por hardware pode isolar memoria sensivel, interceptar eventos relevantes para integridade e mover dados por caminhos controlados de marshalling.

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
- Orquestracao em C++20 para selecao de backend de virtualizacao, buffers de enclave, estado do monitor e auto-testes em modo de desenvolvimento.
- Fronteira de backend preparada para Kernel Driver, Intel VMX, AMD SVM e fallback userspace PoC.
- Driver Linux privilegiado expondo `/dev/kernova` por uma ABI compartilhada via ioctl.
- Caminho AMD SVM em Ring 0 com preparacao de `EFER.SVME`, `VM_HSAVE_PA`, alocacao de VMCB, host-save area e `VMRUN` protegido por gates explicitos.
- Suite de testes nativos cobrindo comportamento SIMD, desempenho, validacao de campos VMCS e rotinas de integridade.
- Ambiente Docker Compose para builds Linux reproduziveis a partir de hosts Windows, macOS ou Linux.
- Console Flask de validacao para fluxos locais de build, teste, status e runtime.

---

## Estrutura do Repositorio

```text
client-web/
  app.py                          Console Flask de validacao para build/test/status
  requirements.txt

service-api/
  service-cpp/
    src/
      marshall/                   Rotinas SIMD de pack/unpack/copy/hash
      monitor/                    Trace engine e rotinas de integridade
      virtualization/
        amd_svm/                  Modelo de capabilities AMD SVM e VMCB
        intel_vmx/                Rotinas Intel VMX de boot, VMCS e VMExit
        cpu_features.cpp          Deteccao de vendor e capacidades da CPU
        hypervisor_backend.cpp    Selecao e ciclo de vida de backends
      main.cpp                    Camada de orquestracao C++20
      vmx_stubs.cpp               Stubs VMX seguros para builds em userspace
    include/
      vmx_defs.inc                Encodings VMCS e constantes VMX
      avx_macros.inc              Biblioteca de macros SIMD
      registers.hpp               Wrappers de CPUID, CR e MSR
      virtualization/             Interfaces de backend e modelo de capacidades da CPU
    tests/                        Suites nativas via CTest
    CMakeLists.txt                Sistema de build principal
    Makefile                      Caminho alternativo de build baixo nivel
    linker.ld                     Layout de memoria bare-metal

infra/
  Dockerfile                      Imagem Linux de build e teste
  compose.yaml                    Servicos builder, tester e web

service-kernel/
  linux/
    kernova_main.c                Device /dev/kernova e dispatcher de ioctls
    amd_svm/                      Caminho privilegiado de validacao AMD SVM
    intel_vmx/                    Stub de capacidade Intel VMX para este marco
    include/                      Estado e interfaces internas do driver
    Makefile                      Entrypoint de build do modulo kernel Linux

shared/
  kernova_abi.h                   Contrato ioctl compartilhado entre user space e kernel

scripts/
  build.sh                        Entrypoint de build, clean, package e pipeline
  test.sh                         Entrypoint CMake + CTest
  docker-entrypoint.sh            Dispatcher de comandos do container
  kernel-build.sh                 Helper de build do modulo kernel Linux
  kernel-load.sh                  Helper de load seguro ou com VMRUN habilitado
  kernel-unload.sh                Helper de unload do driver

docs/
  ARCHITECTURE.md                 Fronteiras do repositorio e subsistemas
```

Essa organizacao segue a mesma convencao dos outros projetos service-oriented do workspace: clientes ficam em `client-*`, backends ficam em `service-api/service-*`, componentes privilegiados de kernel ficam em `service-kernel`, infraestrutura fica em `infra`, comandos operacionais ficam em `scripts`, contratos compartilhados ficam em `shared` e material arquitetural fica em `docs`.

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

Ring 0 / modo nao-raiz VMX ou modo guest SVM
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
  -> detectar vendor da CPU e capacidades de virtualizacao
  -> selecionar backend Kernel Driver, Intel VMX, AMD SVM ou userspace PoC
  -> inicializar backend de hardware quando disponivel
  -> executar auto-testes em modo de desenvolvimento
  -> entrar no loop idle de prova de conceito
```

### Fronteira de Backend de Virtualizacao

`service-api/service-cpp/src/virtualization` concentra a selecao de backend e a deteccao de capacidades da CPU. Ele preserva Intel VMX atras dessa fronteira, prepara o contexto runtime AMD SVM, expoe dados estruturados de validacao no console web e prefere o driver Linux quando `/dev/kernova` estiver carregado.

| Backend | Status Atual | Proposito |
|---------|--------------|-----------|
| Kernel Driver | Caminho Linux privilegiado | Conversa com `/dev/kernova` e delega a validacao de hardware para Ring 0 |
| Userspace PoC | Fallback ativo | Valida SIMD, enclave, trace e integridade sem instrucoes privilegiadas |
| Intel VMX | Alvo existente | Usa o caminho atual VMX/VMCS quando VT-x estiver disponivel |
| AMD SVM | Contexto runtime minimo preparado | Detecta recursos AMD SVM, prepara VMCB/host-save state e reserva `VMRUN` para o driver kernel Linux |

A execucao de backend de hardware fica desabilitada por padrao em builds de desenvolvimento. Para tentar virtualizacao em hardware explicitamente, use `KERNOVA_ENABLE_HARDWARE_BACKEND=1` ou `KERNOVA_STRICT_VMX=1`.

### Driver Kernel Linux

`service-kernel/linux` e o harness privilegiado de validacao. Ele cria `/dev/kernova`, implementa a ABI compartilhada em `shared/kernova_abi.h` e comeca pelo caminho AMD SVM porque a maquina bare metal disponivel para validacao e AMD.

| ioctl | Proposito |
|-------|-----------|
| `KERNOVA_IOCTL_QUERY_CAPS` | Reportar vendor da CPU, capacidade VMX/SVM e estado do driver |
| `KERNOVA_IOCTL_INIT_BACKEND` | Inicializar o backend privilegiado |
| `KERNOVA_IOCTL_CREATE_VM` | Alocar e preparar um contexto minimo de VM |
| `KERNOVA_IOCTL_RUN_VM` | Tentar execucao guest quando explicitamente habilitada |
| `KERNOVA_IOCTL_DESTROY_VM` | Destruir o estado alocado da VM |

### Modelo de Backend AMD SVM

`service-api/service-cpp/src/virtualization/amd_svm` concentra o modelo AMD-V/SVM usado pela selecao de backend. Ele detecta revisao SVM, quantidade de ASIDs e recursos opcionais como nested paging, NRIP save, VMCB clean bits, decode assists e pause filtering. Tambem prepara um contexto runtime minimo com pagina VMCB alinhada a 4KB e host-save area. `VMRUN` permanece bloqueado em builds userspace e fica reservado para o driver Linux privilegiado.

| Area | Dado / Interface | Proposito |
|------|------------------|-----------|
| Descoberta SVM | CPUID `0x80000001`, `0x8000000A` | Detectar suporte SVM e recursos da implementacao |
| MSRs de controle | `EFER.SVME`, `VM_CR`, `VM_HSAVE_PA` | Modelar registradores exigidos antes de `VMRUN` |
| Pagina VMCB | `amd_svm::Vmcb` | Preparar layout de pagina 4KB de controle/save-state |
| Contexto runtime | `amd_svm::SvmRuntimeContext` | Guardar VMCB, host-save area e estado de launch |
| Modelo de capabilities | `amd_svm::SvmCapabilities` | Expor prontidao SVM para backend selection e testes |

### Backend Intel VMX

`service-api/service-cpp/src/virtualization/intel_vmx/core` contem as rotinas VMX de baixo nivel.

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
| `src/virtualization/intel_vmx/boot/entry.asm` | NASM | Entrada de boot, long mode, verificacao VMX e setup SIMD |
| `src/virtualization/intel_vmx/core/vmx_init.asm` | NASM | Setup da regiao VMXON, ativacao VMX e helpers de transicao |
| `src/virtualization/intel_vmx/core/vmcs_config.asm` | NASM | Estado guest/host da VMCS e campos de controle |
| `src/marshall/simd_packer.asm` | NASM | Rotinas SIMD de copy, zero, pack, unpack e hash por blocos |
| `src/virtualization/intel_vmx/monitor/intercept.asm` | NASM | Dispatch de VMExit e stubs de handlers |
| `src/monitor/trace_engine.asm` | NASM | Logging de eventos, verificacoes de integridade e watchpoints |
| `src/main.cpp` | C++20 | Orquestracao de hypervisor, enclave, backend, monitor e testes |
| `src/vmx_stubs.cpp` | C++20 | Stubs VMX para builds de desenvolvimento em userspace |
| `src/virtualization/amd_svm/amd_svm.cpp` | C++20 | Mapeamento de capabilities AMD SVM e inicializacao de VMCB |
| `src/virtualization/cpu_features.cpp` | C++20 | Deteccao de vendor da CPU e capacidades VMX/SVM |
| `src/virtualization/hypervisor_backend.cpp` | C++20 | Selecao de backend para userspace PoC, Intel VMX e AMD SVM |
| `include/registers.hpp` | C++20 | Helpers de CPUID, registradores de controle e MSR |
| `include/virtualization/*.hpp` | C++20 | Interfaces de backend e modelo de capacidades da CPU |
| `include/vmx_defs.inc` | NASM | Constantes VMCS e definicoes VMX |
| `include/avx_macros.inc` | NASM | Macros auxiliares AVX |

---

## Requisitos

### Requisitos do Host

| Requisito | Detalhe |
|-----------|---------|
| CPU | Processador Intel VT-x ou AMD-V/SVM para execucao futura em hardware |
| Firmware | Virtualizacao habilitada na BIOS/UEFI |
| SIMD | AVX2 recomendado; caminhos AVX-512 sao modelados quando disponiveis |
| SO | Linux para validacao nativa direta; Docker recomendado a partir de Windows/macOS |
| Toolchain | CMake, NASM, GCC/G++, QEMU e Docker para o fluxo padrao |
| Driver kernel | Headers Linux do kernel em execucao para compilar `service-kernel/linux` |

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

### Console Web de Validacao

```bash
docker compose -f infra/compose.yaml up web
```

O console de validacao fica exposto em `http://localhost:5000`.

Endpoints do console:

| Endpoint | Proposito |
|----------|-----------|
| `/api/status` | Status de binario, dependencias, sistema e virtualizacao |
| `/api/validation` | Snapshot estruturado de validacao para a UI do navegador |
| `/api/test` | Executa a suite CTest |
| `/api/runtime` | Executa o fluxo de validacao runtime do Kernova |

### Build Manual com CMake

```bash
cmake -S service-api/service-cpp -B service-api/service-cpp/build -DCMAKE_BUILD_TYPE=Release
cmake --build service-api/service-cpp/build --parallel
ctest --test-dir service-api/service-cpp/build --output-on-failure
```

### Driver Kernel Linux

Compile o modulo em Linux bare metal:

```bash
bash scripts/kernel-build.sh
```

Carregue o driver em modo seguro:

```bash
bash scripts/kernel-load.sh
```

Carregue o driver com o caminho experimental AMD `VMRUN` habilitado:

```bash
bash scripts/kernel-load.sh vmrun
KERNOVA_ENABLE_HARDWARE_BACKEND=1 ./service-api/service-cpp/build/Kernova
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
| `test_backend` | Deteccao de capacidades da CPU e selecao de backend de virtualizacao | Nao |
| `test_amd_svm` | Modelo de capabilities AMD SVM e layout VMCB | Nao |

Status atual de validacao:

- Build Docker/Linux concluido com sucesso.
- `test_simd`, `test_vmcs`, `test_performance`, `test_integrity`, `test_backend` e `test_amd_svm` passam.
- Execucao VMX/SVM em hardware continua dependente do ambiente e nao e declarada pela validacao em Docker.
- Validacao do driver kernel Linux fica pendente de execucao bare metal com headers do kernel correspondente e virtualizacao habilitada no firmware.

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
- AMD SVM `VMRUN` existe apenas pelo driver Linux e fica protegido por `allow_vmrun=1` mais `KERNOVA_ENABLE_HARDWARE_BACKEND=1`.
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
