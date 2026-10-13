# Changelog

Formato baseado em [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/);
versionamento [SemVer](https://semver.org/lang/pt-BR/).

## [Não lançado]

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
