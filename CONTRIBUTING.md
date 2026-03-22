# Contributing

Thank you for your interest in contributing to `xmr-worm-advanced`. Please read these guidelines before submitting a pull request.

## Code Style

- Adhere to the `.clang-format` configuration at the project root
- Use `snake_case` for variables and functions; `PascalCase` for classes
- All public API headers must have include guards (`#ifndef XMR_XXX_H`)
- Avoid `using namespace std` in header files

## Pull Requests

1. Fork the repository and create a feature branch: `git checkout -b feature/my-feature`
2. Write clear, atomic commits with descriptive messages
3. Add or update unit tests under `tests/` if you change core logic
4. Ensure the project builds without warnings under both GCC (Linux) and MSVC (Windows)
5. Open a pull request against the `main` branch with a clear description of the change

## Reporting Issues

- Use the GitHub Issues tracker
- Include the OS, compiler version, and exact CMake configuration
- Attach the full build log when reporting compilation failures

## Branch Naming

| Prefix | Purpose |
|---|---|
| `feature/` | New functionality |
| `fix/` | Bug fix |
| `refactor/` | Code cleanup without behavior change |
| `docs/` | Documentation only change |
