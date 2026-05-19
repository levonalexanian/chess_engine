FROM debian:12-slim

LABEL debian.version="12"

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

# --- Base OS + C++ toolchain ------------------------------------------------
RUN apt-get update && apt-get install -y --no-install-recommends \
    bash \
    build-essential \
    cmake \
    ninja-build \
    clang-format \
    clang-tidy \
    git \
    curl \
    ca-certificates \
    gnupg \
    pkg-config \
    libssl-dev \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# --- Node.js (pinned major) --------------------------------------------------
# Node stays in the image for the frontend (Vite + React) build.
ENV NODE_MAJOR=22
RUN curl -fsSL https://deb.nodesource.com/setup_${NODE_MAJOR}.x | bash - \
    && apt-get update \
    && apt-get install -y --no-install-recommends nodejs \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Pin npm to a recent LTS to match what later commits' lockfile expects.
RUN npm install -g npm@10.9.0

# --- Python 3.11 + pip -------------------------------------------------------
# Debian 12 ships python 3.11 in apt, which Conan 2 and the training stack run on.
RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 python3-pip python3-venv python-is-python3 \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Create the non-root vscode user that devcontainer.json expects.
RUN useradd -m vscode || true
USER vscode

# Install Conan 2.x as the vscode user so it lands in $HOME/.local/bin.
ENV PATH="/home/vscode/.local/bin:${PATH}"
RUN pip install --user --break-system-packages "conan>=2,<3"

# Pre-create the conan2 cache dir owned by vscode so the named volume in
# docker-compose.yml inherits the right ownership on first run.
RUN mkdir -p /home/vscode/.conan2

WORKDIR /home/vscode/workspace
