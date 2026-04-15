#ifndef VISUALIZATIONSERIES_H_D4E5F6A1B2C3
#define VISUALIZATIONSERIES_H_D4E5F6A1B2C3

#include <QString>
#include <QVector>
#include <QMetaType>

#include "model/dto/visualizationpoint.h"

// ════════════════════════════════════════════════════════
//  VisualizationSeries
//  图表系列 DTO
//  表示同一条系列曲线或同一组柱形的数据点集合
// ════════════════════════════════════════════════════════
struct VisualizationSeries
{
    QString                    strName;      // 系列名称
    QString                    strColorHex;  // 系列颜色（十六进制文本）
    QVector<VisualizationPoint> vPoints;     // 当前系列包含的数据点
};

Q_DECLARE_METATYPE(VisualizationSeries)

#endif // VISUALIZATIONSERIES_H_D4E5F6A1B2C3
