/opt/homebrew/opt/vc4asm/bin/vc4asm -c project_points_cov2d_inv.c -h project_points_cov2d_inv.h project_points_cov2d_inv.qasm
/opt/homebrew/opt/vc4asm/bin/vc4asm -c spherical_harmonics.c -h spherical_harmonics.h spherical_harmonics.qasm
/opt/homebrew/opt/vc4asm/bin/vc4asm -c calc_bbox.c -h calc_bbox.h calc_bbox.qasm
/opt/homebrew/opt/vc4asm/bin/vc4asm -c calc_tile.c -h calc_tile.h calc_tile.qasm
/opt/homebrew/opt/vc4asm/bin/vc4asm -c sort_copy.c -h sort_copy.h sort_copy.qasm
/opt/homebrew/opt/vc4asm/bin/vc4asm -c render.c -h render.h render.qasm
/opt/homebrew/opt/vc4asm/bin/vc4asm -c scan_rot.c -h scan_rot.h scan_rot.qasm
/opt/homebrew/opt/vc4asm/bin/vc4asm -c scan_sum.c -h scan_sum.h scan_sum.qasm
gmake
rpi-install kernel-gaussian_splat.img
