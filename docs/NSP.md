# 无名杀 Switch版 — NSP 安装说明

[下载独立 NSP](../dist/sanguosha-switch-v0.1.1.nsp)

这是包含程序和全部资源的自制应用 NSP。安装后在 Switch 主菜单选择 **无名杀 Switch版** 即可启动，不需要额外的 NRO 路径或网页服务器。

1. 使用你已有的自制 NSP 安装工具安装该文件。
2. 从主菜单启动 **无名杀 Switch版**。
3. 方向键选将，按 A 进入对局。

需要支持安装与运行未零售签名自制应用的系统环境；官方未修改系统无法运行该包。本项目不提供系统修改方法。**未经过 Switch 真机安装、启动、休眠或底座模式验证。** 当前仍是 4 名武将、11 种牌的原生精简体验版，完整玩法范围见 README。

包内含 Program、Control、Meta 三个 NCA，程序为 AArch64 NSO，应用类型为 Application，中文字体、卡牌和桌面均在 RomFS 内。

- 应用名称：`无名杀 Switch版`
- 显示版本：`0.1.1`
- 应用 Title ID：`0100A53547530000`（本项目选择的自制应用 ID）
- SHA-256：[SHA256SUMS](../dist/SHA256SUMS)

NSP 的内容哈希和提取结果已核验；自制 NCA/NPDM 没有任天堂零售签名。因此哈希通过不等同于在任何 Switch 系统上均可安装或运行。

## 从源码重新生成

准备 devkitPro Switch 开发环境、Python 3 和 [hacBrewPack](https://github.com/pplatoon/hacBrewPack)。使用你自己的、已具备授权的本地打包密钥文件：

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITA64=$DEVKITPRO/devkitA64
export PATH=$DEVKITPRO/tools/bin:$DEVKITA64/bin:$PATH
python3 tools/build_nsp.py --keyset /private/path/prod.keys --hacbrewpack /path/to/hacbrewpack
```

脚本仅读取打包需要的两个键项，在私有临时目录工作，完成后清理临时目录。不要将 `prod.keys`、`title.keys` 或 `keys.dat` 提交到公开仓库。输出文件为 `dist/sanguosha-switch-v0.1.1.nsp`。普通 CI 只构建 NRO，不配置或存储密钥材料。

应用配置位于 `packaging/application.json`，采用 libnx homebrew 的应用权限配置。已有相同 Title ID 的其他应用需要先检查冲突，勿覆盖不相关应用。
