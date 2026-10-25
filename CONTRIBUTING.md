# Contributing to Immich Desktop

Thanks for helping improve this unofficial Qt desktop client for Immich.
This project is fan-made and not affiliated with the official Immich project.

By participating, you agree to follow the [Code of Conduct](CODE_OF_CONDUCT.md).

## Ways to contribute

- **Bugs and UX issues** - open a [bug report](https://github.com/hdmain/immich-desktop/issues/new/choose)
- **Ideas** - open a [feature request](https://github.com/hdmain/immich-desktop/issues/new/choose)
- **Code** - fix issues or polish the UI (Qt Widgets / CMake)
- **Translations** - see [translations/README.md](translations/README.md)
- **Security** - report privately via [SECURITY.md](SECURITY.md); do not open a public issue for vulnerabilities

## Development setup

Requires CMake 3.21+ and Qt 6.2+ (Core, Gui, Widgets, Network, Svg, Multimedia, LinguistTools).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/immich          # Linux / macOS
# Windows: build-qt\immich.exe (or your CMake output dir)
```

Useful targets and notes:

- `update_translations` - refresh `.ts` catalogs and the README language table
- Keep changes focused; match existing style in `src/core` and `src/ui`
- Update `changelog.json` for user-facing changes (same version as `CMakeLists.txt`)

## Pull requests

1. Fork and branch from `main`.
2. Prefer small, reviewable PRs over large mixed changes.
3. Fill out the pull request template.
4. Ensure the app builds; add or extend tests under `tests/` when practical.
5. Do not commit secrets (`.env`, Snap credentials, tokens, certificates).

CI builds packages on push; failures on packaging jobs are worth checking even for doc-only PRs when workflows are shared.

## Issue guidelines

- Search existing issues before opening a new one.
- Include OS, app version (or beta commit), Immich server version, and steps to reproduce when reporting bugs.
- Feature requests should describe the problem and a concrete proposal, not only a wishlist item.

## License

Contributions are licensed under the MIT terms in [LICENSE.txt](LICENSE.txt).
