#include "spatialanalysisservice.h"

#include <limits>
#include <memory>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVariant>
#include <QVector>
#include <QtMath>

#include <qgscoordinatetransform.h>
#include <qgsexception.h>
#include <qgsfeature.h>
#include <qgsfeatureiterator.h>
#include <qgsfeaturesink.h>
#include <qgsfield.h>
#include <qgsfields.h>
#include <qgsgeometry.h>
#include <qgsproject.h>
#include <qgsvectorfilewriter.h>
#include <qgsvectorlayer.h>
#include <qgswkbtypes.h>

namespace
{

struct OverlayPreparedFeature
{
    QgsFeature  feature;  // 原始要素属性
    QgsGeometry geometry; // 已转换到源图层 CRS 且修复后的几何
};

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

QString buildOverlayOutputPath(const AnalysisDataAsset& _assetInput,
    const AnalysisDataAsset& _assetOverlay,
    OverlayOperationType _eOperation)
{
    const QFileInfo _fiSource(_assetInput.strSourcePath);
    const QFileInfo _fiOverlay(_assetOverlay.strSourcePath);
    QString _strSourceBaseName = _fiSource.completeBaseName();
    QString _strOverlayBaseName = _fiOverlay.completeBaseName();
    if (_strSourceBaseName.trimmed().isEmpty()) {
        _strSourceBaseName = _assetInput.strName;
    }
    if (_strOverlayBaseName.trimmed().isEmpty()) {
        _strOverlayBaseName = _assetOverlay.strName;
    }

    const QString _strOperation = (_eOperation == OverlayOperationType::Union)
        ? QStringLiteral("union")
        : QStringLiteral("intersect");
    const QString _strFileName = QStringLiteral("%1_overlay_%2_%3.geojson")
        .arg(sanitizeFilePart(_strSourceBaseName),
            sanitizeFilePart(_strOperation),
            sanitizeFilePart(_strOverlayBaseName));
    return QDir(writableOutputDirectory(_assetInput.strSourcePath))
        .absoluteFilePath(_strFileName);
}

QString overlayOperationDisplayName(OverlayOperationType _eOperation)
{
    return (_eOperation == OverlayOperationType::Union)
        ? QObject::tr("联合")
        : QObject::tr("交集");
}

QString buildUniqueFieldName(const QgsFields& _fieldsExisting,
    const QString& _strPrefix,
    const QString& _strFieldName)
{
    QString _strBaseName = sanitizeFilePart(_strPrefix + _strFieldName);
    if (_strBaseName.trimmed().isEmpty()) {
        _strBaseName = sanitizeFilePart(_strPrefix + QStringLiteral("field"));
    }

    _strBaseName = _strBaseName.left(48);
    QString _strCandidate = _strBaseName;
    int _nSuffix = 2;
    while (_fieldsExisting.indexOf(_strCandidate) >= 0) {
        _strCandidate = QStringLiteral("%1_%2")
            .arg(_strBaseName.left(44))
            .arg(_nSuffix);
        ++_nSuffix;
    }
    return _strCandidate;
}

QgsFields buildOverlayOutputFields(const QgsFields& _fieldsSource,
    const QgsFields& _fieldsOverlay)
{
    QgsFields _fieldsOutput;
    for (int _nFieldIdx = 0; _nFieldIdx < _fieldsSource.size(); ++_nFieldIdx) {
        QgsField _fieldCurrent(_fieldsSource.at(_nFieldIdx));
        _fieldCurrent.setName(buildUniqueFieldName(
            _fieldsOutput, QStringLiteral("src_"), _fieldCurrent.name()));
        _fieldsOutput.append(_fieldCurrent);
    }

    for (int _nFieldIdx = 0; _nFieldIdx < _fieldsOverlay.size(); ++_nFieldIdx) {
        QgsField _fieldCurrent(_fieldsOverlay.at(_nFieldIdx));
        _fieldCurrent.setName(buildUniqueFieldName(
            _fieldsOutput, QStringLiteral("ovl_"), _fieldCurrent.name()));
        _fieldsOutput.append(_fieldCurrent);
    }
    return _fieldsOutput;
}

void appendFeatureAttributes(QgsAttributes& _attributesOutput,
    const QgsFeature* _pFeatureInput,
    int _nFieldCount)
{
    if (_pFeatureInput == nullptr) {
        for (int _nFieldIdx = 0; _nFieldIdx < _nFieldCount; ++_nFieldIdx) {
            _attributesOutput.append(QVariant());
        }
        return;
    }

    const QgsAttributes _attributesInput = _pFeatureInput->attributes();
    for (int _nFieldIdx = 0; _nFieldIdx < _nFieldCount; ++_nFieldIdx) {
        _attributesOutput.append(_nFieldIdx < _attributesInput.size()
            ? _attributesInput.at(_nFieldIdx)
            : QVariant());
    }
}

QgsAttributes buildOverlayAttributes(const QgsFeature* _pFeatureSource,
    int _nSourceFieldCount,
    const QgsFeature* _pFeatureOverlay,
    int _nOverlayFieldCount)
{
    QgsAttributes _attributesOutput;
    _attributesOutput.reserve(_nSourceFieldCount + _nOverlayFieldCount);
    appendFeatureAttributes(_attributesOutput, _pFeatureSource, _nSourceFieldCount);
    appendFeatureAttributes(_attributesOutput, _pFeatureOverlay, _nOverlayFieldCount);
    return _attributesOutput;
}

bool normalizeGeometry(QgsGeometry& _geomInput)
{
    if (_geomInput.isNull() || _geomInput.isEmpty()) {
        return false;
    }
    if (!_geomInput.isGeosValid()) {
        _geomInput = _geomInput.makeValid();
    }
    return !_geomInput.isNull() && !_geomInput.isEmpty();
}

bool transformGeometryToSourceCrs(QgsGeometry& _geomInput,
    const QgsCoordinateTransform& _ctTransform,
    bool _bNeedsTransform)
{
    if (!_bNeedsTransform) {
        return true;
    }

    try {
        const Qgis::GeometryOperationResult _eTransformResult =
            _geomInput.transform(_ctTransform);
        return _eTransformResult == Qgis::GeometryOperationResult::Success;
    } catch (const QgsCsException&) {
        return false;
    }
}

QVector<OverlayPreparedFeature> collectPreparedFeatures(QgsVectorLayer& _layerInput,
    const QgsCoordinateTransform& _ctTransform,
    bool _bNeedsTransform,
    int& _nSkippedCount)
{
    QVector<OverlayPreparedFeature> _vPreparedFeatures;
    _vPreparedFeatures.reserve(static_cast<int>(qMax(0LL, _layerInput.featureCount())));

    QgsFeature _featureCurrent;
    QgsFeatureIterator _itFeature = _layerInput.getFeatures();
    while (_itFeature.nextFeature(_featureCurrent)) {
        QgsGeometry _geomCurrent = _featureCurrent.geometry();
        if (!transformGeometryToSourceCrs(
            _geomCurrent, _ctTransform, _bNeedsTransform)
            || !normalizeGeometry(_geomCurrent)) {
            ++_nSkippedCount;
            continue;
        }

        OverlayPreparedFeature _preparedCurrent;
        _preparedCurrent.feature = _featureCurrent;
        _preparedCurrent.geometry = _geomCurrent;
        _vPreparedFeatures.append(_preparedCurrent);
    }
    return _vPreparedFeatures;
}

bool geometriesMayIntersect(const QgsGeometry& _geomLeft,
    const QgsGeometry& _geomRight)
{
    return !_geomLeft.isNull()
        && !_geomLeft.isEmpty()
        && !_geomRight.isNull()
        && !_geomRight.isEmpty()
        && _geomLeft.boundingBoxIntersects(_geomRight)
        && _geomLeft.intersects(_geomRight);
}

bool prepareOverlayResultGeometry(QgsGeometry& _geomOutput)
{
    return normalizeGeometry(_geomOutput);
}

bool addOverlayOutputFeature(QgsVectorFileWriter* _pWriter,
    const QgsFields& _fieldsOutput,
    QgsGeometry _geomOutput,
    const QgsAttributes& _attributesOutput)
{
    if (_pWriter == nullptr || !prepareOverlayResultGeometry(_geomOutput)) {
        return false;
    }

    QgsFeature _featureOutput;
    _featureOutput.setFields(_fieldsOutput, true);
    _featureOutput.setAttributes(_attributesOutput);
    _featureOutput.setGeometry(_geomOutput);
    return _pWriter->addFeature(_featureOutput);
}

void accumulateGeometryMetrics(const QgsGeometry& _geomInput,
    double& _dAreaTotal,
    double& _dLengthTotal)
{
    _dAreaTotal += _geomInput.area();
    _dLengthTotal += _geomInput.length();
}

QgsGeometry subtractOverlayGeometry(QgsGeometry _geomBase,
    const QgsGeometry& _geomSubtract,
    int& _nSkippedCount)
{
    if (!geometriesMayIntersect(_geomBase, _geomSubtract)) {
        return _geomBase;
    }

    QgsGeometry _geomDifference = _geomBase.difference(_geomSubtract);
    if (_geomDifference.isNull()) {
        ++_nSkippedCount;
        return QgsGeometry();
    }
    if (!_geomDifference.isEmpty()
        && !prepareOverlayResultGeometry(_geomDifference)) {
        ++_nSkippedCount;
        return QgsGeometry();
    }
    return _geomDifference;
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

void SpatialAnalysisService::runOverlayAnalysis(
    const AnalysisDataAsset& _assetInput,
    const AnalysisDataAsset& _assetOverlay,
    OverlayOperationType _eOperation)
{
    if (_assetInput.strSourcePath.trimmed().isEmpty()
        || _assetOverlay.strSourcePath.trimmed().isEmpty()) {
        emitFailure(_assetInput, tr("叠加分析需要两个有效的矢量资产源文件"),
            QStringLiteral("overlay_analysis"));
        return;
    }

    if (_assetInput.strAssetId == _assetOverlay.strAssetId
        || QFileInfo(_assetInput.strSourcePath).absoluteFilePath()
            == QFileInfo(_assetOverlay.strSourcePath).absoluteFilePath()) {
        emitFailure(_assetInput, tr("叠加分析需要选择两个不同的矢量资产"),
            QStringLiteral("overlay_analysis"));
        return;
    }

    QgsVectorLayer _layerSource(
        _assetInput.strSourcePath,
        QFileInfo(_assetInput.strSourcePath).completeBaseName(),
        QStringLiteral("ogr"));
    if (!_layerSource.isValid()) {
        emitFailure(_assetInput,
            tr("无法读取源矢量数据：%1").arg(_assetInput.strSourcePath),
            QStringLiteral("overlay_analysis"));
        return;
    }

    QgsVectorLayer _layerOverlay(
        _assetOverlay.strSourcePath,
        QFileInfo(_assetOverlay.strSourcePath).completeBaseName(),
        QStringLiteral("ogr"));
    if (!_layerOverlay.isValid()) {
        emitFailure(_assetInput,
            tr("无法读取叠加矢量数据：%1").arg(_assetOverlay.strSourcePath),
            QStringLiteral("overlay_analysis"));
        return;
    }

    const long long _lSourceTotal = _layerSource.featureCount();
    const long long _lOverlayTotal = _layerOverlay.featureCount();
    if (_lSourceTotal <= 0 || _lOverlayTotal <= 0) {
        emitFailure(_assetInput, tr("叠加分析要求两个矢量图层都包含有效要素"),
            QStringLiteral("overlay_analysis"));
        return;
    }

    emit analysisProgress(0);

    const QgsFields _fieldsOutput = buildOverlayOutputFields(
        _layerSource.fields(), _layerOverlay.fields());
    const QString _strOutputPath = buildOverlayOutputPath(
        _assetInput, _assetOverlay, _eOperation);
    const QString _strOutputLayerName = QFileInfo(_strOutputPath).completeBaseName();

    QgsVectorFileWriter::SaveVectorOptions _options;
    _options.driverName = QStringLiteral("GeoJSON");
    _options.layerName = _strOutputLayerName;
    _options.fileEncoding = QStringLiteral("UTF-8");
    _options.actionOnExistingFile = QgsVectorFileWriter::CreateOrOverwriteFile;

    std::unique_ptr<QgsVectorFileWriter> _pWriter(
        QgsVectorFileWriter::create(
            _strOutputPath,
            _fieldsOutput,
            Qgis::WkbType::Unknown,
            _layerSource.crs(),
            QgsProject::instance()->transformContext(),
            _options));

    if (!_pWriter || _pWriter->hasError() != QgsVectorFileWriter::NoError) {
        const QString _strError =
            (_pWriter != nullptr && !_pWriter->errorMessage().trimmed().isEmpty())
            ? _pWriter->errorMessage()
            : tr("未知写出错误");
        emitFailure(_assetInput,
            tr("创建叠加结果文件失败：%1").arg(_strError),
            QStringLiteral("overlay_analysis"));
        return;
    }

    const bool _bNeedsTransform = _layerSource.crs().isValid()
        && _layerOverlay.crs().isValid()
        && _layerSource.crs() != _layerOverlay.crs();
    const QgsCoordinateTransform _ctOverlayToSource(
        _layerOverlay.crs(),
        _layerSource.crs(),
        QgsProject::instance()->transformContext());

    const QgsCoordinateTransform _ctIdentity(
        _layerSource.crs(),
        _layerSource.crs(),
        QgsProject::instance()->transformContext());

    long long _lPairTestCount = 0;
    int _nIntersectionCount = 0;
    int _nSourceOnlyCount = 0;
    int _nOverlayOnlyCount = 0;
    int _nSkippedCount = 0;
    double _dAreaTotal = 0.0;
    double _dLengthTotal = 0.0;

    const QVector<OverlayPreparedFeature> _vSourceFeatures =
        collectPreparedFeatures(_layerSource, _ctIdentity, false, _nSkippedCount);
    const QVector<OverlayPreparedFeature> _vOverlayFeatures =
        collectPreparedFeatures(_layerOverlay, _ctOverlayToSource,
            _bNeedsTransform, _nSkippedCount);

    if (_vSourceFeatures.isEmpty() || _vOverlayFeatures.isEmpty()) {
        _pWriter.reset();
        emitFailure(_assetInput, tr("叠加分析要求两个矢量图层都包含可用几何"),
            QStringLiteral("overlay_analysis"));
        return;
    }

    for (int _nSourceIdx = 0; _nSourceIdx < _vSourceFeatures.size(); ++_nSourceIdx) {
        const OverlayPreparedFeature& _preparedSource =
            _vSourceFeatures.at(_nSourceIdx);
        QgsGeometry _geomSourceRemainder = _preparedSource.geometry;

        for (const OverlayPreparedFeature& _preparedOverlay : _vOverlayFeatures) {
            ++_lPairTestCount;

            if (!geometriesMayIntersect(
                _preparedSource.geometry, _preparedOverlay.geometry)) {
                continue;
            }

            QgsGeometry _geomIntersection =
                _preparedSource.geometry.intersection(_preparedOverlay.geometry);
            if (!_geomIntersection.isNull()
                && !_geomIntersection.isEmpty()
                && prepareOverlayResultGeometry(_geomIntersection)) {
                const QgsAttributes _attributesOutput = buildOverlayAttributes(
                    &_preparedSource.feature,
                    _layerSource.fields().size(),
                    &_preparedOverlay.feature,
                    _layerOverlay.fields().size());
                if (!addOverlayOutputFeature(
                    _pWriter.get(), _fieldsOutput, _geomIntersection,
                    _attributesOutput)) {
                    ++_nSkippedCount;
                    continue;
                }

                accumulateGeometryMetrics(
                    _geomIntersection, _dAreaTotal, _dLengthTotal);
                ++_nIntersectionCount;
            } else if (!_geomIntersection.isNull()) {
                ++_nSkippedCount;
            }

            if (_eOperation == OverlayOperationType::Union) {
                _geomSourceRemainder = subtractOverlayGeometry(
                    _geomSourceRemainder,
                    _preparedOverlay.geometry,
                    _nSkippedCount);
                if (_geomSourceRemainder.isNull() || _geomSourceRemainder.isEmpty()) {
                    break;
                }
            }
        }

        const int _nProgress = static_cast<int>(
            qMin(80.0, 80.0 * static_cast<double>(_nSourceIdx + 1)
                / static_cast<double>(_vSourceFeatures.size())));
        emit analysisProgress(_nProgress);

        if (_eOperation == OverlayOperationType::Union
            && !_geomSourceRemainder.isNull()
            && !_geomSourceRemainder.isEmpty()) {
            const QgsAttributes _attributesOutput = buildOverlayAttributes(
                &_preparedSource.feature,
                _layerSource.fields().size(),
                nullptr,
                _layerOverlay.fields().size());
            if (addOverlayOutputFeature(
                _pWriter.get(), _fieldsOutput, _geomSourceRemainder,
                _attributesOutput)) {
                accumulateGeometryMetrics(
                    _geomSourceRemainder, _dAreaTotal, _dLengthTotal);
                ++_nSourceOnlyCount;
            } else {
                ++_nSkippedCount;
            }
        }
    }

    if (_eOperation == OverlayOperationType::Union) {
        for (int _nOverlayIdx = 0;
            _nOverlayIdx < _vOverlayFeatures.size();
            ++_nOverlayIdx) {
            const OverlayPreparedFeature& _preparedOverlay =
                _vOverlayFeatures.at(_nOverlayIdx);
            QgsGeometry _geomOverlayRemainder = _preparedOverlay.geometry;

            for (const OverlayPreparedFeature& _preparedSource : _vSourceFeatures) {
                _geomOverlayRemainder = subtractOverlayGeometry(
                    _geomOverlayRemainder,
                    _preparedSource.geometry,
                    _nSkippedCount);
                if (_geomOverlayRemainder.isNull()
                    || _geomOverlayRemainder.isEmpty()) {
                    break;
                }
            }

            if (!_geomOverlayRemainder.isNull()
                && !_geomOverlayRemainder.isEmpty()) {
                const QgsAttributes _attributesOutput = buildOverlayAttributes(
                    nullptr,
                    _layerSource.fields().size(),
                    &_preparedOverlay.feature,
                    _layerOverlay.fields().size());
                if (addOverlayOutputFeature(
                    _pWriter.get(), _fieldsOutput, _geomOverlayRemainder,
                    _attributesOutput)) {
                    accumulateGeometryMetrics(
                        _geomOverlayRemainder, _dAreaTotal, _dLengthTotal);
                    ++_nOverlayOnlyCount;
                } else {
                    ++_nSkippedCount;
                }
            }

            const int _nProgress = 80 + static_cast<int>(
                qMin(19.0, 19.0 * static_cast<double>(_nOverlayIdx + 1)
                    / static_cast<double>(_vOverlayFeatures.size())));
            emit analysisProgress(_nProgress);
        }
    }

    _pWriter.reset();

    const int _nOutputCount =
        _nIntersectionCount + _nSourceOnlyCount + _nOverlayOnlyCount;
    if (_nOutputCount <= 0) {
        const QString _strFailure = (_eOperation == OverlayOperationType::Union)
            ? tr("叠加联合分析未生成有效结果，请检查两个图层的几何")
            : tr("叠加分析未找到相交要素，请检查两个图层的空间范围");
        emitFailure(_assetInput, _strFailure,
            QStringLiteral("overlay_analysis"));
        return;
    }

    emit analysisProgress(100);

    AnalysisResult _result;
    _result.strType = QStringLiteral("overlay_analysis");
    _result.strToolId = QStringLiteral("overlay_analysis");
    _result.strSourceAssetId = _assetInput.strAssetId;
    _result.strSourceAssetName = tr("%1 × %2").arg(
        _assetInput.strName, _assetOverlay.strName);
    _result.bSuccess = true;
    _result.bHasVisualization = false;
    _result.bHasOutputLayer = true;
    _result.strOutputPath = _strOutputPath;
    _result.strOutputLayerName = _strOutputLayerName;
    _result.strOutputLayerType = QStringLiteral("vector");
    _result.strDesc = tr(
        "叠加分析完成\n"
        "分析类型：%1\n"
        "源数据资产：%2\n"
        "叠加数据资产：%3\n"
        "源要素数：%4（可用几何：%5）\n"
        "叠加要素数：%6（可用几何：%7）\n"
        "候选要素对数：%8\n"
        "输出相交要素数：%9\n"
        "输出源独有要素数：%10\n"
        "输出叠加独有要素数：%11\n"
        "输出要素总数：%12\n"
        "跳过要素数：%13\n"
        "结果面积总和：%14\n"
        "结果长度总和：%15\n"
        "输出图层：%16")
        .arg(overlayOperationDisplayName(_eOperation))
        .arg(_assetInput.strName)
        .arg(_assetOverlay.strName)
        .arg(_lSourceTotal)
        .arg(_vSourceFeatures.size())
        .arg(_lOverlayTotal)
        .arg(_vOverlayFeatures.size())
        .arg(_lPairTestCount)
        .arg(_nIntersectionCount)
        .arg(_nSourceOnlyCount)
        .arg(_nOverlayOnlyCount)
        .arg(_nOutputCount)
        .arg(_nSkippedCount)
        .arg(_dAreaTotal, 0, 'f', 6)
        .arg(_dLengthTotal, 0, 'f', 6)
        .arg(_strOutputPath);
    emit analysisFinished(_result);
}

void SpatialAnalysisService::runOverlayIntersectionAnalysis(
    const AnalysisDataAsset& _assetInput,
    const AnalysisDataAsset& _assetOverlay)
{
    runOverlayAnalysis(_assetInput, _assetOverlay, OverlayOperationType::Intersect);
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
