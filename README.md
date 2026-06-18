# HPM5301 GPIO Simulate SWD Optimize

## 构建方法

1. 下载 HPM 官方 SDK
2. 修改 `build.bat` 中的路径
3. 执行 `build.bat`

## 注意事项

1. 目前代码为启动从 XIP 加载到 ILM 中执行
2. 目前编译后生成 uf2 固件，直接通过 HSLink Pro 的 bootloader 升级即可（HSLink 没有留 JTAG :( ）
3. 目前使用的是 HSLink Pro V1.0 版本硬件，更高版本也许引脚有变化（？）
