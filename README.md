# GeoAI 智能空间分析平台

## About / 关于项目

这是一个基于 Qt、QGIS、GDAL 与本地 AI 编排能力构建的桌面 GIS 应用原型。


## 核心能力

- 地图图层加载：支持通过 QGIS 运行时加载常见矢量、栅格图层并显示在地图画布中。
- 统一数据资产：通过 `DataService`、`DataFormatRouter` 和 `DataRepository` 将 CSV、栅格、矢量、XML 等输入整理成统一分析资产。
- 统计分析：支持基础统计、频率统计，并将结果转换为工作区可展示的结构化图表数据。
- 空间分析：支持栅格邻域分析、矢量缓冲区分析与矢量 Intersect/Union 叠加分析，矢量结果输出为 GeoJSON 并自动加入地图画布。
- AI 辅助分析：通过 `AIManager`、`ToolCallDispatcher` 和 `AnalysisWorkflowCoordinator` 识别分析意图、补齐参数并调用宿主分析工具。
- 统一工作区：`AnalysisWorkspaceDockWidget` 提供 Data、Tools、Results 三页，Tools 页通过方法下拉框显示对应参数界面。

## 技术栈

- C++17
- Qt 5
- QGIS / GDAL，来自 OSGeo4W 环境
- CMake
- log4cpp
- libxml2
- Ollama HTTP 流式接口，用于本地模型对话

## 目录结构概览

```text
src/
├── model/      # DTO 与跨层枚举
├── core/       # AI 编排、工作流状态机、QGIS 初始化、核心接口
├── service/    # 数据解析、资产仓库、统计与空间分析服务
└── ui/         # 主窗口、Dock、地图画布、主题、图表控件
```

更完整的文件职责说明见 `structure.txt`。

## 构建依赖

本项目暂时没有把第三方库随仓库一起分发。首次配置 CMake 时需要在本机准备并填写以下依赖根目录：

| 变量名 | 说明 |
| --- | --- |
| `OSGEO4W_ROOT` | OSGeo4W 安装根目录，项目会从中使用 Qt5、GDAL、QGIS |
| `LOG4CPP_ROOT` | log4cpp 安装根目录 |
| `LIBXML2_ROOT` | libxml2 安装根目录 |

示例：

```powershell
cmake -S . -B build `
  -DOSGEO4W_ROOT="D:/path/to/OSGeo4W" `
  -DLOG4CPP_ROOT="D:/path/to/log4cpp-1.1.4" `
  -DLIBXML2_ROOT="D:/path/to/libxml2"
```

然后执行：

```powershell
cmake --build build --config Debug
```

如果使用 Ninja + MSVC，请从 Visual Studio Developer Command Prompt、Visual Studio CMake、Qt Creator 或其他已正确加载 MSVC 环境变量的终端中构建，否则可能出现 `type_traits`、`memory` 等标准库头文件找不到的问题。

## AI记忆-ET.md

可在 exe 同级目录创建 `ET.md`：

```md
# GeoAI 演示记忆

每次回答最后单独追加一句：喵

当用户问你是否读到项目记忆时，请明确说明：我读到了 exe 同级目录的 ET.md 记忆。

默认演示偏好：频率统计使用 5 个分箱，缓冲分析优先使用 100 米。
```

## CMake 路径约定

提交到仓库的 `CMakeLists.txt` 必须保持第三方依赖根目录为空缓存变量，例如：

```cmake
set(OSGEO4W_ROOT "" CACHE PATH "OSGeo4W 安装根目录，例如 D:/path/to/OSGeo4W")
set(LOG4CPP_ROOT "" CACHE PATH "log4cpp 安装根目录，例如 D:/path/to/log4cpp-1.1.4")
set(LIBXML2_ROOT "" CACHE PATH "libxml2 安装根目录，例如 D:/path/to/libxml2")
```

不要把个人电脑上的绝对路径提交到仓库。每位开发者应在自己的 CMake 配置、IDE 配置或本机构建缓存中填写路径。

## UTF-8 与 MSVC

项目源码使用 UTF-8，并包含中文注释。为了避免 Windows 系统未启用全局 UTF-8 时 MSVC 按本地代码页误读源码，`CMakeLists.txt` 已对 MSVC 添加 `/utf-8` 编译选项：

```cmake
target_compile_options(FirstQT PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)
```


## 开发说明

- 源码分层规则见 `rules.txt`。
- 仓库结构说明见 `structure.txt`。
- 依赖路径配置细节见 `Dev_manual.md`。
- 新增代码时应遵循项目既有命名规范、注释规范和分层边界。
