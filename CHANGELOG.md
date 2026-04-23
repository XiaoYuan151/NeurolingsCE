# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.3.1] - 2026-04-24

### 🚀 Major Changes / 重大变更

#### Update Experience Upgrade / 更新体验升级
- **GitHub Update Check** - Added built-in update checking against GitHub releases
  - GitHub 更新检查 - 新增基于 GitHub Release 的内置更新检查能力
- **About Page Update Center** - Added an update center entry in the about page for version and release information
  - 关于页更新中心 - 在关于页面中新增更新中心入口，用于展示版本与发布信息
- **Settings Page Rework** - Refactored the settings page layout for a clearer and more maintainable structure
  - 设置页重构 - 重构设置页面布局，使功能分区更清晰、结构更易维护

#### Localization & Release Metadata / 本地化与发布元数据
- **Chinese Translation Completion** - Added Chinese translations for the new update-related UI
  - 中文翻译补全 - 为新增更新功能相关界面补充中文翻译
- **Version Metadata Sync** - Synchronized the 0.3.1 version metadata for release packaging and display
  - 版本元数据同步 - 同步 0.3.1 版本元数据，用于发布打包与版本展示

### ✨ Added / 新增功能

- **GitHub release update check** - The application can now check whether a newer version is available on GitHub / 新增 GitHub Release 更新检查能力，应用现在可以检查是否存在新版本
- **In-app update center** - Added update information entry in the about page / 新增应用内更新中心入口，可在关于页查看更新信息
- **Reorganized settings UI** - Improved settings page grouping and readability / 新增更清晰的设置页组织方式，提升可读性与易用性
- **Update feature localization** - Added Chinese text coverage for update-related actions and prompts / 新增更新功能相关操作与提示的中文文本覆盖

### 🐛 Fixed / Bug 修复

- Fixed missing or incomplete Chinese text in the new update-related UI / 修复新增更新相关界面中缺失或不完整的中文文本
- Fixed settings page organization issues that made some options harder to find / 修复设置页结构较杂乱、部分选项不易定位的问题
- Fixed version metadata inconsistency affecting release-facing version display / 修复版本元数据不同步导致的发布侧版本展示不一致问题

### 🔧 Changed / 改进与优化

- **Update Workflow** - Improved how users discover new versions from within the application / 更新流程优化 - 改善用户在应用内发现新版本的方式
- **UI Structure** - Refined the settings page information hierarchy and page layout / 界面结构优化 - 调整设置页信息层级与页面布局
- **Localization** - Expanded translation coverage around the update feature / 本地化优化 - 完善更新功能周边的翻译覆盖
- **Release Preparation** - Refreshed 0.3.1 metadata for packaging and release notes alignment / 发布准备优化 - 刷新 0.3.1 元数据，使打包结果与发布说明保持一致

---

## [0.3.0] - 2026-04-23

### 🚀 Major Changes / 重大变更

#### CLI & Runtime Control Upgrade / CLI 与运行时控制升级
- **Standalone CLI Program** - Added `NeurolingsCE-cli` as an independent command line entry for mascot management and runtime control
  - 独立 CLI 程序 - 新增 `NeurolingsCE-cli`，可作为独立的桌宠管理与运行时控制入口
- **Local IPC Control Path** - Runtime mascot control commands now communicate through local IPC instead of HTTP, with automatic runtime startup when needed
  - 本地 IPC 控制链路 - 运行时桌宠控制命令现通过本地 IPC 通信，并可在需要时自动启动运行时
- **Unified Command Service** - Refactored the command execution flow so CLI and HTTP interfaces share the same core control logic
  - 统一命令服务 - 重构命令执行流程，使 CLI 与 HTTP 接口共用同一套核心控制逻辑

#### Workflow & Packaging Improvements / 工作流与打包增强
- **Mascot Library Refresh** - Added refresh support on the main page to rescan and sync mascot templates
  - 桌宠库刷新 - 在主页新增刷新能力，可重新扫描并同步桌宠模板库
- **Windows Installer Pipeline** - Added Windows installer packaging flow and related release assets
  - Windows 安装器流水线 - 新增 Windows 安装器打包流程及相关发布资源
- **Companion Skill Split** - Split mascot control and companion skill workflows for cleaner automation usage
  - 配套 Skill 拆分 - 拆分桌宠控制与陪伴 skill 工作流，提升自动化调用清晰度

### ✨ Added / 新增功能

- **Independent CLI executable** - `NeurolingsCE-cli` can be used directly in shell scripts, automation tools, and agent workflows / 新增独立 CLI 可执行文件 `NeurolingsCE-cli`，可直接用于脚本、自动化和 agent 工作流
- **Automatic runtime bootstrap** - Mascot control commands can automatically connect to or start the NeurolingsCE runtime when required / 新增运行时自动拉起能力，桌宠控制命令可按需自动连接或启动 NeurolingsCE 运行时
- **Main page refresh action** - Added a refresh button to update mascot data and synchronize the mascot library / 新增主页刷新按钮，可更新桌宠数据并同步桌宠库
- **NeurolingsCE skill tooling** - Added companion skill support, CLI finder cache tooling, and summon helper scripts / 新增 NeurolingsCE 配套 skill、CLI 查找缓存工具与召唤辅助脚本
- **Windows installer support** - Added WiX-based Windows installer and packaging workflow for release distribution / 新增基于 WiX 的 Windows 安装器与打包工作流，便于发布分发

### 🐛 Fixed / Bug 修复

- Fixed cases where mascot windows could still be covered by the taskbar due to topmost layer refresh issues / 修复因置顶层级刷新异常导致桌宠仍可能被任务栏遮挡的问题
- Fixed validation issues in mascot-related CLI command flows and filtered incompatible legacy behavior more safely / 修复桌宠相关 CLI 命令流程中的校验问题，并更安全地过滤不兼容的旧行为
- Fixed edge-case crashes by strengthening crash logging and asset fallback behavior / 通过增强崩溃日志与资源回退逻辑，修复若干边界情况下的崩溃问题
- Fixed existing-instance wake-up behavior and adjusted log file output location / 修复唤醒已有实例的行为问题，并调整日志落盘位置

### 🔧 Changed / 改进与优化

- **Control Architecture** - Reworked mascot control around local IPC for a more reliable desktop-native control path / 控制架构优化 - 围绕本地 IPC 重构桌宠控制流程，使桌面原生控制链路更加稳定
- **Automation Experience** - Optimized summon semantics and CLI path reuse for automated and skill-based invocation / 自动化体验优化 - 优化召唤语义与 CLI 路径复用，提升自动化与 skill 调用体验
- **Documentation** - Updated CLI, local IPC, skill usage, packaging, screenshots, and icon acknowledgements in the documentation / 文档更新 - 补充并更新 CLI、本地 IPC、skill 用法、打包说明、项目截图与图标致谢
- **Release Assets** - Refreshed version metadata, package resources, and release-facing application assets / 发布资源整理 - 更新版本元数据、打包资源与面向发布的应用素材

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

### [0.3.1]
- [@qingchenyouforcc](https://github.com/qingchenyouforcc) - Main development, update center, settings page rework, localization updates / 主要开发，完成更新中心、设置页重构与本地化更新
- [@wyf7685](https://github.com/wyf7685) - CI fixes and code optimization / CI 修复与代码优化

### [0.3.0]
- [@qingchenyouforcc](https://github.com/qingchenyouforcc) - Main development, CLI/IPC integration, packaging, documentation updates / 主要开发，完成 CLI/IPC 集成、打包与文档更新
- [@wyf7685](https://github.com/wyf7685) - CI fixes and code optimization / CI 修复与代码优化

### [0.2.0]
- [@qingchenyouforcc](https://github.com/qingchenyouforcc) - Main development (81 commits) / 主要开发
- [@wyf7685](https://github.com/wyf7685) - CI fixes and code optimization (8 commits) / CI 修复与代码优化

### [0.1.0]
- [@qingchenyouforcc](https://github.com/qingchenyouforcc) - Initial development / 初始开发

---

[0.3.1]: https://github.com/qingchenyouforcc/NeurolingsCE/compare/0.3.0...0.3.1
[0.3.0]: https://github.com/qingchenyouforcc/NeurolingsCE/compare/0.2.0...0.3.0
[0.2.0]: https://github.com/qingchenyouforcc/NeurolingsCE/compare/0.1.x...0.2.0
[0.1.0]: https://github.com/qingchenyouforcc/NeurolingsCE/releases/tag/0.1.x
