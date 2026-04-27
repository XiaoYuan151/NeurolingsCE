# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.3.3] - 2026-04-27

### 🚀 Major Changes / 重大变更

#### Stability & Mascot Interaction Fixes / 稳定性与桌宠交互修复
- **Spawn Stability** - Fixed freezes and crashes that could happen while spawning mascots
  - 生成稳定性 - 修复桌宠生成时可能出现的卡死与崩溃问题
- **Hotspot Interaction Recovery** - Restored patpat hotspot clicks and repeated patpat behavior while holding hotspots
  - 热点交互恢复 - 修复 patpat 热点点击失效，并恢复长按热点时的连续触发行为
- **Active Window Edge Following** - Fixed mascot behavior when following active window edges
  - 活动窗口边缘跟随 - 修复桌宠跟随活动窗口边缘时的异常行为

#### Logging & Debugging Improvements / 日志与调试增强
- **Enhanced Logging System** - Improved application logging and bridged more engine/runtime logs into the app log flow
  - 日志系统增强 - 改进应用日志系统，并将更多引擎与运行时日志接入应用日志流程
- **Runtime and CLI Operation Logs** - Added logs for CLI, HTTP API, local IPC, package import, mascot spawn, and shutdown flows
  - 运行时与 CLI 操作日志 - 为 CLI、HTTP API、本地 IPC、包导入、桌宠生成和关闭流程补充日志
- **Debug Documentation** - Added debugging and log-location notes to README and README_EN
  - 调试文档 - 在 README 与 README_EN 中补充日志位置和调试说明

### ✨ Added / 新增功能

- **Detailed CLI logging** - CLI command execution and runtime control now produce clearer diagnostic logs / 新增更详细的 CLI 日志，命令执行与运行时控制更便于排查
- **Runtime operation logging** - Import, package loading, mascot spawn, close, lifecycle, HTTP, and local IPC flows now include operation logs / 新增运行时操作日志，覆盖导入、包加载、生成、关闭、生命周期、HTTP 与本地 IPC 流程
- **Debugging documentation** - Added documentation for log files and troubleshooting workflows / 新增日志文件与问题排查流程说明

### 🐛 Fixed / Bug 修复

- Fixed freezes that could occur during mascot spawn / 修复桌宠生成过程中可能出现的卡死问题
- Fixed crashes that could occur during mascot spawn / 修复桌宠生成过程中可能出现的崩溃问题
- Fixed crashes that could occur when double-clicking mascots / 修复双击桌宠时可能出现的崩溃问题
- Fixed broken patpat hotspot clicks / 修复 patpat 热点点击失效的问题
- Fixed hotspot hold behavior so the hold state remains active as expected / 修复热点长按状态可能提前失效的问题
- Fixed repeated patpat triggering while holding a hotspot / 修复按住热点时 patpat 无法持续重复触发的问题
- Fixed active-window edge following behavior / 修复活动窗口边缘跟随异常的问题
- Fixed inconsistent settings access for environment parameters and related close/API behavior / 修复环境参数设置访问不一致及相关关闭/API 行为问题

### 🔧 Changed / 改进与优化

- **Interaction Experience** - Refined patpat click, hold, and repeated trigger behavior / 交互体验优化 - 调整 patpat 点击、长按和重复触发逻辑
- **Runtime Safety** - Strengthened boundary handling around spawn, animation, behavior switching, and environment synchronization / 运行时安全性优化 - 增强生成、动画、行为切换和环境同步过程中的边界保护
- **Settings Management** - Centralized environment parameter handling for scale, detachment threshold, breeding toggle, color, and proxy settings / 设置管理优化 - 集中管理缩放、分离阈值、繁殖开关、颜色和代理等环境参数
- **Runtime Structure** - Split mascot runtime state and tray-control responsibilities, and extracted mascot command dispatch with core tests / 运行时结构优化 - 拆分桌宠运行时状态与托盘控制职责，并抽离桌宠命令分发与核心测试
- **Speech Bubble Styling** - Refined speech bubble corner radius and shadow / 对话气泡优化 - 调整桌宠对话气泡圆角与阴影效果
- **Release Metadata** - Refreshed 0.3.3 version metadata / 发布元数据整理 - 更新 0.3.3 版本元数据

---

## [0.3.2] - 2026-04-24

### 🚀 Major Changes / 重大变更

#### Mascot Package Format Upgrade / 桌宠包格式升级
- **Self-contained `.mascot` Packages** - Mascot templates are now stored as single package files instead of extracted folders
  - 自包含 `.mascot` 包 - 桌宠模板现在以单个包文件保存，不再直接作为解压后的文件夹存放
- **Mascot Metadata Support** - Added `info.json` metadata with name, version, description, and author fields
  - 桌宠元数据支持 - 新增 `info.json` 元数据，支持名称、版本、描述和作者信息
- **Legacy Directory Migration** - Existing legacy `.mascot` directories are migrated into the new package format automatically
  - 旧目录自动迁移 - 已存在的旧版 `.mascot` 文件夹会自动迁移为新的包格式

#### Library & Control Surface Improvements / 桌宠库与控制接口增强
- **Mascot Details Panel** - Added a home-page details panel showing preview, version, author, and description for the selected mascot
  - 桌宠详情面板 - 在主页新增详情面板，展示所选桌宠的预览图、版本、作者和描述
- **CLI/API Metadata Output** - CLI and JSON API loaded mascot results now include mascot metadata
  - CLI/API 元数据输出 - CLI 与 JSON API 的已加载桌宠结果现在会包含桌宠元数据
- **Default Mascot Metadata** - Bundled default mascot now includes `info.json` and participates in the same metadata flow
  - 默认桌宠元数据 - 内置默认桌宠现在包含 `info.json`，并接入统一元数据流程

### ✨ Added / 新增功能

- **MascotPackage helper** - Added a shared package helper for reading metadata, installing packages, extracting packages, and migrating legacy folders / 新增 `MascotPackage` 工具，用于读取元数据、安装包、解包和迁移旧目录
- **Package cache directory** - Added a dedicated mascot cache directory for runtime extraction / 新增桌宠缓存目录，用于运行时解包
- **Package metadata fields** - Added version, description, and author fields to loaded mascot information / 为已加载桌宠信息新增版本、描述和作者字段
- **Home-page details UI** - Added selected mascot details and richer list tooltips / 新增主页所选桌宠详情展示与更丰富的列表提示
- **Direct `.mascot` import** - Import workflow can now install `.mascot` package files directly / 导入流程现在可以直接安装 `.mascot` 包文件

### 🐛 Fixed / Bug 修复

- Fixed mascot removal so deleting a template removes the package file instead of trying to delete extracted runtime contents / 修复桌宠删除逻辑，现在会删除包文件，而不是尝试删除运行时解包内容
- Fixed legacy import metadata gaps by generating fallback `info.json` for old mascot folders / 修复旧版导入缺少元数据的问题，会为旧目录生成回退 `info.json`
- Fixed package inspection to reject incomplete mascot packages that miss required XML files or preview images / 修复包检查逻辑，会拒绝缺少必要 XML 文件或预览图片的不完整桌宠包
- Fixed archive filtering so `json` and `txt` files required by mascot metadata and bubble context can be preserved / 修复压缩包过滤逻辑，保留桌宠元数据和气泡上下文所需的 `json` 与 `txt` 文件

### 🔧 Changed / 改进与优化

- **Storage Model** - Mascot storage now tracks package paths separately from extracted cache paths / 存储模型优化 - 桌宠存储现在区分包路径和解包缓存路径
- **Import Workflow** - Legacy Shimeji archives are packaged during import for consistent storage behavior / 导入流程优化 - 旧版 Shimeji 压缩包会在导入时重新打包，保持统一存储行为
- **CLI Output** - Human-readable CLI list output now shows version and author when available / CLI 输出优化 - 可读 CLI 列表输出现在会在可用时展示版本和作者
- **Build Integration** - Added package support source files and default `info.json` to both CMake and Make build paths / 构建集成优化 - 在 CMake 与 Make 构建路径中加入包格式支持源码和默认 `info.json`
- **Release Metadata** - Refreshed 0.3.2 version metadata for packaging and platform manifests / 发布元数据整理 - 更新 0.3.2 版本元数据、打包信息与平台清单

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

### [0.3.3]
- [@qingchenyouforcc](https://github.com/qingchenyouforcc) - Main development, stability fixes, logging, runtime interaction fixes / 主要开发，完成稳定性修复、日志增强与运行时交互修复
- [@wyf7685](https://github.com/wyf7685) - CI fixes and code optimization / CI 修复与代码优化

### [0.3.2]
- [@qingchenyouforcc](https://github.com/qingchenyouforcc) - Main development, mascot package format, metadata, home-page details / 主要开发，完成桌宠包格式、元数据与主页详情展示
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

[0.3.3]: https://github.com/qingchenyouforcc/NeurolingsCE/compare/0.3.2...0.3.3
[0.3.2]: https://github.com/qingchenyouforcc/NeurolingsCE/compare/0.3.1...0.3.2
[0.3.1]: https://github.com/qingchenyouforcc/NeurolingsCE/compare/0.3.0...0.3.1
[0.3.0]: https://github.com/qingchenyouforcc/NeurolingsCE/compare/0.2.0...0.3.0
[0.2.0]: https://github.com/qingchenyouforcc/NeurolingsCE/compare/0.1.x...0.2.0
[0.1.0]: https://github.com/qingchenyouforcc/NeurolingsCE/releases/tag/0.1.x
