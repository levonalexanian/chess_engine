COMPOSE ?= docker compose
DEV     := $(COMPOSE) run --rm -T dev
DEV_TTY := $(COMPOSE) run --rm --service-ports dev

TRAIN_ARGS := $(filter-out $@,$(MAKECMDGOALS))

.PHONY: help image-build install build typecheck test web train sh down clean

help:
	@echo "Chess engine — host-side targets (all real work runs inside the dev container)."
	@echo ""
	@echo "Setup:"
	@echo "  make image-build  Build the dev image from .devcontainer/Dockerfile"
	@echo "  make install      conan install backend/ + npm install frontend/ + pip install -e training/"
	@echo ""
	@echo "Develop:"
	@echo "  make build        CMake configure + build of backend/"
	@echo "  make typecheck    tsc --noEmit on the frontend"
	@echo "  make test         ctest + vitest + pytest training/"
	@echo "  make web          Build the frontend and serve chess-server on :8080"
	@echo "  make train        Run training/scripts/train.py (pass args after --)"
	@echo "  make sh           Interactive shell in the dev container"
	@echo ""
	@echo "Teardown:"
	@echo "  make down         Stop+remove any compose containers"
	@echo "  make clean        down + remove the local image"

image-build:
	$(COMPOSE) build dev

install:
	$(DEV) bash -c 'cd backend && conan install . --profile:host=default --profile:build=default --build=missing && cd ../frontend && npm install && cd ../training && pip install --user -e .'

build:
	$(DEV) bash -c 'cmake --preset dev -S backend && cmake --build backend/build/dev'

typecheck:
	$(DEV) bash -c 'cd frontend && npm run typecheck'

test:
	$(DEV) bash -c 'ctest --test-dir backend/build/dev --output-on-failure && cd frontend && npm run test && cd .. && pytest training/'

web:
	$(DEV_TTY) bash -c 'cd frontend && npm run build && cd /home/vscode/workspace && ./backend/build/dev/server/chess-server'

train:
	$(DEV) bash -c 'python training/scripts/train.py $(TRAIN_ARGS)'

sh:
	$(DEV_TTY) bash

down:
	$(COMPOSE) down --remove-orphans

clean:
	$(COMPOSE) down --rmi local --remove-orphans

%:
	@:
