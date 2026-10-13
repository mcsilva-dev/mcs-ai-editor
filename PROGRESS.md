# Progresso — MCS AI Editor

Spec: [docs/superpowers/specs/2026-10-13-mcs-ai-editor-design.md](docs/superpowers/specs/2026-10-13-mcs-ai-editor-design.md)

## Estado atual

- **Fase:** design (Subprojeto 1 — Núcleo + Edit)
- **Próximo passo:** Miguel revisar o plano do v0.1 e escolher modo de execução → executar
- **Plano atual:** [docs/superpowers/plans/2026-10-13-v0.1-esqueleto.md](docs/superpowers/plans/2026-10-13-v0.1-esqueleto.md)

## Histórico

### 2026-10-13 — Design
- Feito: brainstorming das 6 seções aprovado; spec escrita; repositório criado.
- Decisões: abordagem A (motor próprio, grafo GPU via QRhi); C++20 + Qt 6 + FFmpeg;
  GPLv3; Windows primeiro; Estúdio IA próprio (BYOK, fal.ai primeiro) como Subprojeto 2;
  REA só para inventário clean-room.
- Verificado: n/a (só documentos).
- Spec aprovada pelo Miguel; Subprojeto 0 ampliado para decomposição clean-room do Resolve
  em 4 frentes (mapa estrutural via REA, inventário via API de scripting, formatos em disco,
  testes de caixa-preta com o Resolve como oráculo). Descompilar algoritmos segue proibido.

### 2026-10-13 — Plano v0.1
- Feito: plano do v0.1 com 6 tasks (higiene, toolchain, mcs_media, janela, CI, fechamento).
- Ambiente levantado: VS 2026 Community (MSVC 14.52) + vcpkg embutido no VS; CMake 4.4.4;
  Ninja 1.13.2; clang-format 22.1.3; FFmpeg CLI 9.0.1; MinGW GCC 15.2 (não usado).
  Qt ausente → será instalado 6.11.3 msvc2022_64 em C:\Qt via aqtinstall (Task 2).
- Decisões: Qt 6.11.3 (não 6.12.0 — evitar .0); vcpkg baseline e456309491fd875487bf02b8a0dca76c1b1b00cc;
  FFmpeg só avcodec/avformat/swscale/swresample no v0.1; UI em inglês via tr().
