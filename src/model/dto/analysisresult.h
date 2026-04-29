// src/model/dto/analysisresult.h
#ifndef ANALYSISRESULT_H_C3D4E5F6A1B2
#define ANALYSISRESULT_H_C3D4E5F6A1B2

#include <QString>

#include "model/dto/visualizationdata.h"

// ════════════════════════════════════════════════════════
//  AnalysisResult
//  统一分析结果数据传输对象（DTO）
//  由统计分析服务、空间分析服务或属性查询占位服务产出，
//  经由信号传递给 MainWindow、工作区与 ToolCallDispatcher
// ════════════════════════════════════════════════════════
struct AnalysisResult
{
    QString           strType;             // 分析类型标识，如 "basic_statistics" / "frequency_statistics"
    QString           strToolId;           // 工具唯一标识，供结果历史与自动刷新缓存使用
    QString           strSourceAssetId;    // 当前结果对应的源资产 ID
    QString           strSourceAssetName;  // 当前结果对应的源资产名称
    QString           strDesc;             // 分析结果的人类可读描述，用于日志面板与 AI 回传
    bool              bSuccess = false;    // 分析是否成功执行；false 时 strDesc 描述错误原因
    bool              bHasVisualization = false; // 当前结果是否携带结构化可视化数据
    VisualizationData dataVisualization;   // 结构化可视化数据载荷
};

#endif // ANALYSISRESULT_H_C3D4E5F6A1B2
