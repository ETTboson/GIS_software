#include "statisticalanalysisservice.h"

#include <limits>

#include <QFileInfo>
#include <QStringList>
#include <QtMath>

#include <qgsfeature.h>
#include <qgsfeatureiterator.h>
#include <qgsfeaturerequest.h>
#include <qgsfield.h>
#include <qgsfields.h>
#include <qgsvectorlayer.h>

namespace
{

QString vectorSourceUriForAsset(const AnalysisDataAsset& _assetInput)
{
    const QString _strVectorSourceUri =
        _assetInput.dataVector.strSourceUri.trimmed();
    return _strVectorSourceUri.isEmpty()
        ? _assetInput.strSourcePath
        : _strVectorSourceUri;
}

QString vectorProviderKeyForAsset(const AnalysisDataAsset& _assetInput)
{
    const QString _strProviderKey =
        _assetInput.dataVector.strProviderKey.trimmed();
    return _strProviderKey.isEmpty()
        ? QStringLiteral("ogr")
        : _strProviderKey;
}

} // namespace

StatisticalAnalysisService::StatisticalAnalysisService(QObject* _pParent)
    : QObject(_pParent)
{
}

void StatisticalAnalysisService::runBasicStatistics(
    const AnalysisDataAsset& _assetInput)
{
    emit analysisProgress(0);

    NumericDataset _dataNumeric;
    QString _strError;
    if (!resolveNumericDataset(_assetInput, _dataNumeric, _strError)) {
        emitFailure("basic_statistics", _assetInput,
            _strError);
        return;
    }

    const QVector<double>& _vdValues = _dataNumeric.vdValues;
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
        .arg(_dataNumeric.nRows)
        .arg(_dataNumeric.nCols)
        .arg(_dMinValue, 0, 'f', 6)
        .arg(_dMaxValue, 0, 'f', 6)
        .arg(_dMeanValue, 0, 'f', 6);
    emit analysisFinished(_result);
}

void StatisticalAnalysisService::runFrequencyStatistics(
    const AnalysisDataAsset& _assetInput,
    int _nBinCount)
{
    emit analysisProgress(0);

    NumericDataset _dataNumeric;
    QString _strError;
    if (!resolveNumericDataset(_assetInput, _dataNumeric, _strError)) {
        emitFailure("frequency_statistics", _assetInput,
            _strError);
        return;
    }

    if (_nBinCount < 2) {
        emitFailure("frequency_statistics", _assetInput,
            tr("分箱数必须大于等于 2"));
        return;
    }

    const QVector<double>& _vdValues = _dataNumeric.vdValues;
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

bool StatisticalAnalysisService::resolveNumericDataset(
    const AnalysisDataAsset& _assetInput,
    NumericDataset& _outDataSet,
    QString& _strError)
{
    if (_assetInput.bHasNumericDataset
        && !_assetInput.dataNumeric.vdValues.isEmpty()) {
        _outDataSet = _assetInput.dataNumeric;
        return true;
    }

    if (_assetInput.eAssetType != DataAssetType::Vector
        || _assetInput.dataVector.vNumericFieldNames.isEmpty()) {
        _strError = tr("当前数据资产没有可用的数值视图，无法执行统计分析");
        return false;
    }

    const QString _strSourceUri = vectorSourceUriForAsset(_assetInput);
    if (_strSourceUri.trimmed().isEmpty()) {
        _strError = tr("矢量资产缺少可读取的数据源，无法执行统计分析");
        return false;
    }

    QgsVectorLayer _layerVector(
        _strSourceUri,
        _assetInput.strName,
        vectorProviderKeyForAsset(_assetInput));
    if (!_layerVector.isValid()) {
        _strError = tr("无法读取矢量数据：%1").arg(_strSourceUri);
        return false;
    }

    const QgsFields _fields = _layerVector.fields();
    QList<int> _vnNumericFieldIndices;
    for (const QString& _strFieldName : _assetInput.dataVector.vNumericFieldNames) {
        const int _nFieldIdx = _fields.indexFromName(_strFieldName);
        if (_nFieldIdx >= 0) {
            _vnNumericFieldIndices.append(_nFieldIdx);
        }
    }

    if (_vnNumericFieldIndices.isEmpty()) {
        _strError = tr("矢量资产没有可读取的数值字段，无法执行统计分析");
        return false;
    }

    const qint64 _lFeatureCount = _layerVector.featureCount();
    const qint64 _lValueCount =
        _lFeatureCount * static_cast<qint64>(_vnNumericFieldIndices.size());
    if (_lValueCount > std::numeric_limits<int>::max()) {
        _strError = tr("矢量数值字段过大，当前统计视图无法一次性载入");
        return false;
    }

    _outDataSet = NumericDataset();
    _outDataSet.strSourcePath = _strSourceUri;
    _outDataSet.strName = _assetInput.strName.trimmed().isEmpty()
        ? QFileInfo(_strSourceUri).fileName()
        : _assetInput.strName;
    _outDataSet.strFormat = _assetInput.strSourceFormat;
    _outDataSet.nCols = _vnNumericFieldIndices.size();
    if (_lValueCount > 0) {
        _outDataSet.vdValues.reserve(static_cast<int>(_lValueCount));
    }

    QgsFeatureRequest _request;
    _request.setFlags(Qgis::FeatureRequestFlag::NoGeometry);
    _request.setSubsetOfAttributes(_vnNumericFieldIndices);

    emit analysisProgress(5);

    int _nFeatureCount = 0;
    QgsFeature _featureCurrent;
    QgsFeatureIterator _itFeature = _layerVector.getFeatures(_request);
    while (_itFeature.nextFeature(_featureCurrent)) {
        const QgsAttributes _attributesCurrent = _featureCurrent.attributes();
        int _nSubsetAttrIdx = 0;
        for (int _nFieldIdx : _vnNumericFieldIndices) {
            const QVariant _valueCurrent =
                (_nFieldIdx < _attributesCurrent.size())
                ? _attributesCurrent.at(_nFieldIdx)
                : (_nSubsetAttrIdx < _attributesCurrent.size()
                    ? _attributesCurrent.at(_nSubsetAttrIdx)
                    : QVariant());
            bool _bOk = false;
            const double _dValue = _valueCurrent.toDouble(&_bOk);
            _outDataSet.vdValues.append(_bOk ? _dValue : 0.0);
            ++_nSubsetAttrIdx;
        }
        ++_nFeatureCount;
    }

    _outDataSet.nRows = _nFeatureCount;
    if (_outDataSet.vdValues.isEmpty()) {
        _strError = tr("矢量资产没有可用于统计的数值记录");
        return false;
    }

    return true;
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
