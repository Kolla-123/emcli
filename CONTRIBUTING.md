# Contributing to EMCLI

Thank you for your interest in contributing to EMCLI! This document provides guidelines and instructions.

## Project Goals

EMCLI aims to provide:
- A lightweight, efficient CLI framework for embedded systems
- Zero external dependencies
- Clean, modular architecture
- Comprehensive documentation
- Production-ready code quality

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/emcli.git
   cd emcli
   ```
3. **Create a feature branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

## Development Workflow

### Building
```bash
make              # Build the project
make run          # Build and run
make clean        # Clean build artifacts
```

### Code Style

- **Indentation**: 2 spaces (not tabs)
- **Line Length**: Maximum 80 characters
- **Naming**: 
  - Functions: `snake_case`
  - Types: `UPPER_CASE` or `CamelCase`
  - Constants: `UPPER_CASE`
  - Private functions: prefix with `_`

### Commit Messages

Follow conventional commit format:

```
type(scope): subject

body (optional)

footer (optional)
```

**Types**: feat, fix, docs, style, refactor, test, perf, chore

**Examples**:
```
feat(cli): add command history support

Implements circular buffer for command history
with configurable size via config.h

Closes #42
```

```
fix(jsmn): handle escaped quotes in strings

Previously failed on JSON with escaped quotes.
Now properly tracks escape sequences.
```

## Pull Request Process

1. **Update** your fork with latest changes:
   ```bash
   git fetch origin
   git rebase origin/main
   ```

2. **Push** to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```

3. **Create** a pull request with:
   - Clear title describing changes
   - Description of what was changed and why
   - Reference to related issues
   - Screenshots for UI changes (if applicable)

4. **Respond** to review feedback promptly

## Testing

- Test your changes thoroughly before submitting
- Ensure code compiles without warnings: `gcc -Wall -Wextra -std=c99`
- Test on multiple platforms if possible
- Add test cases for new features

## Documentation

- Update README.md for user-facing changes
- Add inline comments for complex logic
- Document public API in header files
- Include examples for new features

## Area Guidelines

### JSON Parser (`jsmn.c/h`)
- Maintain compatibility with JSMN library
- Document tokenization behavior
- Add performance benchmarks for large JSON

### CLI Framework (`cli.c/h`)
- Keep interface simple and intuitive
- Support both array and linked-list registries
- Maintain backward compatibility

### Configuration (`config.h`)
- Only add new options when necessary
- Document all #defines
- Include rationale in comments

## Issues

### Reporting Bugs
- Use GitHub Issues
- Include reproduction steps
- Specify platform/compiler/version
- Attach relevant files/logs

### Feature Requests
- Discuss in Issues before starting work
- Explain use case and benefits
- Consider memory/performance impact

## Code Review Checklist

Before submitting, ensure:

- [ ] Code compiles without warnings
- [ ] No memory leaks
- [ ] All edge cases handled
- [ ] Documentation updated
- [ ] Commits have clear messages
- [ ] Changes are focused (one feature per PR)

## Questions?

Open an issue or discussion if you have questions about contributing.

## License

By contributing, you agree that your contributions will be licensed under the same license as the project.
