/opt/homebrew/opt/vc4asm/bin/vc4asm -c project_points.c -h project_points.h project_points.qasm
gmake
rpi-install kernel-project_points.img
