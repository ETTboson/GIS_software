// src/service/dataservice.h
#ifndef DATASERVICE_H_B2C3D4E5F6A1
#define DATASERVICE_H_B2C3D4E5F6A1

#include <QObject>
#include <QString>
#include <QStringList>

#include "layerinfo.h"
#include "numericdataset.h"

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
     * @brief 根据文件路径加载数据
     *        支持 GIS 文件与数值分析文件
     * @param_1 _strFilePath: 数据文件的本地绝对路径
     */
    void loadData(const QString& _strFilePath);

    /*
     * @brief 清空所有已加载图层记录
     */
    void clearAllLayers();

    /*
     * @brief 返回当前已加载的全部图层信息列表
     */
    QList<LayerInfo> getLayers() const;

signals:
    /*
     * @brief 数据加载成功时触发
     * @param_1 _layerInfo: 加载成功的图层元信息 DTO
     */
    void dataLoaded(const LayerInfo& _layerInfo);

    /*
     * @brief 数值数据加载成功时触发
     * @param_1 _dataSet: 可直接用于分析的数值型数据集 DTO
     */
    void numericDataLoaded(const NumericDataset& _dataSet);

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
     * @brief 解析 CSV 数值文件
     */
    bool parseCsv(const QString& _strFilePath, NumericDataset& _outDataSet,
        QString& _strError);

    /*
     * @brief 解析简单栅格文本文件
     */
    bool parseSimpleRaster(const QString& _strFilePath, NumericDataset& _outDataSet,
        QString& _strError);

    /*
     * @brief 根据文件路径构造 LayerInfo DTO
     */
    LayerInfo buildLayerInfo(const QString& _strFilePath) const;

    QList<LayerInfo> mvLayers;
};

#endif // DATASERVICE_H_B2C3D4E5F6A1
