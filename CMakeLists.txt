cmake_minimum_required(VERSION 3.20)
project(FirstQT LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_SOURCE_DIR}/resources
    ${CMAKE_SOURCE_DIR}/src/ui/style
)

# ── Qt5 路径（保持和之前一致）──────────────────────────────
set(CMAKE_PREFIX_PATH "D:/OSGeo4W/apps/Qt5")

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Xml Svg Sql PrintSupport)

add_executable(FirstQT
    src/main.cpp
    resources/mainwindow.ui

    src/core/aimanager.cpp
    src/core/aimanager.h
    src/core/conversationcontext.cpp
    src/core/conversationcontext.h
    src/core/memory/systemcontextbuilder.cpp
    src/core/memory/systemcontextbuilder.h
    src/core/memory/memoryfilemanager.cpp
    src/core/memory/memoryfilemanager.h
    src/core/ollamaclient.cpp
    src/core/ollamaclient.h
    src/core/toolcalldispatcher.cpp
    src/core/toolcalldispatcher.h
    src/core/qgsappinitializer.cpp
    src/core/qgsappinitializer.h

    src/model/analysisresult.h
    src/model/chatmessage.h
    src/model/layerinfo.h
    src/model/maptooltype.h
    src/model/numericdataset.h

    src/service/analysisservice.cpp
    src/service/analysisservice.h
    src/service/dataservice.cpp
    src/service/dataservice.h
    src/service/numericdatareader.cpp
    src/service/numericdatareader.h

    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/docks/aidockwidget.cpp
    src/ui/docks/aidockwidget.h
    src/ui/style/imagebutton.cpp
    src/ui/style/imagebutton.h
    src/ui/widgets/mapcanvaswidget.cpp
    src/ui/widgets/mapcanvaswidget.h
    src/ui/widgets/mapcanvasmanager.cpp
    src/ui/widgets/mapcanvasmanager.h
)

target_include_directories(FirstQT PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/style
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    D:/OSGeo4W/apps/gdal-dev/include
    D:/OSGeo4W/apps/qgis-ltr/include
    D:/OOP_2026/log4cpp-1.1.4/include
    D:/OOP_2026/libxml2/include
)

target_link_directories(FirstQT PRIVATE
    D:/OSGeo4W/apps/gdal-dev/lib
    D:/OSGeo4W/apps/qgis-ltr/lib
    D:/OOP_2026/libxml2/lib
    D:/OSGeo4W/apps/Qt5/lib
    $<$<CONFIG:Debug>:D:/OOP_2026/log4cpp-1.1.4/debug>
    $<$<CONFIG:Release>:D:/OOP_2026/log4cpp-1.1.4/release>
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
