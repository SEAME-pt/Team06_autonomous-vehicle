# CI/CD Pipeline Documentation

This directory contains documentation for our GitHub Actions-based CI/CD pipeline that handles building, testing, and deploying to Jetson Nano hardware.

## Pipeline Overview

The pipeline consists of four main jobs:
1. Lint - Performs code quality checks
2. Test - Runs unit tests in a Docker container and collects coverage
3. Build - Compiles the project and creates artifacts
4. Deploy - Deploys to Jetson Nano hardware

## Environment

- Base Image: `jmoreiraseame/jetson-nano-ubuntu:bionic`
- Architecture: ARM64 (linux/arm64)
- Platform: Ubuntu 18.04 Bionic
- Runner: Custom runner (seame) for deployment

## Workflow Jobs

### Lint Job
- Runs on: ubuntu-latest
- Uses QEMU for ARM64 emulation
- Executes linting via `scripts/run_linters.sh`
- Code must pass linting before tests run
- Checks:
  - Code formatting (clang-format)
  - Static analysis (clang-tidy)

### Test Job
- Runs on: ubuntu-latest
- Uses QEMU for ARM64 emulation
- Executes all tests via `scripts/run_tests.sh`
- Generates code coverage reports with lcov
- Uploads coverage to Codecov
- Must pass before build proceeds

### Build Job
- Runs on: ubuntu-latest
- Requires successful test job
- Creates versioned artifacts
- Disables test compilation for production builds
- Uploads artifacts for deployment

### Deploy Job
- Runs on: Custom runner [seame]
- Requires successful build job
- Only runs on main and dev branches
- Deploys to Jetson Nano via SCP
- Sets executable permissions
- Maintains deployment history log

## Version Control

- Version format: 1.0.{run_number}
- Version file included in deployment
- Deployment history logged on target device

## Scripts

The pipeline uses these main scripts:
```bash
scripts/build.sh      # Handles compilation and build process
scripts/run_tests.sh  # Executes test suite and generates coverage
scripts/run_linters.sh # Performs code quality checks
scripts/run_coverage.sh # Generates code coverage reports
```

## Code Coverage

- Coverage reports generated during test job using `run_coverage.sh`
- Uses lcov to capture and process coverage data
- HTML reports generated automatically
- Reports stored as downloadable artifacts in GitHub Actions
- Available for 14 days after each workflow run

## Security

- Deployment credentials stored as GitHub Secrets
- SSH-based deployment
- Restricted to specific branches (main, dev)

## Adding New Components

When adding new components:
1. Add component paths to `run_linters.sh` if needed
2. Ensure tests are added to `run_tests.sh`
3. Update `build.sh` if needed
4. Verify ARM64 compatibility
5. Test in Docker environment first
