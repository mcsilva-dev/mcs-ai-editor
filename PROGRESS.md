# Progresso — MCS AI Editor

Spec: [docs/superpowers/specs/2026-10-13-mcs-ai-editor-design.md](docs/superpowers/specs/2026-10-13-mcs-ai-editor-design.md)

## Estado atual

- **Fase:** v0.1.0 concluído (Subprojeto 1 — Núcleo + Edit)
- **Próximo passo:** plano do v0.2 (importar mídia + Media Pool + viewer com decode)
- **Último plano:** [docs/superpowers/plans/2026-10-13-v0.1-esqueleto.md](docs/superpowers/plans/2026-10-13-v0.1-esqueleto.md)

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

### 2026-10-13 — v0.1.0 (esqueleto)
- Feito: higiene do repo; toolchain (Qt 6.11.3 via aqtinstall, FFmpeg 9.0.2 via vcpkg, presets
  CMake, `scripts/dev.cmd`); `mcs_media` (versão/decoders FFmpeg); janela principal (Edit/Deliver,
  tema escuro, `--smoke`); CI GitHub Actions.
- Verificado: build do zero em 24 s (vcpkg restaurou 7 pacotes do cache em 0,8 s);
  `ctest` 3/3 (tst_toolchain, tst_mainwindow, app_starts); exe roda com PATH limpo (windeployqt);
  YAML do CI válido; clang-format sem diferenças.
- Não verificado: CI (não há repositório remoto).
- Observações/armadilhas:
  - FFmpeg do zero via vcpkg: ~25 min (rel+dbg, 1885 objetos cada); depois vem do cache binário.
  - aqtinstall do PyPI (3.3.0) não suporta o layout de repo do Qt 6.11+; usar commit do GitHub.
  - `py -3.13` não enxerga Python instalado pelo uv; script usa `py -3`.
  - `vcpkg`/MSVC: `vcvars64.bat` chama `vswhere.exe` pelo nome; `dev.cmd` põe o Installer no PATH.
  - `qt_standard_project_setup` põe todos os .exe em `build/<preset>/`; o windeployqt do app
    copia o Qt para lá, por isso os testes definem `QT_PLUGIN_PATH`.
  - Folha de estilo global (`QApplication::setStyleSheet`) embrulha o estilo do app; usar só no widget.
