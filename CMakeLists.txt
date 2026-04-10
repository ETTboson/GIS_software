cmake_minimum_required(VERSION 3.20)

# 以下第三方库当前未随仓库一起管理，因此这些路径仍需要开发者在本机配置为绝对路径。
# 为了支持多人协作，这里只保留少量根目录变量，避免在源码中硬编码多处机器相关路径。
set(OSGEO4W_ROOT "" CACHE PATH "OSGeo4W 安装根目录，例如 D:/path/to/OSGeo4W")
set(LOG4CPP_ROOT "" CACHE PATH "log4cpp 安装根目录，例如 D:/path/to/log4cpp-1.1.4")
set(LIBXML2_ROOT "" CACHE PATH "libxml2 安装根目录，例如 D:/path/to/libxml2")

foreach(_required_root IN ITEMS OSGEO4W_ROOT LOG4CPP_ROOT LIBXML2_ROOT)
    if(NOT DEFINED ${_required_root} OR "${${_required_root}}" STREQUAL "")
        message(FATAL_ERROR
            "${_required_root} 未设置。请在首次配置 CMake 时填写本机第三方库的绝对路径，详见 Dev_manual.md。"
        )
    endif()
endforeach()

set(OSGEO4W_QT5_ROOT "${OSGEO4W_ROOT}/apps/Qt5")
set(OSGEO4W_GDAL_ROOT "${OSGEO4W_ROOT}/apps/gdal-dev")
set(OSGEO4W_QGIS_ROOT "${OSGEO4W_ROOT}/apps/qgis-ltr")
set(LOG4CPP_INCLUDE_DIR "${LOG4CPP_ROOT}/include")
set(LOG4CPP_DEBUG_LIB_DIR "${LOG4CPP_ROOT}/debug")
set(LOG4CPP_RELEASE_LIB_DIR "${LOG4CPP_ROOT}/release")
set(LIBXML2_INCLUDE_DIR "${LIBXML2_ROOT}/include")
set(LIBXML2_LIB_DIR "${LIBXML2_ROOT}/lib")

list(PREPEND CMAKE_PREFIX_PATH "${OSGEO4W_QT5_ROOT}")

project(FirstQT LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_SOURCE_DIR}/resources
    ${CMAKE_SOURCE_DIR}/src/ui/components
)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Xml Svg Sql PrintSupport)

add_executable(FirstQT
    src/main.cpp
    resources/mainwindow.ui

    src/core/ai/aimanager.cpp
    src/core/ai/aimanager.h
    src/core/ai/conversationcontext.cpp
    src/core/ai/conversationcontext.h
    src/core/ai/ollamaclient.cpp
    src/core/ai/ollamaclient.h
    src/core/ai/toolcalldispatcher.cpp
    src/core/ai/toolcalldispatcher.h
    src/core/ai/memory/systemcontextbuilder.cpp
    src/core/ai/memory/systemcontextbuilder.h
    src/core/ai/memory/memoryfilemanager.cpp
    src/core/ai/memory/memoryfilemanager.h
    src/core/workflow/analysisworkflowcoordinator.cpp
    src/core/workflow/analysisworkflowcoordinator.h
    src/core/integration/qgsappinitializer.cpp
    src/core/integration/qgsappinitializer.h
    src/core/interfaces/iaitoolhost.h

    src/model/dto/analysisresult.h
    src/model/dto/chatmessage.h
    src/model/dto/layerinfo.h
    src/model/dto/numericdataset.h
    src/model/enums/maptooltype.h
    src/model/legacy/aimodule.cpp
    src/model/legacy/aimodule.h
    src/model/legacy/analysismodule.cpp
    src/model/legacy/analysismodule.h
    src/model/legacy/datamodule.cpp
    src/model/legacy/datamodule.h

    src/service/analysis/analysisservice.cpp
    src/service/analysis/analysisservice.h
    src/service/data/dataservice.cpp
    src/service/data/dataservice.h
    src/service/data/numericdatareader.cpp
    src/service/data/numericdatareader.h

    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/docks/aidockwidget.cpp
    src/ui/docks/aidockwidget.h
    src/ui/components/imagebutton.cpp
    src/ui/components/imagebutton.h
    src/ui/map/mapcanvaswidget.cpp
    src/ui/map/mapcanvaswidget.h
    src/ui/map/mapcanvasmanager.cpp
    src/ui/map/mapcanvasmanager.h
)

target_include_directories(FirstQT PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    ${OSGEO4W_GDAL_ROOT}/include
    ${OSGEO4W_QGIS_ROOT}/include
    ${LOG4CPP_INCLUDE_DIR}
    ${LIBXML2_INCLUDE_DIR}
)

target_link_directories(FirstQT PRIVATE
    ${OSGEO4W_GDAL_ROOT}/lib
    ${OSGEO4W_QGIS_ROOT}/lib
    ${LIBXML2_LIB_DIR}
    ${OSGEO4W_QT5_ROOT}/lib
    $<$<CONFIG:Debug>:${LOG4CPP_DEBUG_LIB_DIR}>
    $<$<CONFIG:Release>:${LOG4CPP_RELEASE_LIB_DIR}>
)

target_link_libraries(FirstQT PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::Xml
    qgis_core
    qgis_gui
    gdal_i
    log4cpp
    libxml2
    iconv
)
