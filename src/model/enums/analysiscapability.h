#ifndef ANALYSISCAPABILITY_H_D4E5F6A7B8C9
#define ANALYSISCAPABILITY_H_D4E5F6A7B8C9

#include <QFlags>
#include <QMetaType>

// ════════════════════════════════════════════════════════
//  AnalysisCapability
//  统一分析能力标志枚举
//  描述某个分析资产当前可参与的能力集合，
//  供工作区工具启用判断、AI 上下文描述与宿主执行校验使用。
// ════════════════════════════════════════════════════════
enum class AnalysisCapability
{
    None = 0x00,
    Statistical = 0x01,
    SpatialRaster = 0x02,
    SpatialVector = 0x04,
    AttributeQuery = 0x08
};

Q_DECLARE_FLAGS(AnalysisCapabilities, AnalysisCapability)
Q_DECLARE_OPERATORS_FOR_FLAGS(AnalysisCapabilities)
Q_DECLARE_METATYPE(AnalysisCapabilities)

#endif // ANALYSISCAPABILITY_H_D4E5F6A7B8C9
