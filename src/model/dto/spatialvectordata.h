#ifndef SPATIALVECTORDATA_H_B6C7D8E9F0A1
#define SPATIALVECTORDATA_H_B6C7D8E9F0A1

#include <QString>
#include <QStringList>
#include <QMetaType>

// ════════════════════════════════════════════════════════
//  SpatialVectorData
//  矢量资产载荷 DTO
//  保存几何类型、要素规模、字段摘要以及来自表格转换时的坐标列信息，
//  供统一分析工作区与 AI 上下文描述矢量数据概况。
// ════════════════════════════════════════════════════════
struct SpatialVectorData
{
    QString     strGeometryType;        // 几何类型描述，如 Point / LineString / Polygon
    int         nFeatureCount = 0;      // 要素数量
    QStringList vFieldNames;            // 全部字段名列表
    QStringList vNumericFieldNames;     // 数值型字段名列表
    bool        bDerivedFromTable = false; // 是否由表格 XY 列转换得到
    QString     strCoordinateXField;    // 表格来源时的 X 字段名
    QString     strCoordinateYField;    // 表格来源时的 Y 字段名
    QString     strPreviewSummary;      // 矢量摘要文本
};

Q_DECLARE_METATYPE(SpatialVectorData)

#endif // SPATIALVECTORDATA_H_B6C7D8E9F0A1
