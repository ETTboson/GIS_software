#ifndef VISUALIZATIONPOINT_H_E5F6A1B2C3D4
#define VISUALIZATIONPOINT_H_E5F6A1B2C3D4

#include <QString>
#include <QMetaType>

// ════════════════════════════════════════════════════════
//  VisualizationPoint
//  单个可视化数据点 DTO
//  仅承载图表绘制与交互所需的点位信息
// ════════════════════════════════════════════════════════
struct VisualizationPoint
{
    QString strLabel;       // 数据点显示标签
    double  dValue = 0.0;   // 数据点数值
    QString strToolTip;     // 悬停提示文本
    int     nRowIndex = -1; // 可选的源数据行索引
    int     nColIndex = -1; // 可选的源数据列索引
    int     nSourceIndex = -1; // 可选的源序号
};

Q_DECLARE_METATYPE(VisualizationPoint)

#endif // VISUALIZATIONPOINT_H_E5F6A1B2C3D4
