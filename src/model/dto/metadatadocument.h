#ifndef METADATADOCUMENT_H_94A5B6C7D8E9
#define METADATADOCUMENT_H_94A5B6C7D8E9

#include <QString>
#include <QStringList>
#include <QMetaType>

// ════════════════════════════════════════════════════════
//  MetadataDocument
//  元数据文档载荷 DTO
//  保存 XML 根节点与轻量结构摘要，
//  供统一分析工作区展示不可统计文档类资产的基本信息。
// ════════════════════════════════════════════════════════
struct MetadataDocument
{
    QString     strRootName;       // 根节点名称
    QStringList vSummaryLines;     // 结构摘要行列表
    QString     strPreviewSummary; // 文档摘要文本
};

Q_DECLARE_METATYPE(MetadataDocument)

#endif // METADATADOCUMENT_H_94A5B6C7D8E9
