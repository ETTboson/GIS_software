#ifndef SPATIALRASTERDATA_H_C7D8E9F0A1B2
#define SPATIALRASTERDATA_H_C7D8E9F0A1B2

#include <QString>
#include <QMetaType>

// ════════════════════════════════════════════════════════
//  SpatialRasterData
//  栅格资产载荷 DTO
//  保存栅格摘要信息与数值矩阵元信息，
//  供统一分析工作区展示数据概况与能力启用状态。
// ════════════════════════════════════════════════════════
struct SpatialRasterData
{
    QString strFormat;              // 栅格格式标识，如 asc / tif / tiff / img
    int     nRows = 0;              // 栅格行数
    int     nCols = 0;              // 栅格列数
    int     nBandCount = 0;         // 波段数量，首期统计默认使用第一波段
    bool    bHasNoDataValue = false; // 当前是否识别到 NoData 标记
    double  dNoDataValue = 0.0;     // NoData 数值
    QString strPreviewSummary;      // 栅格摘要文本
};

Q_DECLARE_METATYPE(SpatialRasterData)

#endif // SPATIALRASTERDATA_H_C7D8E9F0A1B2
