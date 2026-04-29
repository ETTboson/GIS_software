#ifndef XMLINSPECTOR_H_B9C0D1E2F3A4
#define XMLINSPECTOR_H_B9C0D1E2F3A4

#include <QString>

#include "model/dto/analysisdataasset.h"

// ════════════════════════════════════════════════════════
//  XmlInspector
//  职责：识别简单 XML 文件更适合作为矢量资产还是元数据文档资产导入。
//        首期支持轻量启发式识别：坐标点 XML 与键值/元数据 XML。
//  位于 service/data/ 层。
// ════════════════════════════════════════════════════════
class XmlInspector
{
public:
    /*
     * @brief 检查 XML 文件类型并生成统一分析资产
     * @param_1 _strFilePath: XML 文件绝对路径
     * @param_2 _outAsset: 成功时写出的分析资产 DTO
     * @param_3 _strError: 失败时写出的错误信息
     */
    static bool inspect(const QString& _strFilePath,
        AnalysisDataAsset& _outAsset,
        QString& _strError);
};

#endif // XMLINSPECTOR_H_B9C0D1E2F3A4
