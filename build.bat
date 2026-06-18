set "HPM_SDK_BASE=D:\_tools\hpm_sdk\hpm_sdk"
set "GNURISCV_TOOLCHAIN_PATH=D:\_tools\hpm_sdk\toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win"
set "HPM_SDK_TOOLCHAIN_VARIANT=gcc"
set "PYTHON_EXECUTABLE=D:\_tools\hpm_sdk\tools\python3\python3"

cmake -GNinja -DBOARD=hslinkpro -DHPM_BUILD_TYPE=flash_uf2 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B=./build .
cmake --build ./build