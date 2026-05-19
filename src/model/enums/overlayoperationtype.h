#ifndef OVERLAYOPERATIONTYPE_H_B5C6D7E8F9A0
#define OVERLAYOPERATIONTYPE_H_B5C6D7E8F9A0

#include <QMetaType>

// ════════════════════════════════════════════════════════
//  OverlayOperationType
//  矢量叠加分析操作类型
//  用于 UI、服务层与 AI 工具调用之间统一描述 Intersect / Union。
// ════════════════════════════════════════════════════════
enum class OverlayOperationType
{
    Intersect,
    Union
};

Q_DECLARE_METATYPE(OverlayOperationType)

#endif // OVERLAYOPERATIONTYPE_H_B5C6D7E8F9A0
