# MCS AI Editor — Design (Subprojeto 1: Núcleo + Edit)

- **Data:** 2026-10-13
- **Status:** aprovado (2026-10-13), com Subprojeto 0 ampliado
- **Autor:** Miguel Silva (com Claude)

## 1. Objetivo

Editor de vídeo **open source (GPLv3)**, distribuído publicamente, com o conjunto de
ferramentas do DaVinci Resolve 21 como referência funcional e geração de vídeo por IA
integrada (nível Higgsfield) como diferencial.

Este documento cobre o **Subprojeto 1 — Núcleo + Edit (v0.1 → v1.0)** e registra as
decisões transversais que valem para os demais subprojetos.

**Sucesso do v1.0:** no Windows, editar um vídeo 4K H.264/H.265 de ponta a ponta
(importar, editar com trims avançados, transições, títulos, keyframes) com playback em
tempo real (via proxy quando necessário) e exportar sem crash.

## 2. Regras inegociáveis (clean-room)

- **Nenhum código, binário descompilado, asset, preset ou prompt** da Blackmagic
  (DaVinci Resolve) ou do Higgsfield entra no repositório.
- O Resolve é referência de **funcionalidade**, não de implementação. Fontes permitidas:
  manual oficial público, documentação pública de scripting, observação da interface e
  listagem de nomes de arquivos instalados.
- O **REA** e a análise do Resolve ficam no Subprojeto 0 (decomposição clean-room, ver
  seção 3.1). Permitido: estrutura, nomes, parâmetros, formatos e comportamento
  observável. Proibido: descompilar algoritmos e reimplementar a partir do pseudocódigo
  (obra derivada — risco de DMCA para o projeto GPL inteiro; vedado pelo EULA).
- Interoperabilidade com o Resolve apenas por formatos abertos que ele exporta:
  **OpenTimelineIO, FCPXML, EDL**. O formato `.drp` não é suportado.

## 3. Roadmap de subprojetos

| # | Subprojeto | Observação |
|---|---|---|
| 0 | Decomposição clean-room do Resolve | gera `docs/inventory/` + testes de comportamento (seção 3.1) |
| 1 | **Núcleo + Edit** | este documento |
| 2 | Estúdio IA | spec própria; primeiro provedor **fal.ai** |
| 3 | Color | nós de cor, OpenColorIO, scopes |
| 4 | Fairlight | mixer, efeitos, automação de áudio |
| 5 | Fusion | composição por nós, keying, tracking |
| 6 | Cut | interface rápida + atalhos de IA |

Cada subprojeto tem ciclo próprio: spec → plano → implementação.

### 3.1 Subprojeto 0 — Decomposição clean-room do Resolve

Objetivo: entender como o Resolve funciona e medir o quanto nos aproximamos dele, por
quatro frentes, sem copiar código:

1. **Mapa estrutural (REA):** módulos/DLLs, imports/exports, grafo de dependências,
   bibliotecas de terceiros embutidas → como ele se divide em subsistemas.
2. **Inventário via API de scripting (Python/Lua):** todas as ferramentas do Fusion com
   parâmetros, tipos, padrões e faixas; presets de render; codecs → `docs/inventory/`.
3. **Formatos e comportamento em disco:** caches, proxies, optimized media, estrutura do
   banco de projeto → organização interna e interoperabilidade.
4. **Testes de caixa-preta (Resolve como oráculo):** nossa mídia sintética renderizada no
   Resolve; medições (curvas de cor, curva de transições, quadro exato de cortes/trims)
   viram testes que o MCS AI Editor deve passar.

Saídas: documentos de requisitos e medições numéricas. Nenhum arquivo da instalação do
Resolve vai para o repo. Frentes 2 e 4 exigem Resolve instalado e licenciado na máquina
do analista. Roda em paralelo aos marcos do Subprojeto 1; a frente 4 alimenta v0.4+.

### 3.2 Estúdio IA (resumo das decisões, detalhado em spec futura)

- O software **gera os vídeos ele mesmo**, chamando os modelos diretamente com a **chave
  de API do próprio usuário** (BYOK). Não depende do Higgsfield.
- Qualidade equivalente vem de usar os **mesmos modelos de base** (Kling, Seedance, Veo,
  Wan, MiniMax, LTX...) + uma camada de fluxo **nossa** (presets de câmera, prompt
  enhancer, referências de personagem, multi-shot) + pós-processamento local (upscale,
  interpolação de FPS, deflicker).
- Arquitetura com provedores intercambiáveis (enviar / status / baixar / estimar custo).
  Ordem: **fal.ai** (primeiro) → APIs oficiais diretas → ComfyUI local → Replicate.
- Chave guardada no cofre do SO (Windows Credential Manager); custo estimado e
  confirmado antes de cada geração; resultado baixado imediatamente e salvo no projeto
  com metadados (provedor, modelo, prompt, custo, request_id).
- Módulo opcional: o editor funciona 100% sem ele.

## 4. Stack

| Camada | Escolha | Motivo |
|---|---|---|
| Linguagem | C++20 | performance; padrão dos NLEs |
| UI | Qt 6 Widgets | layout denso e profissional; versão do Qt fixada |
| GPU | Qt RHI (`QRhi`, `QRhiWidget`) | D3D11/12, Vulkan, Metal com um código só |
| Decode/encode | FFmpeg (libav*) | padrão da indústria; build GPL completo (x264/x265 permitidos) |
| Interop | OpenTimelineIO (C++) | troca de timelines; FCPXML/EDL escritos em C++ |
| Áudio | miniaudio (vcpkg, Unlicense/MIT-0) | baixa latência, contagem exata de amostras, backend "nulo" para testar no CI (decidido 2026-10-13: QAudioSink exigiria o módulo Qt Multimedia, que embute outro FFmpeg) |
| Undo | `QUndoStack` | já incluso no Qt |
| Testes | Qt Test | já incluso no Qt |
| Dependências | Qt via instalador oficial / `aqtinstall`; demais via **vcpkg (manifesto)** | versões fixadas no repo |
| Build | CMake + Ninja | |

Plataformas: **Windows primeiro** (release oficial); Linux e macOS compilados no CI desde
o v0.1.

## 5. Arquitetura

```
src/core/      modelo: Projeto, MediaPool, Timeline, Track, Clip, tempo racional
src/edit/      operações de edição puras + comandos de undo
src/media/     FFmpeg: probe, decode HW/SW, cache de quadros, proxies
src/render/    grafo de render GPU (QRhi + shaders)
src/playback/  relógio, agendamento de quadros, saída de áudio
src/interop/   OTIO / FCPXML / EDL (só na fronteira de import/export)
src/ui/        Media Pool, Timeline, Viewers, Inspector, Deliver
tests/
```

Dependências apontam para dentro: `ui → playback/edit → render/media → core`.
`core` não depende de Qt Widgets nem de FFmpeg.

### 5.1 Grafo de render

Cada quadro da timeline é avaliado como um grafo:
`decode → transform/crop → (transição) → composite → saída`. No v1 o grafo é montado
automaticamente a partir da timeline (sem editor de nós visível). Color e Fusion
exporão o mesmo grafo.

Texturas internas em **RGBA16F**; processamento em Rec.709 no v1 (espaço linear/HDR
fica para o Subprojeto Color, sem refazer o pipeline).

### 5.2 Threads e relógio

UI · pool de decode · render · áudio. **Áudio é o relógio mestre**: relógio = amostras
tocadas / sample rate − latência do dispositivo. Vídeo atrasado > 1 quadro é descartado;
adiantado espera. Sem áudio, usa `steady_clock`.

## 6. Pipeline de mídia

1. **Probe:** duração, fps racional, resolução, codec e metadados de cor
   (primaries/transfer/matrix/range) guardados desde o import.
2. **Decode:** HW (D3D11VA no Windows; VAAPI/NVDEC no Linux; VideoToolbox no macOS),
   fallback SW multi-thread. No v1 o quadro HW é copiado para RAM e reenviado à GPU
   (conversão YUV→RGB em shader). *Teto conhecido:* custo de banda; upgrade = zero-copy
   D3D11VA↔QRhi.
3. **Seek** preciso ao quadro: keyframe anterior + decode até o alvo.
4. **Cache LRU** de quadros com orçamento de memória; **prefetch** de N quadros à frente
   na direção do playback.
5. **Áudio:** decode → resample (48 kHz float) → mix das trilhas → ring buffer → sink.
6. **Proxies (v0.8):** transcodificação em segundo plano para codec all-intra em
   resolução reduzida; alternância "Usar proxies"; export sempre do original.
7. **Render cache (v0.8):** trechos renderizados em disco, chave = hash dos parâmetros do
   sub-grafo; mudança de parâmetro invalida automaticamente.

## 7. Modelo de edição

```
Project    { config (fps, resolução, sample rate), mediaPool[], timelines[] }
MediaRef   { id (uuid), caminho, info do probe, proxy? }
Timeline   { fps (racional), videoTracks[], audioTracks[] }
Track      { id, tipo, locked, muted, enabled, items[] ordenados sem sobreposição }
Clip       { id, mediaRef, sourceIn, duração, início, linkGroup, params, timeRemap? }
Transition { entre dois clipes adjacentes; duração; alinhamento; tipo }
Title      { clip gerador: texto, fonte, tamanho, cor, posição, fundo }
```

- Posições em **quadros inteiros (`int64`)** na timeline; tempo racional só na conversão
  para o tempo da fonte. Edição de áudio subframe: fora do v1.
- Posição **explícita** (não baseada em gaps); conversor OTIO faz a ponte.
- Operações são **funções puras** `aplicar(timeline, op) → Result<timeline, erro>`.
- **Undo por snapshot** com trilhas compartilhadas (`shared_ptr`); só a trilha alterada é
  copiada. *Teto conhecido:* memória em projetos enormes; upgrade = comandos por diff.

### 7.1 Operações

insert, overwrite, replace, append, lift, delete, ripple delete, blade, move (com snap),
trim, **ripple**, **roll**, **slip**, **slide**, speed (constante e rampa via time remap),
link/unlink, adicionar/remover/travar/mutar trilha.

Nenhuma operação ultrapassa os limites da mídia; quando não há material, retorna erro
com motivo exibido na UI.

### 7.2 Keyframes e Inspector

`AnimatedValue = { padrão, keyframes[{ quadro relativo ao clipe, valor, hold|linear|bezier }] }`.
Keyframes acompanham o clipe ao mover; os que ficam fora após trim são preservados.

Parâmetros v1: transform (posição, zoom, rotação, âncora), crop (com suavidade),
opacidade, composite (normal, add, multiply, screen, overlay), velocidade/rampa
(quadro mais próximo; optical flow fora do v1), volume e pan.

### 7.3 Transições e títulos

Transições exigem handles; sem material, a operação é recusada com mensagem.
Títulos renderizados via `QPainter` numa textura que entra no grafo.

### 7.4 Arquivo de projeto

JSON com `schemaVersion` e migrações; gravação atômica via `QSaveFile`; autosave
periódico com backups rotativos.

## 8. Interface

- Barra de páginas no rodapé; v1 = **Edit** e **Deliver** (demais páginas entram com
  seus subprojetos, sem botões desabilitados).
- Edit: Media Pool · Viewer fonte · Viewer timeline · Inspector; timeline embaixo com
  ferramentas (seleção A, trim T, blade B, snap N, link).
- Layout fixo com painéis recolhíveis (`QSplitter`), sem janelas flutuantes.
- Timeline desenhada com `QPainter` direto do modelo (fonte única da verdade), só a
  faixa visível; thumbnails e waveforms (picos em cache no disco) em segundo plano.
- Viewers via `QRhiWidget` usando o mesmo grafo do export.
- Atalhos padrão da indústria (J/K/L, I/O, B, A/T, F9/F10, Shift+Del), keymap editável
  em JSON; toda ação é `QAction`.
- Deliver: presets (YouTube 1080p H.264, 4K H.265, ProRes 422 HQ, personalizado),
  intervalo (tudo ou In/Out), fila de render com progresso, export em thread separada.
- Tema escuro (Fusion + QSS); `tr()` desde o início (pt-BR e en); navegação por teclado e
  tooltips com atalho.

## 9. Marcos do Subprojeto 1

| Versão | Entrega | Verificação |
|---|---|---|
| v0.1 | Esqueleto: CMake + Qt + FFmpeg, janela, CI Win/Linux/mac | build verde nos 3 SOs |
| v0.2 | Importar mídia + Media Pool + viewer com decode | testes de probe/decode com mídia sintética |
| v0.3 | Timeline multi-trilha + playback com áudio sincronizado (viewer CPU) | drift A/V < 1 quadro em 10 min (tempo virtual) + playback real no backend nulo |
| v0.4 | Edição básica: corte, mover, blade, undo/redo | testes de operação em tabela |
| v0.5 | Trims avançados: ripple, roll, slip, slide, JKL | testes de operação em tabela |
| v0.6 | Inspector + keyframes; viewers e composição na GPU (QRhi) | testes de interpolação + golden render |
| v0.7 | Transições + títulos + fades de áudio | golden render (PSNR) |
| v0.8 | Proxies + render cache; decode por hardware (D3D11VA), prefetch/cache em workers | 4K H.265 em tempo real em GPU média |
| v0.9 | Deliver + import/export OTIO/FCPXML/EDL | round-trip com timeline exportada do Resolve |
| v1.0 | Instalador Windows, estabilização, docs | checklist de release |

v0.3 é o maior risco técnico e foi posicionado cedo de propósito. Remanejado em 2026-10-13 (aprovado pelo Miguel): GPU/QRhi no v0.6 (quando houver composição) e decode por hardware no v0.8 (quando o alvo for 4K em tempo real); o v0.3 foca só em timeline + sincronia A/V.

## 10. Testes e qualidade

| Nível | Ferramenta | Cobre |
|---|---|---|
| Unitário | Qt Test | tempo racional, modelo, operações (tabelas em texto), keyframes, parsers |
| Integração | Qt Test + mídia sintética | probe, decode HW/SW, seek, drift A/V, export→reimport |
| Golden render | PSNR > 45 dB | quadro renderizado vs referência (tolerância entre GPUs) |
| Fumaça de UI | `QTest` | abrir, agir, salvar, reabrir |

Formato dos testes de edição:

```
dado:     V1: A[0..10) B[10..20)
roll(corte=10, +3)
espera:   V1: A[0..13) B[13..20)
```

- Mídia de teste gerada pelo FFmpeg (`testsrc`, flash + bipe a cada segundo); nenhum
  arquivo com direitos autorais no repo.
- Avisos como erro (`/W4 /WX`; `-Wall -Wextra -Werror`), `clang-format`, AddressSanitizer
  no CI Linux.
- CI GitHub Actions (matriz Win/Linux/mac, cache binário do vcpkg). Ativo após criação do
  repositório remoto, que requer confirmação explícita.

## 11. Processo

- **Conventional Commits**; commits pequenos e coerentes.
- Branch por marco (`marco/vX.Y-nome`) → merge na `main` com testes verdes → tag `vX.Y.0`.
- SemVer 0.x até a 1.0.
- Ritual por etapa: anunciar → testes primeiro → implementar → rodar build/testes e
  mostrar a saída → atualizar `PROGRESS.md` e `CHANGELOG.md` → commit → registrar na
  memória persistente do assistente → ao fim do marco, parar e reportar.

## 12. Licença e higiene

- `LICENSE` (GPLv3), `THIRD_PARTY_NOTICES.md` (Qt, FFmpeg, OTIO e demais).
- `CONTRIBUTING.md` com a regra clean-room da seção 2.

## 13. Fora do escopo do Subprojeto 1

Páginas Media, Cut, Color, Fusion, Fairlight; Estúdio IA; edição de áudio subframe;
optical flow; docking livre; leitura de `.drp`; zero-copy de decode HW; releases oficiais
Linux/macOS.
