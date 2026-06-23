@REM set "HPM_SDK_BASE=C:\hpm\hpm_sdk"
set "HPM_SDK_BASE=D:\_tools\hpm_sdk\hpm_sdk"
set "GNURISCV_TOOLCHAIN_PATH=D:\_tools\hpm_sdk\toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win"
@REM set "GNURISCV_TOOLCHAIN_PATH=C:\hpm\toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win"

@REM set "GNURISCV_TOOLCHAIN_PATH=D:\_tools\ZCC"
@REM set "HPM_SDK_TOOLCHAIN_VARIANT=zcc"
set "HPM_SDK_TOOLCHAIN_VARIANT=gcc"
set "PYTHON_EXECUTABLE=D:\_tools\hpm_sdk\tools\python3\python3"
@REM set "PYTHON_EXECUTABLE=C:\hpm\tools\python3\python3"

cmake -GNinja -DBOARD=hslinkpro -DHPM_BUILD_TYPE=flash_uf2 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B=./build .
cmake --build ./build