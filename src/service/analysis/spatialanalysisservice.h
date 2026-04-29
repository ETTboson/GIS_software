#ifndef SPATIALANALYSISSERVICE_H_E0F1A2B3C4D5
#define SPATIALANALYSISSERVICE_H_E0F1A2B3C4D5

#include <QObject>

#include "model/dto/analysisdataasset.h"
#include "model/dto/analysisresult.h"
#include "model/dto/visualizationdata.h"

// ════════════════════════════════════════════════════════
//  SpatialAnalysisService
//  空间分析能力服务
//  首期仅真正实现栅格邻域分析，
//  统一从栅格资产中的数值视图读取像元矩阵。
// ════════════════════════════════════════════════════════
class SpatialAnalysisService : public QObject
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数
     * @param_1 _pParent: 父对象指针
     */
    explicit SpatialAnalysisService(QObject* _pParent = nullptr);

    /*
     * @brief 析构函数
     */
    ~SpatialAnalysisService() override = default;

    /*
     * @brief 执行邻域分析
     * @param_1 _assetInput: 当前分析资产
     * @param_2 _nWindowSize: 邻域窗口大小
     */
    void runNeighborhoodAnalysis(const AnalysisDataAsset& _assetInput,
        int _nWindowSize);

signals:
    void analysisProgress(int _nPercent);
    void analysisFinished(const AnalysisResult& _result);
    void analysisFailed(const AnalysisResult& _result);

private:
    /*
     * @brief 返回指定行列位置的一维索引
     * @param_1 _nRowIdx: 行索引
     * @param_2 _nColIdx: 列索引
     * @param_3 _nColCount: 总列数
     */
    static int ValueIndex(int _nRowIdx, int _nColIdx, int _nColCount);

    /*
     * @brief 构造失败结果并发出失败信号
     * @param_1 _assetInput: 当前分析资产
     * @param_2 _strError: 错误文本
     */
    void emitFailure(const AnalysisDataAsset& _assetInput,
        const QString& _strError);

    /*
     * @brief 为邻域分析结果构造折线图数据
     * @param_1 _assetInput: 当前分析资产
     * @param_2 _vdNeighborhoodMeans: 按行优先展开的邻域均值列表
     */
    VisualizationData buildNeighborhoodVisualization(
        const AnalysisDataAsset& _assetInput,
        const QVector<double>& _vdNeighborhoodMeans) const;
};

#endif // SPATIALANALYSISSERVICE_H_E0F1A2B3C4D5
