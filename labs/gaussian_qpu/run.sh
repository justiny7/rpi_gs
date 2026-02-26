/opt/homebrew/opt/vc4asm/bin/vc4asm -c project_points_cov2d_inv.c -h project_points_cov2d_inv.h project_points_cov2d_inv.qasm
/opt/homebrew/opt/vc4asm/bin/vc4asm -c spherical_harmonics.c -h spherical_harmonics.h spherical_harmonics.qasm
gmake
rpi-install kernel-gaussian_qpu.img
