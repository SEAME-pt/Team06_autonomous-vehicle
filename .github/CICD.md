# CI/CD Pipeline Documentation

This document explains the Continuous Integration and Continuous Deployment (CI/CD) setup for the ClusterDisplay project.

## Overview

The CI/CD pipeline is implemented using GitHub Actions and consists of the following stages:

1. **Build and Test**: Compiles the code, runs all tests, and generates coverage reports
2. **Deploy**: Deploys the application to the target hardware if tests pass
3. **Notify**: Reports the overall status of the pipeline

## Workflow Configuration

The workflow is defined in `.github/workflows/ci-cd-with-tests.yml` and is triggered by:
- Pushes to `main` and `dev` branches
- Pull requests to `main`
- Manual triggers via the GitHub UI

### Docker Environment

The pipeline uses a pre-built Docker image (`jmoreiraseame/jetson-nano-ubuntu:bionic`) that includes:
- Ubuntu 18.04 base image (matching the target Jetson Nano environment)
- All required dependencies pre-installed (libzmq3-dev, cppzmq, etc.)
- Development tools (git, cmake, build-essential, etc.)
- Qt5 dependencies for the UI component

The Docker image definition is maintained in the `CICD/Dockerfile` directory and can be updated as needed.

## Pipeline Stages

### 1. Build and Test

This stage:
- Sets up the necessary Docker environment (QEMU, Buildx)
- Uses a custom Jetson Nano Ubuntu Docker image for consistent builds
- Installs required dependencies (libzmq3-dev, cppzmq, etc.)
- Compiles the code with test and coverage options enabled
- Runs all unit tests and captures results in JUnit XML format
- Generates code coverage reports
- Uploads test results and coverage reports as artifacts
- Determines whether tests have passed to proceed to deployment

### 2. Deploy

This stage only runs if:
- All tests have passed
- The workflow was triggered by a push to `main` or `dev` branch

The deployment process:
- Copies the compiled binaries to the Jetson Nano hardware
- Sets execute permissions on the binaries

### 3. Notify

This stage always runs and reports the status of the pipeline, indicating:
- If the entire pipeline completed successfully
- If build and tests passed but deployment was skipped or failed
- If build or tests failed

## Test Reporting

Test results are:
- Published using the JUnit reporter action
- Available as artifacts for download
- Used to determine whether to proceed with deployment

## Coverage Reporting

Code coverage reports are:
- Generated using lcov/gcovr
- Available as HTML reports in artifacts
- Uploaded to Codecov for badge generation

## Local Testing

You can run the same tests locally using either:

### Using the run_tests.sh script directly:
```bash
./run_tests.sh
```

### Using Docker (recommended for consistency with CI):
```bash
# Pull the Docker image
docker pull jmoreiraseame/jetson-nano-ubuntu:bionic

# Run tests inside the container
docker run -it --rm --platform linux/arm64 -v $(pwd):/app -w /app jmoreiraseame/jetson-nano-ubuntu:bionic ./run_tests.sh
```

This provides the exact same environment as the CI pipeline, ensuring consistent results.

## Troubleshooting

If the pipeline fails:

1. Check the Actions tab in GitHub to find the failed job
2. Examine the logs to identify the specific failure
3. Download artifacts for detailed test results and coverage reports
4. Make fixes locally and run tests before pushing again

### Common Issues

- **Dependency Problems**: The workflow installs cppzmq from source. If this fails, check the logs for error messages.
- **Compilation Errors**: Look for compiler error messages in the workflow logs.
- **Test Failures**: Check the test results artifacts for detailed test failure information.
- **Deployment Issues**: These are often related to network or credentials problems.

## Adding/Modifying Tests

When adding new components or tests:

1. Add the test file to the appropriate directory
2. Update CMakeLists.txt to include the new test if necessary
3. Ensure the test produces JUnit XML output for CI integration
4. Run tests locally before pushing to ensure they work correctly
