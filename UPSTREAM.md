# 上游来源

- 规则参考： https://github.com/libccy/noname
- 标准牌、美术与字体来源： https://github.com/lieren2023/noname-for-dummies
- Makefile 改编自： https://github.com/switchbrew/switch-examples/blob/master/templates/application/Makefile

`upstream/core` 保留本体的标准武将定义、技能和翻译；`upstream/card-standard.js` 与 `upstream/character-standard.js` 保留懒人版来源。该目录用于规则参考和可复现数据提取，不会被 NRO 执行。

`romfs/portraits/*.jpg` 来自懒人版对应的 `image/character/*.jpg`。`romfs/font.ttf` 是懒人版 `font/yuanli.ttf` 的字符子集，仅保留当前界面使用的字符，未改变字体造型。修改文字时需从原字体重新取子集或换为有合适授权的完整中文字体。

本项目新的原生 C++ 实现不是上游 JS 的逐行翻译；卡牌与武将声明数据由脚本提取。未移植上游 DOM、CSS、事件引擎与扩展接口，不能将 JS 扩展复制到 NRO 中使用。详细来源版本和每个随附上游文件的哈希见 `upstream/provenance.json`。
