# MCS AI Editor

Editor de vídeo open source (GPLv3) com o conjunto de ferramentas do DaVinci Resolve como
referência funcional e geração de vídeo por IA integrada.

> Status: v0.1 (esqueleto). Ver [PROGRESS.md](PROGRESS.md).

## Compilar (Windows)

Pré-requisitos: Visual Studio 2026 com C++ (inclui vcpkg), CMake ≥ 3.25, Ninja,
Qt 6.11.3 `msvc2022_64` em `C:\Qt` (ver `scripts/install-qt.cmd`).

    scripts\dev.cmd cmake --preset debug
    scripts\dev.cmd cmake --build --preset debug
    scripts\dev.cmd ctest --preset debug

## Licença

GPLv3 — ver [LICENSE](LICENSE) e [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
