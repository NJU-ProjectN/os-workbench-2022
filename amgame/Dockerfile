FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get install -y build-essential gcc-multilib qemu-system strace gdb sudo python3 libsdl2-dev libreadline-dev llvm-11 gcc-riscv64-linux-gnu
RUN useradd -ms /bin/bash user
USER user
WORKDIR /home/user
