// src/model/analysisresult.h
#ifndef ANALYSISRESULT_H_C3D4E5F6A1B2
#define ANALYSISRESULT_H_C3D4E5F6A1B2

#include <QString>

// ════════════════════════════════════════════════════════
//  AnalysisResult
//  空间分析结果数据传输对象（DTO）
//  由 AnalysisService 产出，经由信号传递给 MainWindow 与 ToolCallDispatcher
// ════════════════════════════════════════════════════════
struct AnalysisResult
{
    QString strType;    // 分析类型标识，如 "buffer" / "overlay" / "query" / "raster"
    QString strDesc;    // 分析结果的人类可读描述，用于日志面板与 AI 回传
    bool    bSuccess;   // 分析是否成功执行；false 时 strDesc 描述错误原因
};

#endif // ANALYSISRESULT_H_C3D4E5F6A1B2
