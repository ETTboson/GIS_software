#ifndef DATAFORMATROUTER_H_C0D1E2F3A4B5
#define DATAFORMATROUTER_H_C0D1E2F3A4B5

#include <QString>

#include "model/dto/analysisdataasset.h"

// ════════════════════════════════════════════════════════
//  DataFormatRouter
//  职责：统一根据扩展名与内容特征生成分析资产，
//        同时提供地图图层入口的格式白名单判断与表格转点资产辅助转换。
//  位于 service/data/ 层。
// ════════════════════════════════════════════════════════
class DataFormatRouter
{
public:
    /*
     * @brief 判断文件是否可作为地图图层加载到中央画布
     * @param_1 _strFilePath: 输入文件绝对路径
     */
    static bool canLoadAsMapLayer(const QString& _strFilePath);

    /*
     * @brief 根据文件内容生成统一分析资产
     * @param_1 _strFilePath: 输入文件绝对路径
     * @param_2 _outAsset: 成功时写出的分析资产 DTO
     * @param_3 _strError: 失败时写出的错误信息
     */
    static bool routeAnalysisInput(const QString& _strFilePath,
        AnalysisDataAsset& _outAsset,
        QString& _strError);

    /*
     * @brief 依据目标类型构造备用资产
     * @param_1 _assetInput: 原始分析资产 DTO
     * @param_2 _eTargetType: 目标资产类型
     * @param_3 _outAsset: 成功时写出的目标资产 DTO
     * @param_4 _strError: 失败时写出的错误信息
     */
    static bool buildAlternateAsset(const AnalysisDataAsset& _assetInput,
        DataAssetType _eTargetType,
        AnalysisDataAsset& _outAsset,
        QString& _strError);
};

#endif // DATAFORMATROUTER_H_C0D1E2F3A4B5
