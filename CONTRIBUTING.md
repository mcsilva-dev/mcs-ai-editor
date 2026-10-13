# Contribuindo

## Regra clean-room (inegociável)

- Nenhum código, binário descompilado, asset, preset ou prompt da Blackmagic Design
  (DaVinci Resolve) ou do Higgsfield entra neste repositório.
- O Resolve é referência de **funcionalidade**: manual público, documentação pública de
  scripting, observação da interface, nomes de arquivos instalados e medições de
  comportamento (caixa-preta). Descompilar algoritmos para reimplementar é proibido.
- Interoperabilidade apenas via formatos abertos: OpenTimelineIO, FCPXML, EDL.

## Commits

Conventional Commits (`feat:`, `fix:`, `docs:`, `test:`, `build:`, `ci:`, `refactor:`).
Commits pequenos; `main` sempre compila e passa nos testes.

## Código

C++20, `clang-format` (arquivo `.clang-format` na raiz), avisos como erro,
todo texto de UI via `tr()` em inglês.
