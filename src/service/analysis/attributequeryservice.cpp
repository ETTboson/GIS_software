#include "attributequeryservice.h"

AttributeQueryService::AttributeQueryService(QObject* _pParent)
    : QObject(_pParent)
{
}

AnalysisResult AttributeQueryService::buildPlaceholderResult(
    const AnalysisDataAsset& _assetInput) const
{
    AnalysisResult _result;
    _result.strType = "attribute_query";
    _result.strToolId = "attribute_query";
    _result.strSourceAssetId = _assetInput.strAssetId;
    _result.strSourceAssetName = _assetInput.strName;
    _result.bSuccess = false;
    _result.strDesc = tr(
        "属性查询能力即将支持。\n"
        "当前资产：%1\n"
        "当前版本已完成统一工作区、能力判断与工具入口，"
        "后续可在此处接入表达式查询与筛选结果展示。")
        .arg(_assetInput.strName);
    return _result;
}
