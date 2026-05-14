#include "spatialanalysisservice.h"

#include <limits>
#include <memory>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QtMath>

#include <qgsfeature.h>
#include <qgsfeatureiterator.h>
#include <qgsfeaturesink.h>
#include <qgsgeometry.h>
#include <qgsproject.h>
#include <qgsvectorfilewriter.h>
#include <qgsvectorlayer.h>

namespace
{

QString sanitizeFilePart(const QString& _strInput)
{
    QString _strSafe = _strInput.trimmed();
    if (_strSafe.isEmpty()) {
        _strSafe = QStringLiteral("buffer_result");
    }
    _strSafe.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_\\-]+")),
        QStringLiteral("_"));
    return _strSafe;
}

QString formatDistanceForPath(double _dDistance)
{
    QString _strDistance = QString::number(_dDistance, 'f', 3);
    while (_strDistance.contains('.') && _strDistance.endsWith('0')) {
        _strDistance.chop(1);
    }
    if (_strDistance.endsWith('.')) {
        _strDistance.chop(1);
    }
    _strDistance.replace('-', 'm');
    _strDistance.replace('.', 'p');
    return _strDistance;
}

QString writableOutputDirectory(const QString& _strSourcePath)
{
    const QFileInfo _fiSource(_strSourcePath);
    const QString _strSourceOutDir =
        QDir(_fiSource.absolutePath()).absoluteFilePath(QStringLiteral("analysis_outputs"));

    QDir _dirSourceOut(_strSourceOutDir);
    if (_dirSourceOut.mkpath(QStringLiteral("."))
        && QFileInfo(_strSourceOutDir).isWritable()) {
        return _strSourceOutDir;
    }

    QString _strDocumentsDir =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (_strDocumentsDir.trimmed().isEmpty()) {
        _strDocumentsDir = QDir::homePath();
    }

    const QString _strFallbackOutDir = QDir(_strDocumentsDir).absoluteFilePath(
        QStringLiteral("GeoAI/analysis_outputs"));
    QDir _dirFallbackOut(_strFallbackOutDir);
    _dirFallbackOut.mkpath(QStringLiteral("."));
    return _strFallbackOutDir;
}

QString buildBufferOutputPath(const AnalysisDataAsset& _assetInput,
    double _dDistance)
{
    const QFileInfo _fiSource(_assetInput.strSourcePath);
    QString _strBaseName = _fiSource.completeBaseName();
    if (_strBaseName.trimmed().isEmpty()) {
        _strBaseName = _assetInput.strName;
    }

    const QString _strFileName = QStringLiteral("%1_buffer_%2.geojson")
        .arg(sanitizeFilePart(_strBaseName),
            sanitizeFilePart(formatDistanceForPath(_dDistance)));
    return QDir(writableOutputDirectory(_assetInput.strSourcePath))
        .absoluteFilePath(_strFileName);
}

} // namespace

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

void SpatialAnalysisService::runBufferAnalysis(
    const AnalysisDataAsset& _assetInput,
    double _dDistance,
    int _nSegments)
{
    if (_assetInput.strSourcePath.trimmed().isEmpty()) {
        emitFailure(_assetInput, tr("当前矢量资产没有源文件路径，无法执行缓冲区分析"),
            QStringLiteral("buffer_analysis"));
        return;
    }

    if (_dDistance <= 0.0) {
        emitFailure(_assetInput, tr("缓冲距离必须大于 0"),
            QStringLiteral("buffer_analysis"));
        return;
    }

    if (_nSegments <= 0) {
        emitFailure(_assetInput, tr("圆弧分段数必须大于 0"),
            QStringLiteral("buffer_analysis"));
        return;
    }

    QgsVectorLayer _layerVector(
        _assetInput.strSourcePath,
        QFileInfo(_assetInput.strSourcePath).completeBaseName(),
        QStringLiteral("ogr"));
    if (!_layerVector.isValid()) {
        emitFailure(_assetInput,
            tr("无法读取矢量数据：%1").arg(_assetInput.strSourcePath),
            QStringLiteral("buffer_analysis"));
        return;
    }

    const long long _lFeatureTotal = _layerVector.featureCount();
    if (_lFeatureTotal <= 0) {
        emitFailure(_assetInput, tr("当前矢量图层没有可缓冲的要素"),
            QStringLiteral("buffer_analysis"));
        return;
    }

    emit analysisProgress(0);

    const QString _strOutputPath = buildBufferOutputPath(_assetInput, _dDistance);
    const QString _strOutputLayerName =
        QFileInfo(_strOutputPath).completeBaseName();

    QgsVectorFileWriter::SaveVectorOptions _options;
    _options.driverName = QStringLiteral("GeoJSON");
    _options.layerName = _strOutputLayerName;
    _options.fileEncoding = QStringLiteral("UTF-8");
    _options.actionOnExistingFile = QgsVectorFileWriter::CreateOrOverwriteFile;

    std::unique_ptr<QgsVectorFileWriter> _pWriter(
        QgsVectorFileWriter::create(
            _strOutputPath,
            _layerVector.fields(),
            Qgis::WkbType::MultiPolygon,
            _layerVector.crs(),
            QgsProject::instance()->transformContext(),
            _options));

    if (!_pWriter || _pWriter->hasError() != QgsVectorFileWriter::NoError) {
        const QString _strError =
            (_pWriter != nullptr && !_pWriter->errorMessage().trimmed().isEmpty())
            ? _pWriter->errorMessage()
            : tr("未知写出错误");
        emitFailure(_assetInput,
            tr("创建缓冲结果文件失败：%1").arg(_strError),
            QStringLiteral("buffer_analysis"));
        return;
    }

    long long _lProcessedCount = 0;
    int _nBufferedCount = 0;
    int _nSkippedCount = 0;
    double _dAreaTotal = 0.0;
    double _dAreaMin = std::numeric_limits<double>::max();
    double _dAreaMax = 0.0;

    QgsFeature _featureCurrent;
    QgsFeatureIterator _itFeature = _layerVector.getFeatures();
    while (_itFeature.nextFeature(_featureCurrent)) {
        ++_lProcessedCount;

        const QgsGeometry _geomSource = _featureCurrent.geometry();
        if (_geomSource.isNull() || _geomSource.isEmpty()) {
            ++_nSkippedCount;
            continue;
        }

        QgsGeometry _geomBuffer = _geomSource.buffer(_dDistance, _nSegments);
        if (_geomBuffer.isNull() || _geomBuffer.isEmpty()) {
            ++_nSkippedCount;
            continue;
        }

        _geomBuffer.convertToMultiType();

        QgsFeature _featureOutput;
        _featureOutput.setFields(_layerVector.fields(), true);
        _featureOutput.setAttributes(_featureCurrent.attributes());
        _featureOutput.setGeometry(_geomBuffer);
        if (!_pWriter->addFeature(_featureOutput)) {
            ++_nSkippedCount;
            continue;
        }

        const double _dAreaCurrent = _geomBuffer.area();
        _dAreaTotal += _dAreaCurrent;
        _dAreaMin = qMin(_dAreaMin, _dAreaCurrent);
        _dAreaMax = qMax(_dAreaMax, _dAreaCurrent);
        ++_nBufferedCount;

        const int _nProgress = static_cast<int>(
            qMin(99.0, 100.0 * static_cast<double>(_lProcessedCount)
                / static_cast<double>(_lFeatureTotal)));
        emit analysisProgress(_nProgress);
    }

    _pWriter.reset();

    if (_nBufferedCount <= 0) {
        emitFailure(_assetInput, tr("缓冲区分析未生成有效结果，请检查源几何"),
            QStringLiteral("buffer_analysis"));
        return;
    }

    emit analysisProgress(100);

    AnalysisResult _result;
    _result.strType = QStringLiteral("buffer_analysis");
    _result.strToolId = QStringLiteral("buffer_analysis");
    _result.strSourceAssetId = _assetInput.strAssetId;
    _result.strSourceAssetName = _assetInput.strName;
    _result.bSuccess = true;
    _result.bHasVisualization = false;
    _result.bHasOutputLayer = true;
    _result.strOutputPath = _strOutputPath;
    _result.strOutputLayerName = _strOutputLayerName;
    _result.strOutputLayerType = QStringLiteral("vector");
    _result.strDesc = tr(
        "缓冲区分析完成\n"
        "数据资产：%1\n"
        "缓冲距离：%2（源图层 CRS 单位）\n"
        "圆弧分段数：%3\n"
        "源要素数：%4\n"
        "成功缓冲要素数：%5\n"
        "跳过要素数：%6\n"
        "缓冲面积总和：%7\n"
        "最小缓冲面积：%8\n"
        "最大缓冲面积：%9\n"
        "输出图层：%10")
        .arg(_assetInput.strName)
        .arg(_dDistance, 0, 'f', 6)
        .arg(_nSegments)
        .arg(_lFeatureTotal)
        .arg(_nBufferedCount)
        .arg(_nSkippedCount)
        .arg(_dAreaTotal, 0, 'f', 6)
        .arg(_dAreaMin, 0, 'f', 6)
        .arg(_dAreaMax, 0, 'f', 6)
        .arg(_strOutputPath);
    emit analysisFinished(_result);
}

int SpatialAnalysisService::ValueIndex(int _nRowIdx, int _nColIdx, int _nColCount)
{
    return _nRowIdx * _nColCount + _nColIdx;
}

void SpatialAnalysisService::emitFailure(const AnalysisDataAsset& _assetInput,
    const QString& _strError,
    const QString& _strToolId)
{
    AnalysisResult _result;
    const QString _strResolvedToolId = _strToolId.trimmed().isEmpty()
        ? QStringLiteral("neighborhood_analysis")
        : _strToolId;
    _result.strType = _strResolvedToolId;
    _result.strToolId = _strResolvedToolId;
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
