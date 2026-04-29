#ifndef ANALYSISINPUTDESCRIPTOR_H_F8A9B0C1D2E3
#define ANALYSISINPUTDESCRIPTOR_H_F8A9B0C1D2E3

#include <QString>
#include <QMetaType>

#include "model/dto/numericdataset.h"
#include "model/enums/analysismoduletype.h"

// ════════════════════════════════════════════════════════
//  AnalysisInputDescriptor
//  分析输入描述 DTO
//  由统一解析/路由层产出，经由 DataService 传递给 MainWindow，
//  描述当前文件应进入哪个分析模块、是否需要用户确认以及是否附带数值矩阵。
// ════════════════════════════════════════════════════════
struct AnalysisInputDescriptor
{
    QString            strSourcePath;          // 原始输入文件路径
    QString            strDisplayName;         // 界面显示名称
    QString            strSourceFormat;        // 输入格式标识，如 csv/xml/shp/simple_raster
    QString            strSummary;             // 路由摘要与数据概况说明
    QString            strChoicePrompt;        // 需要用户确认时的提示文本
    AnalysisModuleType eModuleType = AnalysisModuleType::None; // 建议目标模块
    AnalysisModuleType eAlternateModuleType = AnalysisModuleType::None; // 备选目标模块
    bool               bNeedsUserChoice = false; // 是否需要 UI 二次确认
    bool               bHasNumericData = false;  // 是否携带可直接分析的数值矩阵
    NumericDataset     dataNumeric{};            // 可直接交给 AnalysisService 的数值数据
};

Q_DECLARE_METATYPE(AnalysisInputDescriptor)

#endif // ANALYSISINPUTDESCRIPTOR_H_F8A9B0C1D2E3
