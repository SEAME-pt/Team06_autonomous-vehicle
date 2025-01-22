# CICD
## Description
Currently we use ```cicd.yml``` to "cross"-compile and deploy code to Jetson Nano on ```main``` pull requests and on ```dev``` for testing.
This action has two parts: **build** and **deploy**.
### Build
**build** will run on a default ```x86``` ```ubuntu-latest``` GitHub hosted runner, since we don't have access to ```ARM64``` runners.
Using ```docker buildx``` and ```QEMU``` we run a custom Docker Image based on ```balenalib/jetson-nano-ubuntu:bionic``` since it emulates the OS running on our Jetson Nano.
On our ```Dockerfile``` we ```RUN``` intallation for every dependency we have in our software and every tool needed for compilation.
```actions/checkout``` and ```actions/upload-artifact``` are also used to get our repo and upload the binaries after compilation.
### Deploy
**deploy** will run on a self-hosted runner on SEA:ME laptop, which starts with Ubuntu running in the background as a service.
```actions/download-artifact``` will be triggered after **build** and will download the binaries, and then they well be shipped to Jetson via SSH using ```scp``` and ```sshpass``` which will use GitHub secrets to get the password needed for the connection.
## Our method vs Cross-compilation
We can't really call our compilation method a cross-compilation because we are using emulation.
This has many advantages but also comes with a downside - it is slower. This isn't a deal breaker because we are not actively using it for development. For that we can either use Jetson Nano to compile, since we know from our ```
Dockerfile``` every dependency we need, or change the **label**
 of the **build** job to run on the SEA-ME laptop, which will eventually be faster.
Since working with Ubuntu:bionic has been our biggest blocker from the beginning, this approach has some clear advantages in its simplicity:
- Setup
- Maintenance
- Consistency
- Runtime testing is a possibility


