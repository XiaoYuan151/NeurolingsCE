# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.2.0] - 2026-03-14

### 🚀 Major Changes / 重大变更

#### UI Overhaul / UI 全面重构
- **ElaWindow WinUI3 Navigation** - Integrated ElaWidgetTools component library with modern navigation layout
  - 全新 WinUI3 风格界面 - 集成 ElaWidgetTools 组件库，采用现代化导航布局
- **Theme Adaptation** - Automatic dark/light mode switching support
  - 主题自适应 - 支持深色/浅色模式自动切换
- **Unified Global Styling** - Consistent styling for manager window, context menu, and dialogs
  - 全局样式统一 - 管理器窗口、右键菜单、对话框样式一致化

#### Architecture Upgrade / 架构升级
- **Core Engine Built-in** - libshijima and libshimejifinder changed to built-in source code, no longer dependent on external submodules
  - 核心引擎内置化 - libshijima 和 libshimejifinder 改为内置源码，不再依赖外部子模块
- **Unified Logging System** - Introduced AppLog to replace original output, supporting分级日志 and CLI output
  - 统一日志系统 - 引入 AppLog 替代原有输出，支持分级日志和 CLI 输出

### ✨ Added / 新增功能

- **ElaWindow Navigation** - WinUI3 style main window interface / WinUI3 风格的主窗口界面
- **Theme Auto-adaptation** - Automatic system dark/light theme adaptation / 自动适配系统深色/浅色主题
- **System Tray** - Support minimize to system tray / 支持最小化到系统托盘
- **Bubble Interaction** - Click on bubble messages to interact / 点击气泡消息可交互
- **High-quality Rendering** - Smooth image rendering under high DPI scaling / 高 DPI 缩放下的平滑图像渲染
- **Smart Detachment** - Progressive mascot detachment when window moves quickly / 窗口快速移动时桌宠渐进式脱离
- **Enhanced i18n** - Complete Chinese/English interface support / 完整的中英文界面支持
- **Unified Logging** - Application-level logging system for easier debugging / 应用级日志系统，便于调试

### 🐛 Fixed / Bug 修复

- Fixed application unable to exit normally / 修复应用无法正常退出的问题
- Fixed freeze when exiting via tray / 修复托盘退出时卡死的问题
- Fixed mascot being minimized along with window / 修复窗口最小化时桌宠也被最小化
- Fixed UI update after language change / 修复更改语言后的界面更新问题
- Fixed hugevil character empty animation issue / 修复 hugevil 角色空动画问题
- Fixed mascot not bouncing when window moves while standing on it / 修复桌宠站在窗口上时移动窗口不弹跳
- Fixed memory issues caused by incomplete resource release / 修复资源释放不完整导致的内存问题

### 🔧 Changed / 改进与优化

- **Performance** - Optimized image rendering quality under high scaling ratios / 优化高缩放比下的图像绘制质量
- **Stability** - Enhanced exception handling and edge case protection / 增强异常处理和边界情况保护
- **Build** - Fixed Linux AppImage and CI workflow / 修复 Linux AppImage 和 CI 工作流
- **Code Structure** - Large-scale module splitting and directory reorganization / 大规模模块拆分和目录重组
- **Dependencies** - Removed unused SimpleZipImporter and miniz / 移除未使用的 SimpleZipImporter 和 miniz

---

## [0.1.0] - 2026-02-26

### 🎉 Initial Release / 初始版本

The first official release of NeurolingsCE! / NeurolingsCE 第一个正式版本发布！

As a community version of the Neuro mascot, NeurolingsCE is a complete port of the legacy mascot application. As a product of NeuForge, it remains an open-source project.

作为 Neuro 桌宠的社区版本，NeurolingsCE 完整移植了旧版本的桌宠，且作为 NeuForge 产物，依然是开源项目。

#### Highlights / 亮点

- **Performance** - At least 40x performance improvement compared to the original / 对比原桌宠，做到了至少 40 倍的性能提升
- **Memory Efficiency** - Reduced memory usage by nearly 100x with multiple mascots / 多个桌宠情况下，内存占用减小近百倍
- **Creator Friendly** - More friendly to content creators / 对创作者更加友好
- **User Friendly** - More convenient for users / 对用户更加方便
- **Tech Stack** - Core written in C++17 with Qt6 framework / 核心采用 C++17 编写，基于 Qt6 框架
- **Based on** - Heavily modified from the semi-finished Shijima-Qt project / 基于半成品项目 Shijima-Qt 进行了大量修改

---

## Contributors / 贡献者

### [0.2.0]
- [@qingchenyouforcc](https://github.com/qingchenyouforcc) - Main development (81 commits) / 主要开发
- [@wyf7685](https://github.com/wyf7685) - CI fixes and code optimization (8 commits) / CI 修复与代码优化

### [0.1.0]
- [@qingchenyouforcc](https://github.com/qingchenyouforcc) - Initial development / 初始开发

---

[0.2.0]: https://github.com/qingchenyouforcc/NeurolingsCE/compare/0.1.x...0.2.0
[0.1.0]: https://github.com/qingchenyouforcc/NeurolingsCE/releases/tag/0.1.x
