# Changelog

Formato baseado em [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/);
versionamento [SemVer](https://semver.org/lang/pt-BR/).

## [Não lançado]

## [0.4.0] - 2026-10-14

### Adicionado
- Edição: blade no playhead (B), apagar com e sem fechar a lacuna (Delete / Shift+Delete),
  mover clipes arrastando com ímã (N liga/desliga), insert/overwrite a partir do viewer de fonte
  com In/Out (I/O, F9/F10); vídeo e áudio ligados andam juntos, com Linked Selection
  (Ctrl+Shift+L) para mexer só no áudio ou só no vídeo.
- Volume por clipe de áudio: Ctrl+Alt+= / Ctrl+Alt+− (±1 dB), exibido no clipe.
- Desfazer/refazer qualquer edição (Ctrl+Z, Ctrl+Shift+Z/Ctrl+Y).
- Projeto: New/Open/Save/Save As (.mcsproj em JSON versionado), gravação atômica, 3 backups e
  autosave a cada 2 minutos; mídia ausente ao abrir gera aviso sem perder a timeline.
- Testes de edição em tabela com timelines em texto.

## [0.3.0] - 2026-10-13

### Adicionado
- Timeline com trilhas V1/A1: append no fim por duplo clique no Media Pool ou Shift+F12.
- Playback com o áudio como relógio mestre (miniaudio): Espaço = play/pause; clique/arrasto
  na timeline = seek; viewer de programa com timecode.
- Decode de áudio para estéreo 48 kHz; decode de vídeo sequencial sem seek.
- Teste de sincronia A/V de 10 minutos (tempo virtual) e playback real no dispositivo simulado.

### Mudado
- Saída de áudio: miniaudio no lugar do QAudioSink; viewer na GPU passa ao v0.6 e decode por
  hardware ao v0.8 (spec atualizada).

## [0.2.0] - 2026-10-13

### Adicionado
- Importar mídia por menu (File > Import Media, Ctrl+I) e arrastando arquivos para o Media Pool.
- Media Pool com thumbnails assíncronos, metadados (resolução, fps, duração, codecs) e
  detecção de duplicados; erros claros para arquivos que não são mídia.
- Viewer de fonte com scrub quadro a quadro e timecode.
- Núcleo: tempo racional e timecode; probe de mídia com metadados de cor; decode com seek exato.
- Testes com mídia sintética gerada pelo próprio FFmpeg (8 suítes).

### Corrigido
- CI do Windows usa o vcpkg embutido no Visual Studio (#1).

## [0.1.0] - 2026-10-13

### Adicionado
- Toolchain: C++20, Qt 6.11.3, FFmpeg 9.0.2 via vcpkg (baseline fixado), presets CMake.
- Janela principal com páginas Edit e Deliver e tema escuro.
- Testes: toolchain (Qt/FFmpeg/UTF-8), janela principal, inicialização do executável.
- CI GitHub Actions (Windows, Linux com ASan, macOS, formatação) — ainda não executado.
- Spec de design do Subprojeto 1; licença GPLv3; regras clean-room.
