# 验证记录 — 0.1.0

验证日期：2026-10-14。

## 已执行并通过

- 原生规则专项测试：武圣、龙胆、咆哮/连弩、杀距离与目标合法性、闪响应、决斗往返、范围锦囊响应、桃自救、强制弃牌、身份胜负。
- 300 场不同随机种子的完整自动对局：每一步核对 78 张卡的数量和唯一 ID，均结束并且没有卡牌丢失或重复。
- AddressSanitizer + UndefinedBehaviorSanitizer 验证通过；当前容器不支持 LeakSanitizer 进程扫描，因此该模式单独禁用，不声称已完成泄漏检测。
- Linux UI 编译采用 `-Wall -Wextra -Werror` 并通过。SDL dummy 后端实际渲染选将和对局截图，人工检查中文字、立绘和排版。
- Switch AArch64 原生交叉编译成功，编译/链接无警告，未使用专有 SDK。
- NRO 头 `NRO0` 和内嵌资产头 `ASET` 检查通过。

NRO 文件大小：9,902,373 字节。

NRO SHA-256：`433fd18cbd7ec81bf091293ef90f5334e4fcd4313871ea424a40eeef29faa338`。

## 编译环境

- devkitA64 r30；GCC 16.1.0；libnx 4.12.0；switch-tools 1.13.1。
- Switch SDL2 2.28.5；SDL2_image 2.0.4；SDL2_ttf 2.22.0。
- 构建依赖来自 devkitPro 官方软件包，下载后按官方包数据库 SHA-256 校验。
- 源码、资源和构建配置随项目提供。CI 使用 devkitPro 官方 Docker 镜像；CI 实际执行结果以 GitHub Actions 为准。

## 尚未验证

**没有 Switch 真机或可用的主机运行环境，未测试真机启动、Joy-Con/Pro Controller、触摸屏、底座模式性能、休眠恢复或 hbmenu 内存限制。** 桌面界面正常和成功编译不能替代这些检查。本首版提供可运行的 NRO 候选产物，不保证所有系统版本与设备行为。

首次实机测试建议记录：系统/Atmosphere 版本、hbmenu 启动模式、卡顿或崩溃时的操作步骤。完整玩法的缺失和简化详见 README。
