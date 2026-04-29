// src/service/data/dataservice.h
#ifndef DATASERVICE_H_B2C3D4E5F6A1
#define DATASERVICE_H_B2C3D4E5F6A1

#include <QObject>
#include <QString>

#include "model/dto/analysisdataasset.h"
#include "model/dto/layerinfo.h"
#include "model/enums/dataassettype.h"

// ════════════════════════════════════════════════════════
//  DataService
//  职责：统一处理地图图层入口与分析数据入口。
//        AddLayer 只负责地图图层链路，OpenData 只负责分析资产链路，
//        并向 MainWindow 发出统一分析资产 DTO。
//  位于 service/data/ 层，不持有任何 UI 对象。
// ════════════════════════════════════════════════════════
class DataService : public QObject
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数
     * @param_1 _pParent: 父对象指针，用于 Qt 对象树内存管理
     */
    explicit DataService(QObject* _pParent = nullptr);

    /*
     * @brief 析构函数
     */
    ~DataService() override;

    /*
     * @brief 把空间图层加载请求送入地图链路
     * @param_1 _strFilePath: 图层文件的本地绝对路径
     */
    void loadLayerToMap(const QString& _strFilePath);

    /*
     * @brief 把数据文件送入分析链路
     * @param_1 _strFilePath: 数据文件的本地绝对路径
     */
    void openDataForAnalysis(const QString& _strFilePath);

    /*
     * @brief 为需要二次确认的资产生成备用类型资产
     * @param_1 _assetInput: 原始分析资产
     * @param_2 _eTargetType: 目标资产类型
     * @param_3 _outAsset: 成功时写出的目标资产 DTO
     * @param_4 _strError: 失败时写出的错误信息
     */
    bool buildAlternateAsset(const AnalysisDataAsset& _assetInput,
        DataAssetType _eTargetType,
        AnalysisDataAsset& _outAsset,
        QString& _strError) const;

    /*
     * @brief 清空所有已加载图层记录
     */
    void clearAllLayers();

    /*
     * @brief 返回当前已加载的全部图层信息列表
     */
    QList<LayerInfo> getLayers() const;

    /*
     * @brief 按文件路径与名称移除一条图层记录
     * @param_1 _strFilePath: 图层源文件路径
     * @param_2 _strLayerName: 图层显示名称
     */
    void removeLayerRecord(const QString& _strFilePath,
        const QString& _strLayerName);

signals:
    /*
     * @brief 地图图层入口加载成功时触发
     * @param_1 _layerInfo: 加载成功的图层元信息 DTO
     */
    void layerLoaded(const LayerInfo& _layerInfo);

    /*
     * @brief 分析资产入口识别成功时触发
     * @param_1 _assetReady: 统一解析后的分析资产 DTO
     */
    void analysisAssetReady(const AnalysisDataAsset& _assetReady);

    /*
     * @brief 数据加载失败时触发
     * @param_1 _strErrorMsg: 失败原因描述
     */
    void dataLoadFailed(const QString& _strErrorMsg);

private:
    /*
     * @brief 解析 Shapefile 格式文件
     */
    bool parseShapefile(const QString& _strFilePath);

    /*
     * @brief 解析 GeoTIFF 格式文件
     */
    bool parseGeoTiff(const QString& _strFilePath);

    /*
     * @brief 解析 GeoJSON 格式文件
     */
    bool parseGeoJson(const QString& _strFilePath);

    /*
     * @brief 根据文件路径构造 LayerInfo DTO
     */
    LayerInfo buildLayerInfo(const QString& _strFilePath) const;

    QList<LayerInfo> mvLayers; // 当前地图图层列表
};

#endif // DATASERVICE_H_B2C3D4E5F6A1
