// src/service/analysisservice.h
#ifndef ANALYSISSERVICE_H_C3D4E5F6A1B2
#define ANALYSISSERVICE_H_C3D4E5F6A1B2

#include <QObject>
#include <QString>

#include "layerinfo.h"
#include "analysisresult.h"
#include "numericdataset.h"

class AnalysisService : public QObject
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数
     * @param_1 _pParent: 父对象指针，用于 Qt 对象树内存管理
     */
    explicit AnalysisService(QObject* _pParent = nullptr);

    /*
     * @brief 析构函数
     */
    ~AnalysisService() override;

    /*
     * @brief 执行基础统计分析
     *        输出最大值、最小值、均值
     */
    void runBasicStatistics();

    /*
     * @brief 执行频率统计
     *        连续值数据按区间分箱，离散值数据按精确值统计
     * @param_1 _nBinCount: 分箱数，最小为 2
     */
    void runFrequencyStatistics(int _nBinCount);

    /*
     * @brief 执行邻域分析
     *        计算每个像元邻域均值，并输出总体统计
     * @param_1 _nWindowSize: 邻域窗口边长，必须为不小于 3 的奇数
     */
    void runNeighborhoodAnalysis(int _nWindowSize);

    /*
     * @brief 兼容旧的 AI 工具接口，内部映射为基础统计
     */
    void runBufferAnalysis(double _dRadiusMeters);

    /*
     * @brief 兼容旧的 AI 工具接口，内部映射为频率统计
     */
    void runOverlayAnalysis(const QString& _strOverlayType);

    /*
     * @brief 兼容旧的 AI 工具接口，内部映射为邻域分析
     */
    void runSpatialQuery(const QString& _strExpression);

    /*
     * @brief 兼容旧的 AI 工具接口，内部映射为邻域分析
     */
    void runRasterCalc(const QString& _strExpression);

signals:
    void analysisProgress(int _nPercent);
    void analysisFinished(const AnalysisResult& _result);
    void analysisFailed(const AnalysisResult& _result);

public slots:
    /*
     * @brief 接收图层加载成功信号，记录当前数据路径
     */
    void onDataReady(const LayerInfo& _layerInfo);

    /*
     * @brief 接收数值数据加载成功信号，缓存可分析数据
     */
    void onNumericDataReady(const NumericDataset& _dataSet);

private:
    /*
     * @brief 返回指定行列位置的一维索引
     */
    int valueIndex(int _nRowIdx, int _nColIdx) const;

    /*
     * @brief 当前是否存在可分析的数值数据
     */
    bool hasNumericData() const;

    /*
     * @brief 构造失败结果并发出失败信号
     */
    void emitFailure(const QString& _strType, const QString& _strError);

    NumericDataset mDataSetReady;   // 当前可分析的数值型数据
    QString        mstrReadyDataPath; // 最近加载的数据路径
};

#endif // ANALYSISSERVICE_H_C3D4E5F6A1B2
