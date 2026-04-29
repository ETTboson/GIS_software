#ifndef STATISTICALANALYSISSERVICE_H_F1A2B3C4D5E6
#define STATISTICALANALYSISSERVICE_H_F1A2B3C4D5E6

#include <QObject>
#include <QMap>

#include "model/dto/analysisdataasset.h"
#include "model/dto/analysisresult.h"
#include "model/dto/visualizationdata.h"

// ════════════════════════════════════════════════════════
//  StatisticalAnalysisService
//  统计分析能力服务
//  统一接收分析资产中的数值视图，提供基础统计与频率统计，
//  不关心数据来自表格、矢量属性还是栅格像元。
// ════════════════════════════════════════════════════════
class StatisticalAnalysisService : public QObject
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数
     * @param_1 _pParent: 父对象指针
     */
    explicit StatisticalAnalysisService(QObject* _pParent = nullptr);

    /*
     * @brief 析构函数
     */
    ~StatisticalAnalysisService() override = default;

    /*
     * @brief 执行基础统计分析
     * @param_1 _assetInput: 当前分析资产
     */
    void runBasicStatistics(const AnalysisDataAsset& _assetInput);

    /*
     * @brief 执行频率统计分析
     * @param_1 _assetInput: 当前分析资产
     * @param_2 _nBinCount: 分箱数
     */
    void runFrequencyStatistics(const AnalysisDataAsset& _assetInput,
        int _nBinCount);

signals:
    void analysisProgress(int _nPercent);
    void analysisFinished(const AnalysisResult& _result);
    void analysisFailed(const AnalysisResult& _result);

private:
    /*
     * @brief 构造失败结果并发出失败信号
     * @param_1 _strToolId: 当前工具标识
     * @param_2 _assetInput: 当前分析资产
     * @param_3 _strError: 错误文本
     */
    void emitFailure(const QString& _strToolId,
        const AnalysisDataAsset& _assetInput,
        const QString& _strError);

    /*
     * @brief 为基础统计结果构造柱状图数据
     * @param_1 _assetInput: 当前分析资产
     * @param_2 _dMinValue: 最小值
     * @param_3 _dMaxValue: 最大值
     * @param_4 _dMeanValue: 均值
     */
    VisualizationData buildBasicStatisticsVisualization(
        const AnalysisDataAsset& _assetInput,
        double _dMinValue,
        double _dMaxValue,
        double _dMeanValue) const;

    /*
     * @brief 为离散频率统计结果构造柱状图数据
     * @param_1 _assetInput: 当前分析资产
     * @param_2 _mapDiscreteCounts: 离散值频次映射
     */
    VisualizationData buildDiscreteFrequencyVisualization(
        const AnalysisDataAsset& _assetInput,
        const QMap<long long, int>& _mapDiscreteCounts) const;

    /*
     * @brief 为连续分箱频率统计结果构造柱状图数据
     * @param_1 _assetInput: 当前分析资产
     * @param_2 _vnBins: 分箱计数列表
     * @param_3 _dMinValue: 最小值
     * @param_4 _dMaxValue: 最大值
     * @param_5 _dBinWidth: 单箱宽度
     */
    VisualizationData buildContinuousFrequencyVisualization(
        const AnalysisDataAsset& _assetInput,
        const QVector<int>& _vnBins,
        double _dMinValue,
        double _dMaxValue,
        double _dBinWidth) const;
};

#endif // STATISTICALANALYSISSERVICE_H_F1A2B3C4D5E6
