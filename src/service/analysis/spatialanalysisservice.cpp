#include "spatialanalysisservice.h"

#include <QtMath>

SpatialAnalysisService::SpatialAnalysisService(QObject* _pParent)
    : QObject(_pParent)
{
}

void SpatialAnalysisService::runNeighborhoodAnalysis(
    const AnalysisDataAsset& _assetInput,
    int _nWindowSize)
{
    if (!_assetInput.bHasNumericDataset
        || _assetInput.dataNumeric.vdValues.isEmpty()) {
        emitFailure(_assetInput, tr("当前栅格资产没有可用的数值视图，无法执行邻域分析"));
        return;
    }

    if (_nWindowSize < 3 || (_nWindowSize % 2) == 0) {
        emitFailure(_assetInput, tr("邻域窗口必须是不小于 3 的奇数"));
        return;
    }

    emit analysisProgress(0);

    const NumericDataset& _dataNumeric = _assetInput.dataNumeric;
    const int _nRadius = _nWindowSize / 2;
    QVector<double> _vdNeighborhoodMeans;
    _vdNeighborhoodMeans.reserve(_dataNumeric.nRows * _dataNumeric.nCols);

    double _dMinMean = 0.0;
    double _dMaxMean = 0.0;
    bool _bFirstValue = true;

    for (int _nRowIdx = 0; _nRowIdx < _dataNumeric.nRows; ++_nRowIdx) {
        for (int _nColIdx = 0; _nColIdx < _dataNumeric.nCols; ++_nColIdx) {
            double _dSumValue = 0.0;
            int _nCount = 0;

            for (int _nNbrRowIdx = qMax(0, _nRowIdx - _nRadius);
                _nNbrRowIdx <= qMin(_dataNumeric.nRows - 1, _nRowIdx + _nRadius);
                ++_nNbrRowIdx) {
                for (int _nNbrColIdx = qMax(0, _nColIdx - _nRadius);
                    _nNbrColIdx <= qMin(_dataNumeric.nCols - 1, _nColIdx + _nRadius);
                    ++_nNbrColIdx) {
                    _dSumValue += _dataNumeric.vdValues[
                        ValueIndex(_nNbrRowIdx, _nNbrColIdx, _dataNumeric.nCols)];
                    ++_nCount;
                }
            }

            const double _dMeanValue = _dSumValue / _nCount;
            _vdNeighborhoodMeans.append(_dMeanValue);

            if (_bFirstValue) {
                _dMinMean = _dMeanValue;
                _dMaxMean = _dMeanValue;
                _bFirstValue = false;
            } else {
                _dMinMean = qMin(_dMinMean, _dMeanValue);
                _dMaxMean = qMax(_dMaxMean, _dMeanValue);
            }
        }
    }

    double _dOverallMean = 0.0;
    for (double _dMeanValue : _vdNeighborhoodMeans) {
        _dOverallMean += _dMeanValue;
    }
    _dOverallMean /= _vdNeighborhoodMeans.size();

    const int _nCenterRow = _dataNumeric.nRows / 2;
    const int _nCenterCol = _dataNumeric.nCols / 2;
    const double _dCenterOriginalValue =
        _dataNumeric.vdValues[ValueIndex(_nCenterRow, _nCenterCol, _dataNumeric.nCols)];
    const double _dCenterNeighborhoodMean =
        _vdNeighborhoodMeans[ValueIndex(_nCenterRow, _nCenterCol, _dataNumeric.nCols)];

    emit analysisProgress(100);

    AnalysisResult _result;
    _result.strType = "neighborhood_analysis";
    _result.strToolId = "neighborhood_analysis";
    _result.strSourceAssetId = _assetInput.strAssetId;
    _result.strSourceAssetName = _assetInput.strName;
    _result.bSuccess = true;
    _result.bHasVisualization = true;
    _result.dataVisualization = buildNeighborhoodVisualization(
        _assetInput, _vdNeighborhoodMeans);
    _result.strDesc = tr(
        "邻域分析完成\n"
        "数据资产：%1\n"
        "窗口大小：%2 × %2\n"
        "邻域均值最小值：%3\n"
        "邻域均值最大值：%4\n"
        "邻域均值总体均值：%5\n"
        "中心像元原值：%6\n"
        "中心像元邻域均值：%7")
        .arg(_assetInput.strName)
        .arg(_nWindowSize)
        .arg(_dMinMean, 0, 'f', 6)
        .arg(_dMaxMean, 0, 'f', 6)
        .arg(_dOverallMean, 0, 'f', 6)
        .arg(_dCenterOriginalValue, 0, 'f', 6)
        .arg(_dCenterNeighborhoodMean, 0, 'f', 6);
    emit analysisFinished(_result);
}

int SpatialAnalysisService::ValueIndex(int _nRowIdx, int _nColIdx, int _nColCount)
{
    return _nRowIdx * _nColCount + _nColIdx;
}

void SpatialAnalysisService::emitFailure(const AnalysisDataAsset& _assetInput,
    const QString& _strError)
{
    AnalysisResult _result;
    _result.strType = "neighborhood_analysis";
    _result.strToolId = "neighborhood_analysis";
    _result.strSourceAssetId = _assetInput.strAssetId;
    _result.strSourceAssetName = _assetInput.strName;
    _result.strDesc = _strError;
    _result.bSuccess = false;
    _result.bHasVisualization = false;
    emit analysisFailed(_result);
}

VisualizationData SpatialAnalysisService::buildNeighborhoodVisualization(
    const AnalysisDataAsset& _assetInput,
    const QVector<double>& _vdNeighborhoodMeans) const
{
    VisualizationSeries _series;
    _series.strName = tr("行趋势");
    _series.strColorHex = "#E76F51";

    for (int _nRowIdx = 0; _nRowIdx < _assetInput.dataNumeric.nRows; ++_nRowIdx) {
        double _dRowSum = 0.0;
        for (int _nColIdx = 0; _nColIdx < _assetInput.dataNumeric.nCols; ++_nColIdx) {
            _dRowSum += _vdNeighborhoodMeans[
                ValueIndex(_nRowIdx, _nColIdx, _assetInput.dataNumeric.nCols)];
        }

        const double _dRowMean = _dRowSum / _assetInput.dataNumeric.nCols;
        VisualizationPoint _point;
        _point.strLabel = tr("第%1行").arg(_nRowIdx + 1);
        _point.dValue = _dRowMean;
        _point.strToolTip = tr("数据资产：%1\n行号：%2\n行邻域均值：%3")
            .arg(_assetInput.strName)
            .arg(_nRowIdx + 1)
            .arg(_dRowMean, 0, 'f', 6);
        _point.nRowIndex = _nRowIdx;
        _point.nSourceIndex = _nRowIdx;
        _series.vPoints.append(_point);
    }

    VisualizationData _dataVisualization;
    _dataVisualization.strTitle = tr("邻域分析行趋势");
    _dataVisualization.strXAxisTitle = tr("行号");
    _dataVisualization.strYAxisTitle = tr("邻域均值");
    _dataVisualization.chartTypeSuggested = VisualizationChartType::LineChart;
    _dataVisualization.strSourceAnalysisType = "neighborhood_analysis";
    _dataVisualization.vSeries.append(_series);
    return _dataVisualization;
}
