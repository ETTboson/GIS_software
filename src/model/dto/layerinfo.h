// src/model/dto/layerinfo.h
#ifndef LAYERINFO_H_A1B2C3D4E5F6
#define LAYERINFO_H_A1B2C3D4E5F6

#include <QString>

// ════════════════════════════════════════════════════════
//  LayerInfo
//  图层信息数据传输对象（DTO）
//  纯数据结构，不含任何业务逻辑与信号槽
//  在 DataService、MainWindow、图层面板之间传递图层元信息
// ════════════════════════════════════════════════════════
struct LayerInfo
{
    QString strFilePath;       // 图层对应的本地文件绝对路径
    QString strSourceUri;      // QGIS provider 可直接加载的源 URI，文件图层通常等于 strFilePath
    QString strProviderKey;    // QGIS provider 标识，如 "ogr" / "gdal"
    QString strDatabasePath;   // 空间数据库文件路径，非数据库图层为空
    QString strTableName;      // 空间数据库表名，非数据库图层为空
    QString strGeometryColumn; // 空间数据库几何列名，非数据库图层为空
    QString strName;           // 图层显示名称（通常取文件名或空间表名）
    QString strType;           // 图层类型标识，如 "vector" / "raster"
    bool    bVisible;          // 图层当前是否在地图视图中显示
};

#endif // LAYERINFO_H_A1B2C3D4E5F6
