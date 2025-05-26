# CI/CD Workflow Documentation

This document describes the GitHub Actions CI/CD pipeline for the Cluster Display project, defined in `../.github/workflows/cicd.yml`.

## Overview

The CI/CD pipeline automates the testing, building, and deployment process for the Jetson Nano project. It ensures code quality through comprehensive testing and provides reliable deployment to target hardware.

## Pipeline Architecture

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│    Test     │───▶│    Build    │───▶│   Deploy    │
│   Job       │    │    Job      │    │    Job      │
└─────────────┘    └─────────────┘    └─────────────┘
      │                    │                  │
      ▼                    ▼                  ▼
  Unit Tests         ARM64 Build        Jetson Nano
  Docker ARM64       Versioned          SSH/SCP
  QEMU Emulation     Artifacts          Deployment
```

## Workflow Jobs

### 1. Test Job (`test`)
**Purpose**: Validates code quality and functionality

**Platform**: `ubuntu-latest` with ARM64 emulation

**Key Steps**:
- Sets up QEMU for ARM64 emulation
- Configures Docker Buildx for multi-platform builds
- Pulls Jetson Nano Ubuntu Docker image
- Runs all tests in ARM64 Docker container


### 2. Build Job (`build`)
**Purpose**: Compiles the project for ARM64 architecture

**Dependencies**: Requires `test` job to pass

**Key Steps**:
- Cross-compiles for Jetson Nano (ARM64)
- Creates versioned build artifacts
- Generates version metadata
- Uploads artifacts for deployment


### 3. Deploy Job (`deploy`)
**Purpose**: Deploys artifacts to Jetson Nano

**Platform**: Custom runner (`seame`)

**Dependencies**: Requires `build` job to pass

**Conditions**: Only runs on `main` and `dev` branches

**Key Steps**:
- Downloads build artifacts
- Sets up SSH authentication
- Transfers files via SCP
- Sets executable permissions
- Verifies deployment
- Logs deployment history


## Docker Configuration

### Base Image
- **Image**: `jmoreiraseame/jetson-nano-ubuntu:bionic`
- **Architecture**: ARM64 (linux/arm64)
- **Platform**: Ubuntu 18.04 Bionic

### Features
- Pre-installed development tools
- ZeroMQ libraries and dependencies
- CMake and build essentials
- Qt libraries and dependencies


## File Structure

```
.github/workflows/
└── cicd.yml                 # Main CI/CD workflow

CICD/
├── README.md
├── Dockerfile              # Custom Docker image
├── updatedockerimage.sh    # Docker image update script
└── *.yml.bak              # Backup workflow files
```
