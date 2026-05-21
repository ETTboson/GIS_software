#ifndef ATTRIBUTEQUERYSERVICE_H_D0E1F2A3B4C5
#define ATTRIBUTEQUERYSERVICE_H_D0E1F2A3B4C5

#include <QObject>

#include "model/dto/analysisdataasset.h"
#include "model/dto/analysisresult.h"

// ════════════════════════════════════════════════════════
//  AttributeQueryService
//  属性查询与空间关系查询服务
//  基于表单式参数筛选矢量要素，并把命中结果写出为
//  可加载到地图的结果图层。
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
     * @brief 按属性条件筛选当前矢量资产要素
     * @param_1 _assetInput: 当前分析资产
     * @param_2 _strFieldName: 查询字段名
     * @param_3 _strOperatorId: 运算符标识
     * @param_4 _strValueText: 查询值文本
     */
    void runAttributeQuery(const AnalysisDataAsset& _assetInput,
        const QString& _strFieldName,
        const QString& _strOperatorId,
        const QString& _strValueText);

    /*
     * @brief 按空间关系筛选源矢量资产要素
     * @param_1 _assetSource: 被筛选的源矢量资产
     * @param_2 _assetTarget: 指定区域矢量资产
     * @param_3 _strRelationId: 空间关系标识
     */
    void runSpatialRelationQuery(const AnalysisDataAsset& _assetSource,
        const AnalysisDataAsset& _assetTarget,
        const QString& _strRelationId);

signals:
    void analysisProgress(int _nPercent);
    void analysisFinished(const AnalysisResult& _result);
    void analysisFailed(const AnalysisResult& _result);

private:
    /*
     * @brief 构造失败结果并发出失败信号
     * @param_1 _assetInput: 当前分析资产
     * @param_2 _strError: 错误文本
     * @param_3 _strToolId: 工具标识
     */
    void emitFailure(const AnalysisDataAsset& _assetInput,
        const QString& _strError,
        const QString& _strToolId);
};

#endif // ATTRIBUTEQUERYSERVICE_H_D0E1F2A3B4C5
