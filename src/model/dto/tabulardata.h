#ifndef TABULARDATA_H_A5B6C7D8E9F0
#define TABULARDATA_H_A5B6C7D8E9F0

#include <QString>
#include <QStringList>
#include <QMetaType>

// ════════════════════════════════════════════════════════
//  TabularData
//  表格资产载荷 DTO
//  保存列结构、数值列、坐标列识别结果与摘要信息，
//  供统一分析工作区决定统计能力与 CSV 转点提示。
// ════════════════════════════════════════════════════════
struct TabularData
{
    int         nRowCount = 0;           // 数据行数
    int         nColCount = 0;           // 列数
    QStringList vFieldNames;             // 全部字段名
    QStringList vNumericFieldNames;      // 数值字段名
    bool        bHasCoordinateColumns = false; // 是否检测到坐标列
    QString     strCoordinateXField;     // 识别到的 X/Lon 字段名
    QString     strCoordinateYField;     // 识别到的 Y/Lat 字段名
    QString     strPreviewSummary;       // 表格摘要文本
};

Q_DECLARE_METATYPE(TabularData)

#endif // TABULARDATA_H_A5B6C7D8E9F0
