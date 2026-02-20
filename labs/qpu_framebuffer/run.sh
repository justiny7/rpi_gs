/opt/homebrew/opt/vc4asm/bin/vc4asm -c qpu_code.c -h qpu_code.h qpu_code.qasm
gmake
rpi-install kernel-qpu_write.img
