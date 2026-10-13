# Progresso — MCS AI Editor

Spec: [docs/superpowers/specs/2026-10-13-mcs-ai-editor-design.md](docs/superpowers/specs/2026-10-13-mcs-ai-editor-design.md)

## Estado atual

- **Fase:** design (Subprojeto 1 — Núcleo + Edit)
- **Próximo passo:** plano de implementação do v0.1 (esqueleto + CI)

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
