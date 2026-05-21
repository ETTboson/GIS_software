#include "attributequeryservice.h"

#include "service/data/spatialdatabaseservice.h"

#include <limits>
#include <memory>

#include <QDir>
#include <QFileInfo>
#include <QMetaType>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVariant>
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

namespace
{

struct VectorOutputTarget
{
    QString strDatabasePath; // 目标 SpatiaLite 数据库路径
    QString strTableName;    // 目标结果表名
    QString strSourceUri;    // 目标结果图层 URI
    QString strLayerName;    // 结果图层显示名
};

QString sanitizeFilePart(const QString& _strInput)
{
    QString _strSafe = _strInput.trimmed();
    if (_strSafe.isEmpty()) {
        _strSafe = QStringLiteral("query_result");
    }
    _strSafe.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_\\-]+")),
        QStringLiteral("_"));
    while (_strSafe.startsWith('_')) {
        _strSafe.remove(0, 1);
    }
    while (_strSafe.endsWith('_')) {
        _strSafe.chop(1);
    }
    return _strSafe.isEmpty() ? QStringLiteral("query_result") : _strSafe;
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

QString sourceFilePathForAsset(const AnalysisDataAsset& _assetInput)
{
    QString _strDatabasePath;
    QString _strTableName;
    if (SpatialDatabaseService::parseLayerSourceUri(
            _assetInput.strSourcePath, _strDatabasePath, _strTableName)) {
        return _strDatabasePath;
    }
    if (!_assetInput.dataVector.strDatabasePath.trimmed().isEmpty()) {
        return _assetInput.dataVector.strDatabasePath;
    }
    return _assetInput.strSourcePath;
}

QString sourceLayerUriForAsset(const AnalysisDataAsset& _assetInput)
{
    if (!_assetInput.dataVector.strSourceUri.trimmed().isEmpty()) {
        return _assetInput.dataVector.strSourceUri;
    }
    return _assetInput.strSourcePath;
}

QString sourceBaseNameForAsset(const AnalysisDataAsset& _assetInput)
{
    QString _strDatabasePath;
    QString _strTableName;
    if (SpatialDatabaseService::parseLayerSourceUri(
            _assetInput.strSourcePath, _strDatabasePath, _strTableName)
        && !_strTableName.trimmed().isEmpty()) {
        return _strTableName;
    }
    if (!_assetInput.dataVector.strTableName.trimmed().isEmpty()) {
        return _assetInput.dataVector.strTableName;
    }

    QString _strBaseName = QFileInfo(sourceFilePathForAsset(_assetInput))
        .completeBaseName();
    if (_strBaseName.trimmed().isEmpty()) {
        _strBaseName = _assetInput.strName;
    }
    return _strBaseName;
}

QString resultDatabasePathForAsset(const AnalysisDataAsset& _assetInput)
{
    return QDir(writableOutputDirectory(sourceFilePathForAsset(_assetInput)))
        .absoluteFilePath(QStringLiteral("analysis_results.sqlite"));
}

QString normalizedOperatorId(const QString& _strOperatorId)
{
    const QString _strNormalized = _strOperatorId.trimmed().toLower();
    if (_strNormalized == QStringLiteral("=")
        || _strNormalized == QStringLiteral("==")
        || _strNormalized == QStringLiteral("eq")) {
        return QStringLiteral("eq");
    }
    if (_strNormalized == QStringLiteral("!=")
        || _strNormalized == QStringLiteral("<>")
        || _strNormalized == QStringLiteral("ne")) {
        return QStringLiteral("ne");
    }
    if (_strNormalized == QStringLiteral(">") || _strNormalized == QStringLiteral("gt")) {
        return QStringLiteral("gt");
    }
    if (_strNormalized == QStringLiteral(">=") || _strNormalized == QStringLiteral("gte")) {
        return QStringLiteral("gte");
    }
    if (_strNormalized == QStringLiteral("<") || _strNormalized == QStringLiteral("lt")) {
        return QStringLiteral("lt");
    }
    if (_strNormalized == QStringLiteral("<=") || _strNormalized == QStringLiteral("lte")) {
        return QStringLiteral("lte");
    }
    if (_strNormalized == QStringLiteral("contains")
        || _strNormalized == QString::fromUtf8("包含")) {
        return QStringLiteral("contains");
    }
    return QString();
}

QString operatorDisplayName(const QString& _strOperatorId)
{
    const QString _strOperator = normalizedOperatorId(_strOperatorId);
    if (_strOperator == QStringLiteral("eq")) {
        return QStringLiteral("=");
    }
    if (_strOperator == QStringLiteral("ne")) {
        return QStringLiteral("!=");
    }
    if (_strOperator == QStringLiteral("gt")) {
        return QStringLiteral(">");
    }
    if (_strOperator == QStringLiteral("gte")) {
        return QStringLiteral(">=");
    }
    if (_strOperator == QStringLiteral("lt")) {
        return QStringLiteral("<");
    }
    if (_strOperator == QStringLiteral("lte")) {
        return QStringLiteral("<=");
    }
    if (_strOperator == QStringLiteral("contains")) {
        return QObject::tr("包含");
    }
    return _strOperatorId;
}

bool isNumericVariantType(QMetaType::Type _eType)
{
    return _eType == QMetaType::Int
        || _eType == QMetaType::UInt
        || _eType == QMetaType::LongLong
        || _eType == QMetaType::ULongLong
        || _eType == QMetaType::Double;
}

bool isNumericOperator(const QString& _strOperatorId)
{
    return _strOperatorId == QStringLiteral("eq")
        || _strOperatorId == QStringLiteral("ne")
        || _strOperatorId == QStringLiteral("gt")
        || _strOperatorId == QStringLiteral("gte")
        || _strOperatorId == QStringLiteral("lt")
        || _strOperatorId == QStringLiteral("lte");
}

bool isTextOperator(const QString& _strOperatorId)
{
    return _strOperatorId == QStringLiteral("eq")
        || _strOperatorId == QStringLiteral("ne")
        || _strOperatorId == QStringLiteral("contains");
}

bool matchNumericValue(double _dFieldValue,
    double _dExpectedValue,
    const QString& _strOperatorId)
{
    const double _dTolerance = 1e-12;
    if (_strOperatorId == QStringLiteral("eq")) {
        return qAbs(_dFieldValue - _dExpectedValue) <= _dTolerance;
    }
    if (_strOperatorId == QStringLiteral("ne")) {
        return qAbs(_dFieldValue - _dExpectedValue) > _dTolerance;
    }
    if (_strOperatorId == QStringLiteral("gt")) {
        return _dFieldValue > _dExpectedValue;
    }
    if (_strOperatorId == QStringLiteral("gte")) {
        return _dFieldValue >= _dExpectedValue;
    }
    if (_strOperatorId == QStringLiteral("lt")) {
        return _dFieldValue < _dExpectedValue;
    }
    if (_strOperatorId == QStringLiteral("lte")) {
        return _dFieldValue <= _dExpectedValue;
    }
    return false;
}

bool matchTextValue(const QString& _strFieldValue,
    const QString& _strExpectedValue,
    const QString& _strOperatorId)
{
    if (_strOperatorId == QStringLiteral("eq")) {
        return _strFieldValue.compare(_strExpectedValue, Qt::CaseInsensitive) == 0;
    }
    if (_strOperatorId == QStringLiteral("ne")) {
        return _strFieldValue.compare(_strExpectedValue, Qt::CaseInsensitive) != 0;
    }
    if (_strOperatorId == QStringLiteral("contains")) {
        return _strFieldValue.contains(_strExpectedValue, Qt::CaseInsensitive);
    }
    return false;
}

VectorOutputTarget buildAttributeOutputTarget(const AnalysisDataAsset& _assetInput,
    const QString& _strFieldName,
    const QString& _strOperatorId,
    const QString& _strValueText)
{
    const QString _strBaseName = sourceBaseNameForAsset(_assetInput);
    const QString _strValuePart = sanitizeFilePart(_strValueText).left(20);
    const QString _strTableName = SpatialDatabaseService::sanitizeIdentifier(
        QStringLiteral("%1_query_attr_%2_%3_%4")
            .arg(sanitizeFilePart(_strBaseName),
                sanitizeFilePart(_strFieldName),
                sanitizeFilePart(_strOperatorId),
                _strValuePart));

    VectorOutputTarget _targetOutput;
    _targetOutput.strDatabasePath = resultDatabasePathForAsset(_assetInput);
    _targetOutput.strTableName = _strTableName;
    _targetOutput.strSourceUri = SpatialDatabaseService::buildLayerSourceUri(
        _targetOutput.strDatabasePath, _targetOutput.strTableName);
    _targetOutput.strLayerName = _targetOutput.strTableName;
    return _targetOutput;
}

VectorOutputTarget buildSpatialOutputTarget(const AnalysisDataAsset& _assetSource,
    const AnalysisDataAsset& _assetTarget,
    const QString& _strRelationId)
{
    const QString _strSourceBaseName = sourceBaseNameForAsset(_assetSource);
    const QString _strTargetBaseName = sourceBaseNameForAsset(_assetTarget);
    const QString _strTableName = SpatialDatabaseService::sanitizeIdentifier(
        QStringLiteral("%1_query_%2_%3")
            .arg(sanitizeFilePart(_strSourceBaseName),
                sanitizeFilePart(_strRelationId),
                sanitizeFilePart(_strTargetBaseName)));

    VectorOutputTarget _targetOutput;
    _targetOutput.strDatabasePath = resultDatabasePathForAsset(_assetSource);
    _targetOutput.strTableName = _strTableName;
    _targetOutput.strSourceUri = SpatialDatabaseService::buildLayerSourceUri(
        _targetOutput.strDatabasePath, _targetOutput.strTableName);
    _targetOutput.strLayerName = _targetOutput.strTableName;
    return _targetOutput;
}

bool resolveWrittenOutputTarget(const VectorOutputTarget& _targetInput,
    VectorOutputTarget& _targetResolved,
    QString& _strError)
{
    SpatialDatabaseService _databaseService;
    SpatialTableInfo _tableInfo;
    if (!_databaseService.getSpatialTableInfo(
            _targetInput.strDatabasePath,
            _targetInput.strTableName,
            _tableInfo,
            _strError)) {
        if (_strError.trimmed().isEmpty()) {
            _strError = QObject::tr("结果已写入数据库，但未能确认空间表注册状态");
        }
        return false;
    }

    _targetResolved = _targetInput;
    _targetResolved.strTableName = _tableInfo.strTableName;
    _targetResolved.strSourceUri = _tableInfo.strSourceUri;
    _targetResolved.strLayerName = _tableInfo.strTableName;
    return true;
}

bool writeFeaturesToSpatialite(QgsVectorLayer& _layerSource,
    const QVector<QgsFeature>& _vMatchedFeatures,
    const VectorOutputTarget& _targetOutput,
    QString& _strError)
{
    const QFileInfo _fiOutputDatabase(_targetOutput.strDatabasePath);
    QgsVectorFileWriter::SaveVectorOptions _options =
        SpatialDatabaseService::buildSpatialiteSaveOptions(
            _targetOutput.strTableName,
            _fiOutputDatabase.exists());

    std::unique_ptr<QgsVectorFileWriter> _pWriter(
        QgsVectorFileWriter::create(
            _targetOutput.strDatabasePath,
            _layerSource.fields(),
            _layerSource.wkbType(),
            _layerSource.crs(),
            QgsProject::instance()->transformContext(),
            _options));

    if (!_pWriter || _pWriter->hasError() != QgsVectorFileWriter::NoError) {
        _strError = (_pWriter != nullptr && !_pWriter->errorMessage().trimmed().isEmpty())
            ? _pWriter->errorMessage()
            : QObject::tr("未知写出错误");
        return false;
    }

    for (const QgsFeature& _featureInput : _vMatchedFeatures) {
        QgsFeature _featureOutput(_featureInput);
        if (!_pWriter->addFeature(_featureOutput)) {
            _strError = QObject::tr("写入查询结果要素失败");
            return false;
        }
    }

    _pWriter.reset();
    return true;
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

QString normalizedRelationId(const QString& _strRelationId)
{
    const QString _strRelation = _strRelationId.trimmed().toLower();
    if (_strRelation == QStringLiteral("intersects")
        || _strRelation == QStringLiteral("intersect")
        || _strRelation == QString::fromUtf8("相交")) {
        return QStringLiteral("intersects");
    }
    if (_strRelation == QStringLiteral("within")
        || _strRelation == QString::fromUtf8("被包含")) {
        return QStringLiteral("within");
    }
    if (_strRelation == QStringLiteral("contains")
        || _strRelation == QString::fromUtf8("包含")) {
        return QStringLiteral("contains");
    }
    return QString();
}

QString relationDisplayName(const QString& _strRelationId)
{
    const QString _strRelation = normalizedRelationId(_strRelationId);
    if (_strRelation == QStringLiteral("within")) {
        return QObject::tr("Within");
    }
    if (_strRelation == QStringLiteral("contains")) {
        return QObject::tr("Contains");
    }
    return QObject::tr("Intersects");
}

bool geometryMatchesRelation(const QgsGeometry& _geomSource,
    const QgsGeometry& _geomTarget,
    const QString& _strRelationId)
{
    if (_geomSource.isNull() || _geomSource.isEmpty()
        || _geomTarget.isNull() || _geomTarget.isEmpty()) {
        return false;
    }

    const QString _strRelation = normalizedRelationId(_strRelationId);
    if (_strRelation == QStringLiteral("intersects")) {
        return _geomSource.boundingBoxIntersects(_geomTarget)
            && _geomSource.intersects(_geomTarget);
    }
    if (_strRelation == QStringLiteral("within")) {
        return _geomSource.boundingBox().intersects(_geomTarget.boundingBox())
            && _geomSource.within(_geomTarget);
    }
    if (_strRelation == QStringLiteral("contains")) {
        return _geomSource.boundingBox().contains(_geomTarget.boundingBox())
            && _geomSource.contains(_geomTarget);
    }
    return false;
}

AnalysisResult buildZeroMatchResult(const AnalysisDataAsset& _assetInput,
    const QString& _strToolId,
    const QString& _strDesc)
{
    AnalysisResult _result;
    _result.strType = _strToolId;
    _result.strToolId = _strToolId;
    _result.strSourceAssetId = _assetInput.strAssetId;
    _result.strSourceAssetName = _assetInput.strName;
    _result.bSuccess = true;
    _result.bHasVisualization = false;
    _result.bHasOutputLayer = false;
    _result.strDesc = _strDesc;
    return _result;
}

} // namespace

AttributeQueryService::AttributeQueryService(QObject* _pParent)
    : QObject(_pParent)
{
}

void AttributeQueryService::runAttributeQuery(
    const AnalysisDataAsset& _assetInput,
    const QString& _strFieldName,
    const QString& _strOperatorId,
    const QString& _strValueText)
{
    const QString _strInputUri = sourceLayerUriForAsset(_assetInput);
    if (_strInputUri.trimmed().isEmpty()) {
        emitFailure(_assetInput, tr("当前矢量资产没有源路径，无法执行属性查询"),
            QStringLiteral("attribute_query"));
        return;
    }

    const QString _strOperator = normalizedOperatorId(_strOperatorId);
    if (_strOperator.trimmed().isEmpty()) {
        emitFailure(_assetInput, tr("属性查询运算符无效：%1").arg(_strOperatorId),
            QStringLiteral("attribute_query"));
        return;
    }

    QgsVectorLayer _layerVector(
        _strInputUri,
        sourceBaseNameForAsset(_assetInput),
        QStringLiteral("ogr"));
    if (!_layerVector.isValid()) {
        emitFailure(_assetInput, tr("无法读取矢量数据：%1").arg(_strInputUri),
            QStringLiteral("attribute_query"));
        return;
    }

    const int _nFieldIdx = _layerVector.fields().indexOf(_strFieldName);
    if (_nFieldIdx < 0) {
        emitFailure(_assetInput, tr("字段不存在：%1").arg(_strFieldName),
            QStringLiteral("attribute_query"));
        return;
    }

    const QgsField _fieldQuery = _layerVector.fields().at(_nFieldIdx);
    const bool _bNumericField = isNumericVariantType(_fieldQuery.type());
    if (_bNumericField && !isNumericOperator(_strOperator)) {
        emitFailure(_assetInput, tr("数值字段不支持该运算符：%1")
            .arg(operatorDisplayName(_strOperator)),
            QStringLiteral("attribute_query"));
        return;
    }
    if (!_bNumericField && !isTextOperator(_strOperator)) {
        emitFailure(_assetInput, tr("文本字段只支持 =、!= 和 contains 查询"),
            QStringLiteral("attribute_query"));
        return;
    }

    bool _bExpectedOk = true;
    double _dExpectedValue = 0.0;
    if (_bNumericField) {
        _dExpectedValue = _strValueText.trimmed().toDouble(&_bExpectedOk);
        if (!_bExpectedOk) {
            emitFailure(_assetInput, tr("数值字段查询值不是有效数字：%1").arg(_strValueText),
                QStringLiteral("attribute_query"));
            return;
        }
    }

    emit analysisProgress(0);

    QVector<QgsFeature> _vMatchedFeatures;
    const long long _lFeatureTotal = qMax(0LL, _layerVector.featureCount());
    long long _lProcessedCount = 0;

    QgsFeature _featureCurrent;
    QgsFeatureIterator _itFeature = _layerVector.getFeatures();
    while (_itFeature.nextFeature(_featureCurrent)) {
        ++_lProcessedCount;

        const QVariant _valueField = _featureCurrent.attribute(_nFieldIdx);
        bool _bMatched = false;
        if (_bNumericField) {
            bool _bFieldOk = false;
            const double _dFieldValue = _valueField.toDouble(&_bFieldOk);
            _bMatched = _bFieldOk
                && matchNumericValue(_dFieldValue, _dExpectedValue, _strOperator);
        } else {
            _bMatched = matchTextValue(
                _valueField.toString(), _strValueText, _strOperator);
        }

        if (_bMatched) {
            _vMatchedFeatures.append(_featureCurrent);
        }

        if (_lFeatureTotal > 0) {
            const int _nProgress = static_cast<int>(
                qMin(95.0, 100.0 * static_cast<double>(_lProcessedCount)
                    / static_cast<double>(_lFeatureTotal)));
            emit analysisProgress(_nProgress);
        }
    }

    if (_vMatchedFeatures.isEmpty()) {
        emit analysisProgress(100);
        emit analysisFinished(buildZeroMatchResult(
            _assetInput,
            QStringLiteral("attribute_query"),
            tr(
                "属性查询完成\n"
                "数据资产：%1\n"
                "查询条件：%2 %3 %4\n"
                "源要素数：%5\n"
                "命中要素数：0\n"
                "未生成高亮结果图层。")
                .arg(_assetInput.strName,
                    _strFieldName,
                    operatorDisplayName(_strOperator),
                    _strValueText)
                .arg(_lFeatureTotal)));
        return;
    }

    const VectorOutputTarget _targetOutput = buildAttributeOutputTarget(
        _assetInput, _strFieldName, _strOperator, _strValueText);
    QString _strWriteError;
    if (!writeFeaturesToSpatialite(
            _layerVector, _vMatchedFeatures, _targetOutput, _strWriteError)) {
        emitFailure(_assetInput,
            tr("写出属性查询结果失败：%1").arg(_strWriteError),
            QStringLiteral("attribute_query"));
        return;
    }

    VectorOutputTarget _targetResolved;
    QString _strResolveError;
    if (!resolveWrittenOutputTarget(
            _targetOutput, _targetResolved, _strResolveError)) {
        emitFailure(_assetInput,
            tr("属性查询结果写入后无法确认空间表：%1").arg(_strResolveError),
            QStringLiteral("attribute_query"));
        return;
    }

    emit analysisProgress(100);

    AnalysisResult _result;
    _result.strType = QStringLiteral("attribute_query");
    _result.strToolId = QStringLiteral("attribute_query");
    _result.strSourceAssetId = _assetInput.strAssetId;
    _result.strSourceAssetName = _assetInput.strName;
    _result.bSuccess = true;
    _result.bHasVisualization = false;
    _result.bHasOutputLayer = true;
    _result.strOutputPath = _targetResolved.strSourceUri;
    _result.strOutputLayerName = _targetResolved.strLayerName;
    _result.strOutputLayerType = QStringLiteral("vector");
    _result.strDesc = tr(
        "属性查询完成\n"
        "数据资产：%1\n"
        "查询条件：%2 %3 %4\n"
        "源要素数：%5\n"
        "命中要素数：%6\n"
        "输出数据库：%7\n"
        "输出空间表：%8")
        .arg(_assetInput.strName,
            _strFieldName,
            operatorDisplayName(_strOperator),
            _strValueText)
        .arg(_lFeatureTotal)
        .arg(_vMatchedFeatures.size())
        .arg(_targetResolved.strDatabasePath,
            _targetResolved.strTableName);
    emit analysisFinished(_result);
}

void AttributeQueryService::runSpatialRelationQuery(
    const AnalysisDataAsset& _assetSource,
    const AnalysisDataAsset& _assetTarget,
    const QString& _strRelationId)
{
    const QString _strRelation = normalizedRelationId(_strRelationId);
    if (_strRelation.trimmed().isEmpty()) {
        emitFailure(_assetSource, tr("空间关系无效：%1").arg(_strRelationId),
            QStringLiteral("spatial_query"));
        return;
    }

    const QString _strSourceUri = sourceLayerUriForAsset(_assetSource);
    const QString _strTargetUri = sourceLayerUriForAsset(_assetTarget);
    if (_strSourceUri.trimmed().isEmpty() || _strTargetUri.trimmed().isEmpty()) {
        emitFailure(_assetSource, tr("空间查询需要两个有效矢量资产"),
            QStringLiteral("spatial_query"));
        return;
    }

    if (_assetSource.strAssetId == _assetTarget.strAssetId
        || _strSourceUri.trimmed() == _strTargetUri.trimmed()) {
        emitFailure(_assetSource, tr("空间查询需要选择另一个区域图层作为目标"),
            QStringLiteral("spatial_query"));
        return;
    }

    QgsVectorLayer _layerSource(
        _strSourceUri,
        sourceBaseNameForAsset(_assetSource),
        QStringLiteral("ogr"));
    QgsVectorLayer _layerTarget(
        _strTargetUri,
        sourceBaseNameForAsset(_assetTarget),
        QStringLiteral("ogr"));
    if (!_layerSource.isValid()) {
        emitFailure(_assetSource, tr("无法读取源矢量数据：%1").arg(_strSourceUri),
            QStringLiteral("spatial_query"));
        return;
    }
    if (!_layerTarget.isValid()) {
        emitFailure(_assetSource, tr("无法读取目标区域数据：%1").arg(_strTargetUri),
            QStringLiteral("spatial_query"));
        return;
    }

    const long long _lSourceTotal = qMax(0LL, _layerSource.featureCount());
    if (_lSourceTotal <= 0 || _layerTarget.featureCount() <= 0) {
        emitFailure(_assetSource, tr("空间查询要求源图层和目标区域图层都包含要素"),
            QStringLiteral("spatial_query"));
        return;
    }

    emit analysisProgress(0);

    const bool _bNeedsTransform = _layerSource.crs().isValid()
        && _layerTarget.crs().isValid()
        && _layerSource.crs() != _layerTarget.crs();
    const QgsCoordinateTransform _ctTargetToSource(
        _layerTarget.crs(),
        _layerSource.crs(),
        QgsProject::instance()->transformContext());

    QVector<QgsGeometry> _vTargetGeometries;
    int _nSkippedCount = 0;

    QgsFeature _featureTarget;
    QgsFeatureIterator _itTarget = _layerTarget.getFeatures();
    while (_itTarget.nextFeature(_featureTarget)) {
        QgsGeometry _geomTarget = _featureTarget.geometry();
        if (!transformGeometryToSourceCrs(
                _geomTarget, _ctTargetToSource, _bNeedsTransform)
            || !normalizeGeometry(_geomTarget)) {
            ++_nSkippedCount;
            continue;
        }
        _vTargetGeometries.append(_geomTarget);
    }

    if (_vTargetGeometries.isEmpty()) {
        emitFailure(_assetSource, tr("目标区域图层没有可用几何"),
            QStringLiteral("spatial_query"));
        return;
    }

    QVector<QgsFeature> _vMatchedFeatures;
    long long _lProcessedCount = 0;
    QgsFeature _featureSource;
    QgsFeatureIterator _itSource = _layerSource.getFeatures();
    while (_itSource.nextFeature(_featureSource)) {
        ++_lProcessedCount;

        QgsGeometry _geomSource = _featureSource.geometry();
        if (!normalizeGeometry(_geomSource)) {
            ++_nSkippedCount;
            continue;
        }

        bool _bMatched = false;
        for (const QgsGeometry& _geomTarget : _vTargetGeometries) {
            if (geometryMatchesRelation(_geomSource, _geomTarget, _strRelation)) {
                _bMatched = true;
                break;
            }
        }

        if (_bMatched) {
            _vMatchedFeatures.append(_featureSource);
        }

        const int _nProgress = static_cast<int>(
            qMin(95.0, 100.0 * static_cast<double>(_lProcessedCount)
                / static_cast<double>(_lSourceTotal)));
        emit analysisProgress(_nProgress);
    }

    if (_vMatchedFeatures.isEmpty()) {
        emit analysisProgress(100);
        emit analysisFinished(buildZeroMatchResult(
            _assetSource,
            QStringLiteral("spatial_query"),
            tr(
                "空间查询完成\n"
                "源数据资产：%1\n"
                "目标区域资产：%2\n"
                "空间关系：%3\n"
                "源要素数：%4\n"
                "目标区域可用几何数：%5\n"
                "命中要素数：0\n"
                "未生成高亮结果图层。")
                .arg(_assetSource.strName,
                    _assetTarget.strName,
                    relationDisplayName(_strRelation))
                .arg(_lSourceTotal)
                .arg(_vTargetGeometries.size())));
        return;
    }

    const VectorOutputTarget _targetOutput = buildSpatialOutputTarget(
        _assetSource, _assetTarget, _strRelation);
    QString _strWriteError;
    if (!writeFeaturesToSpatialite(
            _layerSource, _vMatchedFeatures, _targetOutput, _strWriteError)) {
        emitFailure(_assetSource,
            tr("写出空间查询结果失败：%1").arg(_strWriteError),
            QStringLiteral("spatial_query"));
        return;
    }

    VectorOutputTarget _targetResolved;
    QString _strResolveError;
    if (!resolveWrittenOutputTarget(
            _targetOutput, _targetResolved, _strResolveError)) {
        emitFailure(_assetSource,
            tr("空间查询结果写入后无法确认空间表：%1").arg(_strResolveError),
            QStringLiteral("spatial_query"));
        return;
    }

    emit analysisProgress(100);

    AnalysisResult _result;
    _result.strType = QStringLiteral("spatial_query");
    _result.strToolId = QStringLiteral("spatial_query");
    _result.strSourceAssetId = _assetSource.strAssetId;
    _result.strSourceAssetName = _assetSource.strName;
    _result.bSuccess = true;
    _result.bHasVisualization = false;
    _result.bHasOutputLayer = true;
    _result.strOutputPath = _targetResolved.strSourceUri;
    _result.strOutputLayerName = _targetResolved.strLayerName;
    _result.strOutputLayerType = QStringLiteral("vector");
    _result.strDesc = tr(
        "空间查询完成\n"
        "源数据资产：%1\n"
        "目标区域资产：%2\n"
        "空间关系：%3\n"
        "源要素数：%4\n"
        "目标区域可用几何数：%5\n"
        "命中要素数：%6\n"
        "跳过几何数：%7\n"
        "输出数据库：%8\n"
        "输出空间表：%9")
        .arg(_assetSource.strName,
            _assetTarget.strName,
            relationDisplayName(_strRelation))
        .arg(_lSourceTotal)
        .arg(_vTargetGeometries.size())
        .arg(_vMatchedFeatures.size())
        .arg(_nSkippedCount)
        .arg(_targetResolved.strDatabasePath,
            _targetResolved.strTableName);
    emit analysisFinished(_result);
}

void AttributeQueryService::emitFailure(const AnalysisDataAsset& _assetInput,
    const QString& _strError,
    const QString& _strToolId)
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
