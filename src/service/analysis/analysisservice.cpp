// src/service/analysisservice.cpp
#include "analysisservice.h"

#include <QMap>
#include <QMetaObject>
#include <QtMath>

AnalysisService::AnalysisService(QObject* _pParent)
    : QObject(_pParent)
{
    mDataSetReady.nRows = 0;
    mDataSetReady.nCols = 0;
}

AnalysisService::~AnalysisService()
{
}

void AnalysisService::onDataReady(const LayerInfo& _layerInfo)
{
    mstrReadyDataPath = _layerInfo.strFilePath;
}

void AnalysisService::onNumericDataReady(const NumericDataset& _dataSet)
{
    mDataSetReady = _dataSet;
    mstrReadyDataPath = _dataSet.strSourcePath;
}

void AnalysisService::runBasicStatistics()
{
    if (!hasNumericData()) {
        emitFailure("basic_statistics", tr("当前没有可分析的 CSV 或简单栅格数据"));
        return;
    }

    emit analysisProgress(0);

    const QVector<double>& _vdValues = mDataSetReady.vdValues;
    double _dMinValue = _vdValues[0];
    double _dMaxValue = _vdValues[0];
    double _dSumValue = 0.0;

    for (double _dValue : _vdValues) {
        if (_dValue < _dMinValue) {
            _dMinValue = _dValue;
        }
        if (_dValue > _dMaxValue) {
            _dMaxValue = _dValue;
        }
        _dSumValue += _dValue;
    }

    emit analysisProgress(100);

    AnalysisResult _result;
    _result.strType = "basic_statistics";
    _result.bSuccess = true;
    _result.strDesc = tr(
        "基础统计分析完成\n"
        "数据集：%1\n"
        "格式：%2\n"
        "尺寸：%3 行 × %4 列\n"
        "最小值：%5\n"
        "最大值：%6\n"
        "均值：%7")
        .arg(mDataSetReady.strName)
        .arg(mDataSetReady.strFormat)
        .arg(mDataSetReady.nRows)
        .arg(mDataSetReady.nCols)
        .arg(_dMinValue, 0, 'f', 6)
        .arg(_dMaxValue, 0, 'f', 6)
        .arg(_dSumValue / _vdValues.size(), 0, 'f', 6);
    emit analysisFinished(_result);
}

void AnalysisService::runFrequencyStatistics(int _nBinCount)
{
    if (!hasNumericData()) {
        emitFailure("frequency_statistics", tr("当前没有可分析的 CSV 或简单栅格数据"));
        return;
    }

    if (_nBinCount < 2) {
        emitFailure("frequency_statistics", tr("分箱数必须大于等于 2"));
        return;
    }

    emit analysisProgress(0);

    const QVector<double>& _vdValues = mDataSetReady.vdValues;
    double _dMinValue = _vdValues[0];
    double _dMaxValue = _vdValues[0];
    QMap<QString, int> _mapDiscreteCounts;
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
    if (_bDiscrete) {
        for (double _dValue : _vdValues) {
            const QString _strKey = QString::number(static_cast<long long>(qRound64(_dValue)));
            _mapDiscreteCounts[_strKey] += 1;
        }

        _vLines << tr("频率统计完成（离散值模式）");
        _vLines << tr("数据集：%1").arg(mDataSetReady.strName);
        for (auto _it = _mapDiscreteCounts.constBegin(); _it != _mapDiscreteCounts.constEnd(); ++_it) {
            _vLines << tr("值 %1 : %2 次").arg(_it.key()).arg(_it.value());
        }
    } else {
        QVector<int> _vnBins(_nBinCount, 0);
        const double _dRange = _dMaxValue - _dMinValue;
        const double _dBinWidth = (_dRange <= 0.0) ? 1.0 : (_dRange / _nBinCount);

        for (double _dValue : _vdValues) {
            int _nBinIdx = (_dRange <= 0.0) ? 0
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
        _vLines << tr("数据集：%1").arg(mDataSetReady.strName);
        _vLines << tr("范围：[%1, %2]").arg(_dMinValue, 0, 'f', 6).arg(_dMaxValue, 0, 'f', 6);
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
    }

    emit analysisProgress(100);

    AnalysisResult _result;
    _result.strType = "frequency_statistics";
    _result.bSuccess = true;
    _result.strDesc = _vLines.join("\n");
    emit analysisFinished(_result);
}

void AnalysisService::runNeighborhoodAnalysis(int _nWindowSize)
{
    if (!hasNumericData()) {
        emitFailure("neighborhood_analysis", tr("当前没有可分析的 CSV 或简单栅格数据"));
        return;
    }

    if (_nWindowSize < 3 || (_nWindowSize % 2) == 0) {
        emitFailure("neighborhood_analysis", tr("邻域窗口必须是不小于 3 的奇数"));
        return;
    }

    emit analysisProgress(0);

    const int _nRadius = _nWindowSize / 2;
    QVector<double> _vdNeighborhoodMeans;
    _vdNeighborhoodMeans.reserve(mDataSetReady.nRows * mDataSetReady.nCols);

    double _dMinMean = 0.0;
    double _dMaxMean = 0.0;
    bool _bFirstValue = true;

    for (int _nRowIdx = 0; _nRowIdx < mDataSetReady.nRows; ++_nRowIdx) {
        for (int _nColIdx = 0; _nColIdx < mDataSetReady.nCols; ++_nColIdx) {
            double _dSumValue = 0.0;
            int    _nCount = 0;

            for (int _nNbrRowIdx = qMax(0, _nRowIdx - _nRadius);
                _nNbrRowIdx <= qMin(mDataSetReady.nRows - 1, _nRowIdx + _nRadius);
                ++_nNbrRowIdx) {
                for (int _nNbrColIdx = qMax(0, _nColIdx - _nRadius);
                    _nNbrColIdx <= qMin(mDataSetReady.nCols - 1, _nColIdx + _nRadius);
                    ++_nNbrColIdx) {
                    _dSumValue += mDataSetReady.vdValues[valueIndex(_nNbrRowIdx, _nNbrColIdx)];
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

    const int _nCenterRow = mDataSetReady.nRows / 2;
    const int _nCenterCol = mDataSetReady.nCols / 2;
    const double _dCenterOriginalValue =
        mDataSetReady.vdValues[valueIndex(_nCenterRow, _nCenterCol)];
    const double _dCenterNeighborhoodMean =
        _vdNeighborhoodMeans[valueIndex(_nCenterRow, _nCenterCol)];

    emit analysisProgress(100);

    AnalysisResult _result;
    _result.strType = "neighborhood_analysis";
    _result.bSuccess = true;
    _result.strDesc = tr(
        "邻域分析完成\n"
        "数据集：%1\n"
        "窗口大小：%2 × %2\n"
        "邻域均值最小值：%3\n"
        "邻域均值最大值：%4\n"
        "邻域均值总体均值：%5\n"
        "中心像元原值：%6\n"
        "中心像元邻域均值：%7")
        .arg(mDataSetReady.strName)
        .arg(_nWindowSize)
        .arg(_dMinMean, 0, 'f', 6)
        .arg(_dMaxMean, 0, 'f', 6)
        .arg(_dOverallMean, 0, 'f', 6)
        .arg(_dCenterOriginalValue, 0, 'f', 6)
        .arg(_dCenterNeighborhoodMean, 0, 'f', 6);
    emit analysisFinished(_result);
}

void AnalysisService::runBufferAnalysis(double _dRadiusMeters)
{
    Q_UNUSED(_dRadiusMeters)
    runBasicStatistics();
}

void AnalysisService::runOverlayAnalysis(const QString& _strOverlayType)
{
    Q_UNUSED(_strOverlayType)
    runFrequencyStatistics(10);
}

void AnalysisService::runSpatialQuery(const QString& _strExpression)
{
    Q_UNUSED(_strExpression)
    runNeighborhoodAnalysis(3);
}

void AnalysisService::runRasterCalc(const QString& _strExpression)
{
    Q_UNUSED(_strExpression)
    runNeighborhoodAnalysis(3);
}

AnalysisResult AnalysisService::executeToolCall(const QString& _strToolName,
    const QJsonObject& _jsonArgs)
{
    AnalysisResult _result;
    bool _bCaptured = false;

    const QMetaObject::Connection _okConn = connect(this,
        &AnalysisService::analysisFinished, this,
        [&](const AnalysisResult& _analysisResult) {
            if (!_bCaptured) {
                _result = _analysisResult;
                _bCaptured = true;
            }
        });
    const QMetaObject::Connection _failConn = connect(this,
        &AnalysisService::analysisFailed, this,
        [&](const AnalysisResult& _analysisResult) {
            if (!_bCaptured) {
                _result = _analysisResult;
                _bCaptured = true;
            }
        });

    if (_strToolName == "run_buffer_analysis") {
        runBufferAnalysis(_jsonArgs["radius_meters"].toDouble(0.0));
    } else if (_strToolName == "run_overlay_analysis") {
        runOverlayAnalysis(_jsonArgs["overlay_type"].toString());
    } else if (_strToolName == "run_spatial_query") {
        runSpatialQuery(_jsonArgs["query_expression"].toString());
    } else if (_strToolName == "run_raster_calc") {
        runRasterCalc(_jsonArgs["expression"].toString());
    } else {
        _result.strType = _strToolName;
        _result.bSuccess = false;
        _result.strDesc = tr("未知分析工具：%1").arg(_strToolName);
        _bCaptured = true;
    }

    disconnect(_okConn);
    disconnect(_failConn);

    if (!_bCaptured) {
        _result.strType = _strToolName;
        _result.bSuccess = false;
        _result.strDesc = tr("分析执行后未返回结果");
    }
    return _result;
}

bool AnalysisService::hasReadyNumericData() const
{
    return hasNumericData();
}

NumericDataset AnalysisService::currentDataSet() const
{
    return mDataSetReady;
}

QString AnalysisService::readyDataPath() const
{
    return mstrReadyDataPath;
}

int AnalysisService::valueIndex(int _nRowIdx, int _nColIdx) const
{
    return _nRowIdx * mDataSetReady.nCols + _nColIdx;
}

bool AnalysisService::hasNumericData() const
{
    return mDataSetReady.nRows > 0
        && mDataSetReady.nCols > 0
        && !mDataSetReady.vdValues.isEmpty();
}

void AnalysisService::emitFailure(const QString& _strType, const QString& _strError)
{
    AnalysisResult _result;
    _result.strType = _strType;
    _result.strDesc = _strError;
    _result.bSuccess = false;
    emit analysisFailed(_result);
}
