#ifndef ATTRIBUTEQUERYSERVICE_H_D0E1F2A3B4C5
#define ATTRIBUTEQUERYSERVICE_H_D0E1F2A3B4C5

#include <QObject>

#include "model/dto/analysisdataasset.h"
#include "model/dto/analysisresult.h"

// ════════════════════════════════════════════════════════
//  AttributeQueryService
//  属性查询能力占位服务
//  首期只提供统一的“即将支持”结果包装，
//  供工作区工具面板显示未来能力边界。
// ════════════════════════════════════════════════════════
class AttributeQueryService : public QObject
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数
     * @param_1 _pParent: 父对象指针
     */
    explicit AttributeQueryService(QObject* _pParent = nullptr);

    /*
     * @brief 析构函数
     */
    ~AttributeQueryService() override = default;

    /*
     * @brief 生成属性查询占位结果
     * @param_1 _assetInput: 当前分析资产
     */
    AnalysisResult buildPlaceholderResult(const AnalysisDataAsset& _assetInput) const;
};

#endif // ATTRIBUTEQUERYSERVICE_H_D0E1F2A3B4C5
