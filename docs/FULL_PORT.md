# 完整懒人包移植：0.2.0 引擎验证阶段

目标是保留完整无名杀懒人包的 JavaScript 引擎、全部武将技能、游戏模式、扩展与素材。0.1.1 的原生精简规则没有继续扩写。

## 当前已完成

- 全量导入工具：复制上游全部 Git 跟踪文件，只排除 `.git` 管理数据，逐文件比较 SHA-256。
- 实际导入结果：14,604 个文件，1,676,124,066 字节，来源提交 `e52bf9159ce0d8d6a590b6d8db4cfac3f609930c`。
- Switch 原生启动器：调用 `webOfflineCreate` 读取应用自身的 `HtmlDocument` NCA。
- NSP 构建：生成 Program、Control、Manual（HtmlDocument）、Meta 四个 NCA，CNMT 中登记 HtmlDocument 内容记录。
- 已完成全量原包实验容器的构建。修复大文件快速复制造成的不完整导出后，逐块写入、长度检查和最终 SHA-256 独立核验通过。详见 `full-port/container-validation.json`。
- 小型检测 NSP：检测模块、动态 import、存储、资源请求、Worker、JavaScript 语法及音频。

**整包封装完成不等于完整移植已可玩。Switch 网页引擎兼容性、原包实际启动、对局、扩展加载与跨启动存档尚未实机验证。** 全量容器保留原始源码，没有通过替换规则或删武将规避兼容问题。

## 先进行实机检测

[下载小型引擎验证 NSP](../dist/wumingsha-runtime-probe-v0.2.0.nsp)

1. 在已有的支持自制 NSP 的系统环境安装。该检测程序名称为“无名杀 引擎验证”，Title ID 为 `0100A53547540000`，与历史体验版不同。
2. 启动后按 A 打开离线检测页面，等待约 8 秒。
3. 点击音频测试，确认是否能听到声音。
4. 点击“保存检测结果并返回”。正常情况下结果写入 `sd:/switch/wumingsha-switch/runtime-report.json`。
5. 再次打开检测页，检查报告中的 `storageSeenFromEarlierRun`；它用于确认存储能否跨启动保留。

若无法打开检测页面，请记录启动器显示的 `Web launch result` 错误码；若报告没有写出，请拍下检测页面。小型检测包不附带约 1.68 GB 原包，页面会明确禁用原包启动入口。

## 构建完整实验容器

准备 devkitPro、Python 3、[hacPack](https://github.com/DarkMatterCore/hacPack)，以及自己的本地打包密钥。

```sh
git clone https://github.com/lieren2023/noname-for-dummies.git /path/to/upstream
git -C /path/to/upstream checkout e52bf9159ce0d8d6a590b6d8db4cfac3f609930c
python3 tools/stage_full_package.py --upstream /path/to/upstream --output /path/to/new-stage
export DEVKITPRO=/opt/devkitpro
python3 tools/build_web_runtime.py --keyset /private/prod.keys --hacpack /path/to/hacpack \
  --payload-stage /path/to/new-stage --output /path/to/wumingsha-full-runtime-experimental.nsp
```

全量包内包含检测页面和原始 `game/index.html`，可点击“尝试启动完整原包”。它是兼容性实验产物，不能标为已经完成的可玩完整版。完整实验容器约 1.68 GB，超过 GitHub 普通文件大小限制，不直接提交至仓库。

只构建小型检测包时省略 `--payload-stage` 和 `--output`。未配置任何打包密钥到 CI 或仓库。

## 已执行的验证与限制

- C 启动器交叉编译采用 `-Wall -Wextra -Werror` 并通过。
- 检测脚本经 Acorn 的 ES5 解析器检查；Node 单元测试覆盖存储被禁用、模块执行超时、异步资源请求和报告导出。它们不替代真实浏览器测试。
- 使用 hactool 独立核验小型及全量 NSP 的内容哈希与网页目录；全量容器中所有 14,604 个原包路径存在。小型 NSP 的 HtmlDocument 元数据引用也已逐项核对。
- 未用伪造的浏览器能力结果宣布完整原包兼容。后续依据真机检测结果决定语法/模块适配、存储桥接或更换运行环境。

参考 API：[libnx web applets](https://switchbrew.github.io/libnx/web_8h.html)。后续必须完成：完整原包启动 → 单机对局 → UI 扩展 → 持久化存档 → 手柄映射 → 音频/性能与联机验证。
