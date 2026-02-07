# 🚀 Quick Start Guide

Guia rápido para começar com o Micro-Hypervisor em 5 minutos.

## Opção 1: Interface CLI (Mais Fácil)

### Passo 1: Instalar Dependências

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake nasm qemu-system-x86 python3

# macOS
brew install cmake nasm qemu python3

# Arch Linux
sudo pacman -S base-devel cmake nasm qemu-system-x86 python
```

### Passo 2: Usar a CLI

```bash
# Menu interativo (recomendado)
python3 interface/cli.py

# Ou comandos diretos
python3 interface/cli.py build --clean    # Compilar
python3 interface/cli.py test             # Testar
python3 interface/cli.py run              # Rodar no QEMU
```

## Opção 2: Docker (Ambiente Isolado)

### Passo 1: Instalar Docker

```bash
# Ubuntu
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker $USER

# Reiniciar sessão
```

### Passo 2: Usar Docker

```bash
# Build
docker-compose build

# Rodar testes
docker-compose run tester

# Shell para desenvolvimento
docker-compose run builder
```

## Opção 3: Web Dashboard (Visual)

### Passo 1: Instalar Python + Flask

```bash
sudo apt-get install python3 python3-pip
pip3 install Flask psutil
```

### Passo 2: Iniciar Dashboard

```bash
python3 interface/web.py
# Acesse: http://localhost:5000
```

Use a interface web para:
- Compilar o projeto
- Rodar testes
- Ver status do sistema
- Monitorar recursos

## Opção 4: Scripts Automatizados

### Build Completo

```bash
# Instalar dependências + build + testes
chmod +x scripts/build.sh
./scripts/build.sh --all
```

### Testes

```bash
chmod +x scripts/test.sh
./scripts/test.sh
```

## Verificação Rápida

```bash
# 1. Ver estrutura
ls -la src/ include/ tests/

# 2. Ver dependências
which gcc nasm cmake qemu-system-x86_64

# 3. Compilar
python3 interface/cli.py build

# 4. Ver binário
ls -lh build/MicroHypervisor

# 5. Rodar testes
python3 interface/cli.py test
```

## Troubleshooting Rápido

### "command not found: python3"

```bash
# Ubuntu/Debian
sudo apt-get install python3

# Criar alias
echo "alias python=python3" >> ~/.bashrc
source ~/.bashrc
```

### "VT-x not supported"

```bash
# Verificar suporte
cat /proc/cpuinfo | grep vmx

# Se vazio, habilite VT-x na BIOS:
# 1. Entre na BIOS
# 2. Procure "Intel Virtualization Technology"
# 3. Habilite
# 4. Salve e reinicie
```

### "Docker permission denied"

```bash
sudo usermod -aG docker $USER
newgrp docker
```

### "Build failed"

```bash
# Limpar e tentar novamente
python3 interface/cli.py clean
python3 interface/cli.py build --clean
```

## Próximos Passos

1. **Leia a documentação completa**: [README.md](README.md)
2. **Entenda a arquitetura**: [docs/INSTRUCTIONS.md](docs/INSTRUCTIONS.md)
3. **Explore o código**: Comece em [src/main.cpp](src/main.cpp)
4. **Contribua**: Veja issues e pull requests

## Exemplos de Uso

### Desenvolvimento Iterativo

```bash
# Editar código
vim src/main.cpp

# Rebuild rápido
python3 interface/cli.py build

# Testar
python3 interface/cli.py test

# Rodar
python3 interface/cli.py run
```

### Debugging

```bash
# Terminal 1: Iniciar QEMU em modo debug
python3 interface/cli.py run --debug

# Terminal 2: Conectar GDB
gdb build/MicroHypervisor
(gdb) target remote :1234
(gdb) break _start
(gdb) continue
```

### Criação de ISO

```bash
# Build
python3 interface/cli.py build

# Criar ISO
cd build
make iso

# Rodar ISO
qemu-system-x86_64 -m 2G -cdrom MicroHypervisor.iso
```

## Suporte

- **Issues**: GitHub Issues
- **Docs**: [docs/](docs/)
- **CLI Help**: `python3 interface/cli.py --help`

---

**Tempo estimado**: 5-10 minutos para setup completo

**Dificuldade**: ⭐⭐☆☆☆ (Fácil/Média)

**Pré-requisitos**: Ubuntu 20.04+, macOS 11+, ou Arch Linux
