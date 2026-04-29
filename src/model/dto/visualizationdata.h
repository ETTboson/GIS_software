#ifndef VISUALIZATIONDATA_H_C3D4E5F6A1B2
#define VISUALIZATIONDATA_H_C3D4E5F6A1B2

#include <QString>
#include <QVector>
#include <QMetaType>

#include "model/enums/visualizationcharttype.h"
#include "model/dto/visualizationseries.h"

// ════════════════════════════════════════════════════════
//  VisualizationData
//  图表展示 DTO
//  由统计分析服务或空间分析服务生成，经由 AnalysisResult 传递到 UI 可视化层
// ════════════════════════════════════════════════════════
struct VisualizationData
{
    QString                    strTitle;               // 图表标题
    QString                    strXAxisTitle;          // X 轴标题
    QString                    strYAxisTitle;          // Y 轴标题
    VisualizationChartType     chartTypeSuggested = VisualizationChartType::BarChart; // 推荐图表类型
    QVector<VisualizationSeries> vSeries;             // 图表系列列表
    QString                    strSourceAnalysisType;  // 来源分析类型
};

Q_DECLARE_METATYPE(VisualizationData)

#endif // VISUALIZATIONDATA_H_C3D4E5F6A1B2
