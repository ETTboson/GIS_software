# 开发人员依赖路径配置说明

## 1. 文档目的

本项目已经将 `CMakeLists.txt` 中的大部分硬编码绝对路径收敛为少量可配置变量，方便多人协作。

项目内部源码、资源文件都使用仓库内相对路径管理；

但是 Qt5、GDAL、QGIS、log4cpp、libxml2 目前仍然安装在开发者各自机器上，没有随仓库一起分发，因此这些第三方库的安装根目录仍然需要按本机环境配置为绝对路径。

## 2. 需要开发者修改的 CMake 变量

首次在本机配置项目时，请填写以下 3 个变量：

| 变量名 | 作用 | 应填写的目录 |
| --- | --- | --- |
| `OSGEO4W_ROOT` | 提供 Qt5、GDAL、QGIS 的统一根目录 | OSGeo4W 安装根目录 |
| `LOG4CPP_ROOT` | 提供 log4cpp 的头文件和调试/发布库目录 | log4cpp 安装根目录 |
| `LIBXML2_ROOT` | 提供 libxml2 的头文件和库目录 | libxml2 安装根目录 |

## 3. 每个变量的目录层级要求

### 3.1 `OSGEO4W_ROOT`

`OSGEO4W_ROOT` 应指向 OSGeo4W 的安装根目录。CMake 会基于这个根目录自动拼接出以下路径：

- `${OSGEO4W_ROOT}/apps/Qt5`
- `${OSGEO4W_ROOT}/apps/Qt5/lib`
- `${OSGEO4W_ROOT}/apps/gdal-dev/include`
- `${OSGEO4W_ROOT}/apps/gdal-dev/lib`
- `${OSGEO4W_ROOT}/apps/qgis-ltr/include`
- `${OSGEO4W_ROOT}/apps/qgis-ltr/lib`

也就是说，开发者不需要分别填写 Qt、GDAL、QGIS 的多个路径

但注意**需要验证 OSGeo4W 目录结构完整**

### 3.2 `LOG4CPP_ROOT`

`LOG4CPP_ROOT` 应指向 log4cpp 安装根目录。CMake 会自动使用以下子目录：

- `${LOG4CPP_ROOT}/include`
- `${LOG4CPP_ROOT}/debug`
- `${LOG4CPP_ROOT}/release`

请确认当前机器上的 log4cpp 安装目录同时包含头文件目录和 Debug/Release 对应的库目录，否则链接阶段可能失败。

### 3.3 `LIBXML2_ROOT`

`LIBXML2_ROOT` 应指向 libxml2 安装根目录。CMake 会自动使用以下子目录：

- `${LIBXML2_ROOT}/include`
- `${LIBXML2_ROOT}/lib`

## 4. 推荐配置方式

可以使用以下任意一种方式传入变量。

### 4.1 使用命令行配置

```powershell
cmake -S . -B build ^
  -DOSGEO4W_ROOT="D:/path/to/OSGeo4W" ^
  -DLOG4CPP_ROOT="D:/path/to/log4cpp-1.1.4" ^
  -DLIBXML2_ROOT="D:/path/to/libxml2"
```

如果你使用 PowerShell，也可以写成：

```powershell
cmake -S . -B build `
  -DOSGEO4W_ROOT="D:/path/to/OSGeo4W" `
  -DLOG4CPP_ROOT="D:/path/to/log4cpp-1.1.4" `
  -DLIBXML2_ROOT="D:/path/to/libxml2"
```

### 4.2 使用 CMake GUI

- 选择源码目录和构建目录。
- 点击 Configure。
- 在变量列表中填写 `OSGEO4W_ROOT`、`LOG4CPP_ROOT`、`LIBXML2_ROOT`。
- 再次点击 Configure，确认无报错后点击 Generate。

### 4.3 使用 Visual Studio / CLion / Qt Creator

- 在 IDE 的 CMake 配置页面中添加 3 个缓存变量。
- 变量名必须与 `CMakeLists.txt` 中保持一致。
- 修改后重新执行 CMake Configure。

### 4.4 修改CmakeList.txt 进行配置
- 打开CMakeLists.txt,找到如下片段
```CMakeLists.txt
set(OSGEO4W_ROOT "" CACHE PATH "OSGeo4W 安装根目录，例如 D:/path/to/OSGeo4W")
set(LOG4CPP_ROOT "" CACHE PATH "log4cpp 安装根目录，例如 D:/path/to/log4cpp-1.1.4")
set(LIBXML2_ROOT "" CACHE PATH "libxml2 安装根目录，例如 D:/path/to/libxml2")
```
D:/OSGeo4W
D:/OOP_2026/libxml2
D:/OOP_2026/log4cpp-1.1.4
- 把其中的路径补全，形如下面的样子
```CMakeLists.txt
set(OSGEO4W_ROOT "D:/OSGeo4W" CACHE PATH "OSGeo4W 安装根目录")
set(LOG4CPP_ROOT "D:/OOP_2026/log4cpp-1.1.4" CACHE PATH "log4cpp 安装根目录")
set(LIBXML2_ROOT "D:/OOP_2026/libxml2" CACHE PATH "libxml2 安装根目录")
```
- **特别强调！如果需要把代码文件同步到代码仓库，务必检查提交的CMakeLists.txt，确保没有使用本机的绝对路径，提交时形如下面的样子**
```CMakeLists.txt
set(OSGEO4W_ROOT "" CACHE PATH "OSGeo4W 安装根目录，例如 D:/path/to/OSGeo4W")
set(LOG4CPP_ROOT "" CACHE PATH "log4cpp 安装根目录，例如 D:/path/to/log4cpp-1.1.4")
set(LIBXML2_ROOT "" CACHE PATH "libxml2 安装根目录，例如 D:/path/to/libxml2")
```
## 5. 注意

- 不要把自己的本机绝对路径重新写回 `CMakeLists.txt`
- 不要把自己的本机绝对路径重新写回 `CMakeLists.txt`
- 不要把自己的本机绝对路径重新写回 `CMakeLists.txt`
