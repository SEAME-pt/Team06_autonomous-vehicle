# CI/CD Pipeline Documentation

This directory contains documentation for our GitHub Actions-based CI/CD pipeline that handles building, testing, and deploying to Jetson Nano hardware.

## Pipeline Overview

The pipeline consists of three main jobs:
1. Test - Runs unit tests in a Docker container
2. Build - Compiles the project and creates artifacts
3. Deploy - Deploys to Jetson Nano hardware

## Environment

- Base Image: `jmoreiraseame/jetson-nano-ubuntu:bionic`
- Architecture: ARM64 (linux/arm64)
- Platform: Ubuntu 18.04 Bionic
- Runner: Custom runner (seame) for deployment

## Workflow Jobs

### Test Job
- Runs on: ubuntu-latest
- Uses QEMU for ARM64 emulation
- Executes all tests via `scripts/run_tests.sh`
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

The pipeline uses two main scripts:
```bash
scripts/build.sh      # Handles compilation and build process
scripts/run_tests.sh  # Executes test suite
```

## Security

- Deployment credentials stored as GitHub Secrets
- SSH-based deployment
- Restricted to specific branches (main, dev)

## Adding New Components

When adding new components:
1. Ensure tests are added to `run_tests.sh`
2. Update `build.sh` if needed
3. Verify ARM64 compatibility
4. Test in Docker environment first
