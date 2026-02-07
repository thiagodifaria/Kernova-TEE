# Micro-Hypervisor - Interfaces de Gerenciamento

Esta pasta contém as interfaces para gerenciar o Micro-Hypervisor.

## CLI (Command Line Interface)

### Uso Básico

```bash
# Menu interativo (recomendado para começar)
python3 interface/cli.py

# Comandos diretos
python3 interface/cli.py build              # Compilar projeto
python3 interface/cli.py build --clean      # Rebuild
python3 interface/cli.py test               # Rodar testes
python3 interface/cli.py run                # Rodar no QEMU
python3 interface/cli.py run --debug        # Debug mode
python3 interface/cli.py status             # Ver status
python3 interface/cli.py clean              # Limpar build
```

### Exemplos

```bash
# Fluxo completo de desenvolvimento
python3 interface/cli.py build --clean      # Limpa e compila
python3 interface/cli.py test               # Roda testes
python3 interface/cli.py run                # Executa no QEMU

# Desenvolvimento iterativo
python3 interface/cli.py build              # Compila rápido
python3 interface/cli.py run --debug        # Debug com GDB
```

## Web Interface (Dashboard)

### Iniciar

```bash
# Modo 1: Direto
python3 interface/web.py

# Modo 2: Via Docker
docker-compose up web

# Acesse: http://localhost:5000
```

### Funcionalidades

- **Status Monitor**: Ver status do projeto em tempo real
- **Build Control**: Compile com um clique
- **Test Runner**: Execute testes e veja resultados
- **System Info**: Monitor CPU e memória
- **Auto-refresh**: Atualização automática a cada 5s

## Docker

### Build

```bash
docker build -t microhypervisor:latest .
```

### Run

```bash
# Shell interativo
docker run -it --rm -v $(pwd):/workspace microhypervisor:latest

# Rodar build
docker run -it --rm -v $(pwd):/workspace microhypervisor:latest build

# Rodar testes
docker run -it --rm -v $(pwd):/workspace microhypervisor:latest test
```

### Docker Compose

```bash
# Build e start todos serviços
docker-compose up -d

# Entrar no container builder
docker-compose run builder

# Rodar testes
docker-compose run tester

# Web interface
docker-compose up web
```

## Scripts Automatizados

### build.sh

Script completo de build com instalação de dependências:

```bash
# Instalar dependências + build
./scripts/build.sh --all

# Apenas build
./scripts/build.sh --build

# Build + testes
./scripts/build.sh --build --test

# Help
./scripts/build.sh --help
```

### test.sh

Roda todos os testes automaticamente:

```bash
./scripts/test.sh
```

## Fluxo de Trabalho Recomendado

### Desenvolvimento Local

```bash
# 1. Menu interativo para começar
python3 interface/cli.py

# 2. Ou comandos diretos
python3 interface/cli.py build --clean
python3 interface/cli.py test
```

### Com Docker (Ambiente Isolado)

```bash
# 1. Start ambiente
docker-compose up -d

# 2. Entrar no container
docker-compose run builder

# 3. Trabalhar no projeto
./scripts/build.sh --all
./scripts/test.sh
```

### Com Web Dashboard

```bash
# 1. Iniciar dashboard
python3 interface/web.py

# 2. Acessar http://localhost:5000

# 3. Usar interface web para build/testes
```

## Troubleshooting

### CLI não encontra Python

```bash
# Ubuntu/Debian
sudo apt-get install python3 python3-pip

# Instalar Flask
pip3 install Flask psutil
```

### Docker sem permissão

```bash
# Adicionar usuário ao grupo docker
sudo usermod -aG docker $USER
newgrp docker
```

### QEMU não instalado

```bash
# Ubuntu/Debian
sudo apt-get install qemu-system-x86 kvm

# macOS
brew install qemu

# Verificar instalação
qemu-system-x86_64 --version
```

### Build falha

```bash
# Limpar tudo e tentar novamente
python3 interface/cli.py clean
python3 interface/cli.py build --clean
```

## Arquitetura das Interfaces

```
interface/
├── cli.py          # CLI principal
├── web.py          # Web dashboard
├── templates/      # HTML templates (auto-gerado)
└── README.md       # Este arquivo

scripts/
├── build.sh        # Build automatizado
├── test.sh         # Testes automatizados
└── docker-entrypoint.sh

docker-compose.yml  # Orquestração Docker
Dockerfile          # Imagem Docker
```

## Limitações

**Docker**: VT-x não funciona dentro de containers
- Docker é usado apenas para build e testes unitários
- Para testar VT-x real, use máquina virtual ou hardware

**Web Interface**: Monitoramento apenas
- Não roda hypervisor via web
- Apenas gerencia build/testes/status

## Próximos Passos

- [ ] Adicionar logging mais detalhado
- [ ] Implementar hot-reload para desenvolvimento
- [ ] Adicionar métricas de performance
- [ ] Criar pacotes de release
- [ ] Integrar com CI/CD
