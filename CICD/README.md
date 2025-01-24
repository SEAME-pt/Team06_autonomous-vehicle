# Continuous Integration and Deployment (CI/CD)

## Overview
Our CI/CD pipeline is designed to compile and deploy code for the Jetson Nano platform efficiently. It leverages GitHub Actions for automation, combining **build** and **deploy** stages to ensure code quality and streamline deployment processes.

Currently, the `cicd.yml` workflow handles:
1. **Cross-compilation-like builds** on `main` pull requests for production.
2. Testing on the `dev` branch.

---

## Workflow Breakdown

### **1. Build Stage**
- **Environment**:
  - Runs on a default `x86` GitHub-hosted runner (`ubuntu-latest`) since we don't have free access to ARM64 GitHub-hosted runners.
  - Utilizes `docker buildx` and `QEMU` to emulate the ARM64 environment of the Jetson Nano.

- **Docker Image**:
  - The custom Docker image is based on `balenalib/jetson-nano-ubuntu:bionic`, matching the Jetson Nano's OS.
  - The `Dockerfile` handles:
    - Installing all dependencies required for the software.
    - Setting up tools necessary for compilation.

- **Key Actions**:
  - **Checkout**: Uses `actions/checkout` to retrieve the code repository.
  - **Artifact Upload**: Employs `actions/upload-artifact` to save compiled binaries for the deploy stage.

---

### **2. Deploy Stage**
- **Environment**:
  - Runs on a self-hosted SEA:ME laptop with Ubuntu, operating in the background as a service.

- **Process**:
  1. **Artifact Retrieval**:
     - Uses `actions/download-artifact` to fetch the compiled binaries from the build stage.
  2. **Deployment**:
     - Transfers binaries to the Jetson Nano via SSH, utilizing:
       - `scp` for secure file copying.
       - `sshpass` for automating SSH connections, with GitHub secrets securely storing credentials.

---

## Methodology: Emulation vs. Cross-Compilation
Our current build method employs emulation rather than true cross-compilation. While this approach has pros and cons, it has proven effective for our use case.

### **Advantages of Emulation**:
- **Simplicity**: Minimal setup compared to complex cross-compilation toolchains.
- **Consistency**: Ensures alignment with the target environment.
- **Maintenance**: Reduces the need for constant updates to toolchains and configurations.
- **Runtime Testing**: Allows potential runtime validation within the emulated environment.

### **Downsides**:
- **Slower Performance**: Emulation introduces overhead, making builds slower than native compilation.

### **Mitigating the Downside**:
For active development:
- Compile directly on the Jetson Nano.
- Alternatively, label the **build** job to run on the SEA:ME laptop for faster builds.

---

## Why This Approach?
While working with `ubuntu:bionic` has been a significant challenge, our emulation-based pipeline offers several advantages:
- **Ease of Setup**: Quickly establish a consistent build environment.
- **Low Maintenance**: Reduce the complexity of toolchain updates.
- **Reliability**: Consistent results across builds.
- **Flexibility**: Ability to test software in an environment mirroring the Jetson Nano runtime.

This method strikes a balance between simplicity and functionality, making it well-suited to our current needs.
