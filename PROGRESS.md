# Progresso — MCS AI Editor

Spec: [docs/superpowers/specs/2026-10-13-mcs-ai-editor-design.md](docs/superpowers/specs/2026-10-13-mcs-ai-editor-design.md)

## Estado atual

- **Fase:** v0.4.0 concluído (Subprojeto 1 — Núcleo + Edit)
- **Próximo passo:** plano do v0.5 (trilha A2 primeiro; depois trims ripple/roll/slip/slide e JKL)
- **Último plano:** [docs/superpowers/plans/2026-10-14-v0.4-edicao.md](docs/superpowers/plans/2026-10-14-v0.4-edicao.md)
- **Repositório:** https://github.com/mcsilva-dev/mcs-ai-editor (main protegida: PR + CI verde)

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

### 2026-10-13 — Publicação e CI
- Repositório público criado; e-mail dos commits trocado para o noreply do GitHub antes do 1º push.
- 1º CI: macOS, Linux (ASan) e formatação verdes; Windows falhou porque o `msvc-dev-cmd`
  exporta `VCPKG_ROOT` do Visual Studio. Corrigido no PR #1 (Windows usa o vcpkg do VS).
- `main` protegida (PR obrigatório, 4 checks obrigatórios, sem force push), valendo para admin.

### 2026-10-13 — v0.2.0 (import, Media Pool, viewer)
- Feito: `mcs_core` (racional, timecode, MediaInfo); `probe` com metadados de cor; `FrameDecoder`
  com seek exato; `MediaPoolModel` (duplicados, erros, thumbnails via QtConcurrent); página Edit
  com Media Pool, viewer de fonte (QPainter), Ctrl+I e arrastar-e-soltar.
- Verificado: build limpo em 26 s; `ctest` 8/8; exe roda com PATH limpo.
- Observações/armadilhas:
  - Gerador sintético: o encoder mpeg4 transforma todo quadro em keyframe (detecção de corte de
    cena) → `sc_threshold` alto; e a duração precisa ir no pacote, senão o .mov descarta o último
    quadro (edit list). Teste `synthHasAllFramesAndRealGop` protege isso (60 pacotes, 5 keyframes).
  - Qt só entrega Drop ao widget que aceitou o DragEnter (testes devem enviar os dois).
  - Com 20 núcleos o decoder em threads só solta quadros no esvaziamento final (normal).
  - CI roda duas vezes (push + pull_request) para o mesmo código: melhoria futura.

### 2026-10-13 — v0.3.0 (timeline + playback sincronizado)
- Feito: modelo de timeline (V1/A1, append ligado, conversões quadro↔amostra); `AudioDecoder`
  (estéreo float 48 kHz); `FrameDecoder` sequencial sem seek; `mcs_playback` (TimelineVideo,
  TimelineAudio, PlaybackEngine com buffer circular SPSC e áudio como relógio, AudioOutput com
  miniaudio, Player com relógio de fallback); UI com timeline, viewer de programa, Shift+F12 e Espaço.
- Decisões (Miguel): miniaudio no lugar do QAudioSink; GPU/QRhi no v0.6; decode HW no v0.8.
- Verificado: ctest 14/14; sincronia A/V de 10 min em tempo virtual (1,2 s de teste); playback real
  no backend nulo do miniaudio; exe roda com PATH limpo.
- Observações/armadilhas:
  - Decode em várias threads: o decoder entra em "esvaziamento" ~19 quadros antes do fim do
    arquivo e ainda solta quadros — só EOF exige seek (antes: 15 seeks em 60 quadros sequenciais).
  - Áudio sintético dos testes é uma rampa por amostra: prova amostra exata no mixer e no motor.

### 2026-10-14 — v0.4.0 (edição básica e arquivo de projeto)
- Feito: `edit_ops` puro (blade, lift, ripple delete, insert/overwrite de trecho da fonte, mover,
  ímã, volume por clipe); `Clip` com `link` e `gainDb`; `mcs_edit` com `TimelineDocument` (ponto
  único de edição, undo/redo por snapshot); `.mcsproj` JSON versionado (gravação atômica, 3
  backups, autosave de 2 min); timeline interativa (seleção, Ctrl alterna, arrastar com ímã,
  Linked Selection); atalhos B, Delete, Shift+Del, N, Ctrl+Shift+L, Ctrl+Alt+=/−, I, O, F9, F10;
  menus New/Open/Save/Save As e Undo/Redo; título com `*` para alterações não salvas.
- Decisões (Miguel): edição básica + projeto no v0.4; Linked Selection para tirar/baixar o áudio
  de um vídeo e pôr outro (F10 com arquivo só de áudio substitui o trecho em A1); trilha A2
  adiada para o início do v0.5; Subprojeto 7 (controle por IA via MCP) registrado na spec —
  o `TimelineDocument` já é o ponto de entrada que o MCP vai usar.
- Verificado: ctest 18/18; exe com PATH limpo (`--smoke`).
- Observações/armadilhas:
  - `~QUndoStack` chama `clear()`, que emite `cleanChanged` se havia alterações: ao fechar a janela
    com mudanças não salvas o slot rodava num objeto meio destruído (0xc0000409). O documento
    bloqueia os sinais da pilha no destrutor.
  - Testes de edição em tabela com timelines em texto (`V1: A[0..10)@3 | A1: a[0..10)`) — barato
    escrever casos novos.
