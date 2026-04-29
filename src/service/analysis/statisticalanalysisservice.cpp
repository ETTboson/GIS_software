#include "statisticalanalysisservice.h"

#include <QStringList>
#include <QtMath>

StatisticalAnalysisService::StatisticalAnalysisService(QObject* _pParent)
    : QObject(_pParent)
{
}

void StatisticalAnalysisService::runBasicStatistics(
    const AnalysisDataAsset& _assetInput)
{
    if (!_assetInput.bHasNumericDataset
        || _assetInput.dataNumeric.vdValues.isEmpty()) {
        emitFailure("basic_statistics", _assetInput,
            tr("当前数据资产没有可用的数值视图，无法执行基础统计"));
        return;
    }

    emit analysisProgress(0);

    const QVector<double>& _vdValues = _assetInput.dataNumeric.vdValues;
    double _dMinValue = _vdValues[0];
    double _dMaxValue = _vdValues[0];
    double _dSumValue = 0.0;

    for (double _dValue : _vdValues) {
        _dMinValue = qMin(_dMinValue, _dValue);
        _dMaxValue = qMax(_dMaxValue, _dValue);
        _dSumValue += _dValue;
    }

    emit analysisProgress(100);

    const double _dMeanValue = _dSumValue / _vdValues.size();

    AnalysisResult _result;
    _result.strType = "basic_statistics";
    _result.strToolId = "basic_statistics";
    _result.strSourceAssetId = _assetInput.strAssetId;
    _result.strSourceAssetName = _assetInput.strName;
    _result.bSuccess = true;
    _result.bHasVisualization = true;
    _result.dataVisualization = buildBasicStatisticsVisualization(
        _assetInput, _dMinValue, _dMaxValue, _dMeanValue);
    _result.strDesc = tr(
        "基础统计分析完成\n"
        "数据资产：%1\n"
        "类型：%2\n"
        "数值矩阵：%3 行 × %4 列\n"
        "最小值：%5\n"
        "最大值：%6\n"
        "均值：%7")
        .arg(_assetInput.strName)
        .arg(_assetInput.strSourceFormat)
        .arg(_assetInput.dataNumeric.nRows)
        .arg(_assetInput.dataNumeric.nCols)
        .arg(_dMinValue, 0, 'f', 6)
        .arg(_dMaxValue, 0, 'f', 6)
        .arg(_dMeanValue, 0, 'f', 6);
    emit analysisFinished(_result);
}

void StatisticalAnalysisService::runFrequencyStatistics(
    const AnalysisDataAsset& _assetInput,
    int _nBinCount)
{
    if (!_assetInput.bHasNumericDataset
        || _assetInput.dataNumeric.vdValues.isEmpty()) {
        emitFailure("frequency_statistics", _assetInput,
            tr("当前数据资产没有可用的数值视图，无法执行频率统计"));
        return;
    }

    if (_nBinCount < 2) {
        emitFailure("frequency_statistics", _assetInput,
            tr("分箱数必须大于等于 2"));
        return;
    }

    emit analysisProgress(0);

    const QVector<double>& _vdValues = _assetInput.dataNumeric.vdValues;
    double _dMinValue = _vdValues[0];
    double _dMaxValue = _vdValues[0];
    QMap<long long, int> _mapDiscreteCounts;
    bool _bDiscrete = true;

    for (double _dValue : _vdValues) {
        _dMinValue = qMin(_dMinValue, _dValue);
        _dMaxValue = qMax(_dMaxValue, _dValue);

        const double _dRounded = qRound64(_dValue);
        if (!qFuzzyCompare(_dValue + 1.0, _dRounded + 1.0)) {
            _bDiscrete = false;
        }
    }

    QStringList _vLines;
    VisualizationData _dataVisualization;
    if (_bDiscrete) {
        for (double _dValue : _vdValues) {
            const long long _lKey = qRound64(_dValue);
            _mapDiscreteCounts[_lKey] += 1;
        }

        _vLines << tr("频率统计完成（离散值模式）");
        _vLines << tr("数据资产：%1").arg(_assetInput.strName);
        for (auto _it = _mapDiscreteCounts.constBegin();
            _it != _mapDiscreteCounts.constEnd(); ++_it) {
            _vLines << tr("值 %1 : %2 次").arg(_it.key()).arg(_it.value());
        }
        _dataVisualization = buildDiscreteFrequencyVisualization(
            _assetInput, _mapDiscreteCounts);
    } else {
        QVector<int> _vnBins(_nBinCount, 0);
        const double _dRange = _dMaxValue - _dMinValue;
        const double _dBinWidth = (_dRange <= 0.0) ? 1.0 : (_dRange / _nBinCount);

        for (double _dValue : _vdValues) {
            int _nBinIdx = (_dRange <= 0.0)
                ? 0
                : static_cast<int>((_dValue - _dMinValue) / _dBinWidth);
            if (_nBinIdx >= _nBinCount) {
                _nBinIdx = _nBinCount - 1;
            }
            if (_nBinIdx < 0) {
                _nBinIdx = 0;
            }
            _vnBins[_nBinIdx] += 1;
        }

        _vLines << tr("频率统计完成（分箱模式）");
        _vLines << tr("数据资产：%1").arg(_assetInput.strName);
        _vLines << tr("范围：[%1, %2]")
            .arg(_dMinValue, 0, 'f', 6)
            .arg(_dMaxValue, 0, 'f', 6);
        _vLines << tr("分箱数：%1").arg(_nBinCount);
        for (int _nBinIdx = 0; _nBinIdx < _nBinCount; ++_nBinIdx) {
            const double _dLeft = _dMinValue + _nBinIdx * _dBinWidth;
            const double _dRight = (_nBinIdx == _nBinCount - 1)
                ? _dMaxValue
                : (_dLeft + _dBinWidth);
            _vLines << tr("[%1, %2] : %3 次")
                .arg(_dLeft, 0, 'f', 6)
                .arg(_dRight, 0, 'f', 6)
                .arg(_vnBins[_nBinIdx]);
        }
        _dataVisualization = buildContinuousFrequencyVisualization(
            _assetInput, _vnBins, _dMinValue, _dMaxValue, _dBinWidth);
    }

    emit analysisProgress(100);

    AnalysisResult _result;
    _result.strType = "frequency_statistics";
    _result.strToolId = "frequency_statistics";
    _result.strSourceAssetId = _assetInput.strAssetId;
    _result.strSourceAssetName = _assetInput.strName;
    _result.bSuccess = true;
    _result.bHasVisualization = true;
    _result.dataVisualization = _dataVisualization;
    _result.strDesc = _vLines.join("\n");
    emit analysisFinished(_result);
}

void StatisticalAnalysisService::emitFailure(const QString& _strToolId,
    const AnalysisDataAsset& _assetInput,
    const QString& _strError)
{
    AnalysisResult _result;
    _result.strType = _strToolId;
    _result.strToolId = _strToolId;
    _result.strSourceAssetId = _assetInput.strAssetId;
    _result.strSourceAssetName = _assetInput.strName;
    _result.strDesc = _strError;
    _result.bSuccess = false;
    _result.bHasVisualization = false;
    emit analysisFailed(_result);
}

VisualizationData StatisticalAnalysisService::buildBasicStatisticsVisualization(
    const AnalysisDataAsset& _assetInput,
    double _dMinValue,
    double _dMaxValue,
    double _dMeanValue) const
{
    VisualizationSeries _series;
    _series.strName = tr("基础统计");
    _series.strColorHex = "#3E7CB1";

    VisualizationPoint _pointMin;
    _pointMin.strLabel = tr("最小值");
    _pointMin.dValue = _dMinValue;
    _pointMin.strToolTip = tr("数据资产：%1\n最小值：%2")
        .arg(_assetInput.strName)
        .arg(_dMinValue, 0, 'f', 6);
    _pointMin.nSourceIndex = 0;
    _series.vPoints.append(_pointMin);

    VisualizationPoint _pointMax;
    _pointMax.strLabel = tr("最大值");
    _pointMax.dValue = _dMaxValue;
    _pointMax.strToolTip = tr("数据资产：%1\n最大值：%2")
        .arg(_assetInput.strName)
        .arg(_dMaxValue, 0, 'f', 6);
    _pointMax.nSourceIndex = 1;
    _series.vPoints.append(_pointMax);

    VisualizationPoint _pointMean;
    _pointMean.strLabel = tr("均值");
    _pointMean.dValue = _dMeanValue;
    _pointMean.strToolTip = tr("数据资产：%1\n均值：%2")
        .arg(_assetInput.strName)
        .arg(_dMeanValue, 0, 'f', 6);
    _pointMean.nSourceIndex = 2;
    _series.vPoints.append(_pointMean);

    VisualizationData _dataVisualization;
    _dataVisualization.strTitle = tr("基础统计结果");
    _dataVisualization.strXAxisTitle = tr("统计指标");
    _dataVisualization.strYAxisTitle = tr("数值");
    _dataVisualization.chartTypeSuggested = VisualizationChartType::BarChart;
    _dataVisualization.strSourceAnalysisType = "basic_statistics";
    _dataVisualization.vSeries.append(_series);
    return _dataVisualization;
}

VisualizationData StatisticalAnalysisService::buildDiscreteFrequencyVisualization(
    const AnalysisDataAsset& _assetInput,
    const QMap<long long, int>& _mapDiscreteCounts) const
{
    VisualizationSeries _series;
    _series.strName = tr("频率统计");
    _series.strColorHex = "#2A9D8F";

    for (auto _it = _mapDiscreteCounts.constBegin();
        _it != _mapDiscreteCounts.constEnd(); ++_it) {
        VisualizationPoint _point;
        _point.strLabel = QString::number(_it.key());
        _point.dValue = _it.value();
        _point.strToolTip = tr("数据资产：%1\n值：%2\n频次：%3")
            .arg(_assetInput.strName)
            .arg(_it.key())
            .arg(_it.value());
        _point.nSourceIndex = static_cast<int>(_it.key());
        _series.vPoints.append(_point);
    }

    VisualizationData _dataVisualization;
    _dataVisualization.strTitle = tr("频率统计结果");
    _dataVisualization.strXAxisTitle = tr("数据值");
    _dataVisualization.strYAxisTitle = tr("出现次数");
    _dataVisualization.chartTypeSuggested = VisualizationChartType::BarChart;
    _dataVisualization.strSourceAnalysisType = "frequency_statistics";
    _dataVisualization.vSeries.append(_series);
    return _dataVisualization;
}

VisualizationData StatisticalAnalysisService::buildContinuousFrequencyVisualization(
    const AnalysisDataAsset& _assetInput,
    const QVector<int>& _vnBins,
    double _dMinValue,
    double _dMaxValue,
    double _dBinWidth) const
{
    VisualizationSeries _series;
    _series.strName = tr("频率统计");
    _series.strColorHex = "#2A9D8F";

    for (int _nBinIdx = 0; _nBinIdx < _vnBins.size(); ++_nBinIdx) {
        const double _dLeft = _dMinValue + _nBinIdx * _dBinWidth;
        const double _dRight = (_nBinIdx == _vnBins.size() - 1)
            ? _dMaxValue
            : (_dLeft + _dBinWidth);

        VisualizationPoint _point;
        _point.strLabel = tr("[%1, %2]")
            .arg(_dLeft, 0, 'f', 2)
            .arg(_dRight, 0, 'f', 2);
        _point.dValue = _vnBins[_nBinIdx];
        _point.strToolTip = tr("数据资产：%1\n区间：[%2, %3]\n频次：%4")
            .arg(_assetInput.strName)
            .arg(_dLeft, 0, 'f', 6)
            .arg(_dRight, 0, 'f', 6)
            .arg(_vnBins[_nBinIdx]);
        _point.nSourceIndex = _nBinIdx;
        _series.vPoints.append(_point);
    }

    VisualizationData _dataVisualization;
    _dataVisualization.strTitle = tr("频率统计结果");
    _dataVisualization.strXAxisTitle = tr("分箱区间");
    _dataVisualization.strYAxisTitle = tr("出现次数");
    _dataVisualization.chartTypeSuggested = VisualizationChartType::BarChart;
    _dataVisualization.strSourceAnalysisType = "frequency_statistics";
    _dataVisualization.vSeries.append(_series);
    return _dataVisualization;
}
