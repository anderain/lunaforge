# Lunaforge
Lunaforge 是一款专为特定嵌入式设备平台设计的游戏开发工具链。
它目前仍在开发中，文档尚未完成。

目标是将重要的组件用标准 C89 编写，部分组件用 Typescript 实现。

## 组件
| 组件 | 介绍 |
|---|---|
| 碎灵符（Runeshard） | Sprite图分切工具，把素材转化为8x8的小图集合 |
| 照骨镜（Bonegaze） | 项目构建工具，类似 `make` |
| 董狐笔（Khronicler）| K-Basic脚本语言解释器 |
| 珍珑局（Gopuzzle） | 地图编辑器，支持 GDI 或者 SDL 作为图形后端 | 

## 法宝
| 法宝 | 介绍 |
|---|---|
| jasmine89 | JSON解析工具，用来解析配置文件和项目文件 |
| salvia89 | 无依赖的 sprintf 替代品 |
| raster-fonts | 栅格化像素字体 |
| key-code-tester | WinCE 用的按键值测试工具 |
| color-palette | VGA 调色板定义 |
| color-wheel-maker | 色轮 SVG 生成工具 |