# =============================================================================
# Micro-Hypervisor - Docker Build Environment
# =============================================================================
# Nota: VT-x não funciona dentro de containers, mas podemos usar
# Docker para build, testes unitários e CI/CD
# =============================================================================

FROM ubuntu:22.04

# Metadata
LABEL maintainer="thiag"
LABEL description="Micro-Hypervisor build environment"
LABEL version="1.0"

# Prevenir prompts interativos
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Instalar dependências de build
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    nasm \
    qemu-system-x86 \
    qemu-utils \
    gdb \
    python3 \
    python3-pip \
    git \
    wget \
    curl \
    vim \
    strace \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

# Instalar ferramentas adicionais
RUN pip3 install \
    pytest \
    pytest-cov \
    Flask \
    requests

# Criar usuário não-root para desenvolvimento
RUN useradd -m -s /bin/bash developer && \
    echo "developer ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Criar diretórios de trabalho
WORKDIR /workspace
RUN mkdir -p /workspace/build /workspace/tests /workspace/docs

# Configurar ambiente
ENV PATH="/workspace/build:${PATH}"
ENV PKG_CONFIG_PATH="/usr/local/lib/pkgconfig"

# Copiar scripts
COPY scripts/docker-entrypoint.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

# Expor portas para interface web (se implementada)
EXPOSE 5000

# Set entrypoint
ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
CMD ["bash"]
