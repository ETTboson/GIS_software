#ifndef DATAASSETTYPE_H_E4F5A6B7C8D9
#define DATAASSETTYPE_H_E4F5A6B7C8D9

#include <QMetaType>

// ════════════════════════════════════════════════════════
//  DataAssetType
//  统一分析数据资产类型枚举
//  用于标识当前导入资产属于栅格、矢量、表格还是元数据文档，
//  供数据解析层、仓库层、工作区 UI 与 AI 上下文共享。
// ════════════════════════════════════════════════════════
enum class DataAssetType
{
    None = 0,
    Raster,
    Vector,
    Table,
    MetadataDocument
};

Q_DECLARE_METATYPE(DataAssetType)

#endif // DATAASSETTYPE_H_E4F5A6B7C8D9
