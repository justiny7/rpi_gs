/opt/homebrew/opt/vc4asm/bin/vc4asm -c project_points_cov2d_inv.c -h project_points_cov2d_inv.h project_points_cov2d_inv.qasm
gmake
rpi-install kernel-project_points_cov2d_inv.img
