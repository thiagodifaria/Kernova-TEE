# Instruções Detalhadas de Uso

## Guia de Início Rápido

### 1. Verificar Suporte a VT-x

Antes de compilar, verifique se seu processador suporta Intel VT-x:

```bash
# Linux
cat /proc/cpuinfo | grep vmx
# Se aparecer "vmx" na flags, VT-x é suportado

# Verificar se está habilitado no BIOS
dmesg | grep -i vmx
# Se aparecer "vmx: enabled", está habilitado
```

Se não estiver habilitado:
1. Entre na BIOS/UEFI
2. Procure por "Intel Virtualization Technology" ou "Intel VT-x"
3. Habilite a opção
4. Salve e reinicie

### 2. Instalar Dependências

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake nasm qemu-system-x86 qemu-utils gdb
```

#### Fedora/RHEL/CentOS
```bash
sudo dnf install -y gcc-c++ cmake nasm qemu-system-x86 qemu-kvm gdb
```

#### Arch Linux
```bash
sudo pacman -S base-devel cmake nasm qemu-system-x86 gdb
```

#### macOS
```bash
brew install cmake nasm qemu
```

### 3. Compilar o Projeto

#### Usando CMake (Recomendado)
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

#### Usando Make (Simples)
```bash
make all
```

### 4. Rodar Testes

```bash
cd build
ctest --output-on-failure
```

Ou individualmente:
```bash
./tests/test_simd
./tests/test_performance
./tests/test_vmcs
./tests/test_integrity
```

### 5. Rodar o Hypervisor

#### No QEMU (Recomendado para desenvolvimento)
```bash
# Com KVM (Linux)
make run
# ou
qemu-system-x86_64 -enable-kvm -m 2G -kernel build/MicroHypervisor -nographic

# Sem KVM (qualquer SO)
qemu-system-x86_64 -m 2G -kernel build/MicroHypervisor -nographic
```

#### Criar ISO Bootável
```bash
cd build
make iso
# Grave a ISO em um USB ou rode em VM
```

#### Hardware Real (CUIDADO - Risco de travamento)
```bash
# Grave em USB bootável
dd if=MicroHypervisor.iso of=/dev/sdX bs=1M status=progress
# Boot pelo USB
```

## Debugging

### Preparar para Debug

Compile com símbolos de debug:
```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make clean
make -j$(nproc)
```

### Debug com QEMU + GDB

Terminal 1:
```bash
qemu-system-x86_64 -enable-kvm -m 2G -kernel MicroHypervisor -s -S -nographic
```

Terminal 2:
```bash
gdb MicroHypervisor
(gdb) target remote :1234
(gdb) hbreak _start        # Hardware breakpoint no entry point
(gdb) continue             # Continuar execução
(gdb) info registers       # Ver registradores
(gdb) x/10i $pc            # Disassembler próximas instruções
(gdb) stepi                # Step instruction
```

### Comandos GDB Úteis

```gdb
# Breakpoints
hbreak *0x1000             # Hardware breakpoint
break vmx_init             # Breakpoint em função
break *vmx_init+42         # Breakpoint em offset específico

# Execução
continue                   # Continuar até próximo breakpoint
stepi                      # Executar uma instrução
nexti                      # Executar uma instrução (pula calls)

# Inspecionar
info registers             # Todos os registradores
print $rax                 # Valor específico
x/10x 0x1000              # Examinar memória
x/5i $pc                   # Disassembler próximas 5 instruções
backtrace                  # Stack trace

# VMCS (se VMX estiver ativo)
vmread 0x6800              # Ler campo VMCS (Guest CR0)
```

## Troubleshooting

### Erro: "VMX not supported"

**Causa**: VT-x não está habilitado no BIOS ou processador não suporta.

**Solução**:
1. Verifique se o CPU suporta: `cat /proc/cpuinfo | grep vmx`
2. Entre na BIOS e habilite "Intel Virtualization Technology"
3. Reinicie

### Erro: "VMXON failed"

**Causa**: VT-x está desabilitado ou em uso por outro hypervisor (VirtualBox, VMware, etc.)

**Solução**:
```bash
# Pare todos os hypervisors
sudo systemctl stop libvirtd
sudo modprobe -r kvm_intel
sudo modprobe -r kvm

# Tente novamente
```

### Erro: Segmentation Fault

**Causa**: Provável acesso inválido à memória ou VMCS não configurado corretamente.

**Solução**:
```bash
# Rodar com debug
gdb --args ./MicroHypervisor
(gdb) run
(gdb) backtrace  # Ver onde falhou
```

### Erro: Compilação falha com "undefined reference"

**Causa**: Falta linkar algum objeto ou biblioteca.

**Solução**:
```bash
make clean
make all
# Se falhar, verificar se todos os .asm estão sendo compilados
```

## Arquitetura em Detalhes

### Fluxo de Inicialização

```
_start (entry.asm)
    ↓
check_vmx_support
    ↓
enable_cr4_vmx
    ↓
setup_page_tables
    ↓
init_simd_state
    ↓
hypervisor_init (C++)
    ↓
vmx_init
    ↓
vmcs_setup
    ↓
vmx_launch → Transfer controle para guest
```

### Estrutura de Memória

```
0x100000  - Código e dados do hypervisor
0x200000  - VMXON region (4KB aligned)
0x201000  - VMCS region (4KB aligned)
0x202000  - Stack (16KB)
0x210000  - Enclave (1MB)
0x410000  - Trace buffer
```

### Campos VMCS Importantes

- **Guest State**: CR0, CR3, CR4, RIP, RSP, RFLAGS
- **Host State**: CR0, CR3, CR4, RIP (vmexit_handler), RSP
- **Control Fields**: Pin-based, Proc-based, VM-exit, VM-entry controls

## Performance Optimization

### Flags de Compilação

```bash
# Maximum performance
cmake -DCMAKE_BUILD_TYPE=Release ..
make CFLAGS="-O3 -march=native -mtune=native"

# Profile-guided optimization (avançado)
cmake -DCMAKE_BUILD_TYPE=Release ..
make
./MicroHypervisor  # Rodar com workload típico
make gcov
make clean
make CFLAGS="-O3 -march=native -fprofile-use"
```

### Ajustes de Runtime

No `src/main.cpp`:
```cpp
// Aumentar tamanho do buffer de data
constexpr uint64_t DATA_BUFFER_SIZE = 0x20000;  // 128KB

// Ajustar intervalo de heartbeat
constexpr uint64_t HEARTBEAT_INTERVAL_MS = 500;
```

## Segurança

### Melhores Práticas

1. **Sempre verifique integridade** antes de confiar no enclave
2. **Use RDRAND** para números aleatórios criptograficamente seguros
3. **Zero memória** sensível antes de liberar
4. **Valide todos os inputs** do guest

### Auditoria

```bash
# Verificar se não há dados vazando na pilha
objdump -d MicroHypervisor | grep -A 20 "call.*pack"

# Verificar se VMCS está configurado corretamente
gdb --args ./MicroHypervisor
(gdb) break vmcs_setup
(gdb) run
(gdb) vmread 0x6800  # Ver Guest CR0
(gdb) vmread 0x4402  # Ver exit reason
```

## Extensões

### Adicionando Novo VMExit Handler

1. Edite `src/monitor/intercept.asm`
2. Adicione em `vmexit_dispatch`:
```asm
cmp rdi, EXIT_REASON_SEU_EXIT
je .handle_seu_exit
```

3. Implemente o handler:
```asm
.handle_seu_exit:
    ; Seu código aqui
    xor eax, eax
    ret
```

### Adicionando Operação SIMD

1. Edite `src/marshall/simd_packer.asm`
2. Use macros de `include/avx_macros.inc`:
```asm
global minha_operacao_simd
minha_operacao_simd:
    SIMD_PACK_INIT
    ; Seu código AVX aqui
    vzeroupper
    ret
```

## Recursos Adicionais

- Intel SDM Volume 3C: https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
- NASM Documentation: https://www.nasm.us/docs.php
- QEMU Documentation: https://www.qemu.org/docs/master/
- OSDev Wiki: https://wiki.osdev.org/

## Suporte

Para bugs ou questões:
1. Verifique a documentação
2. Procure issues similares
3. Abra um novo issue com detalhes completos
