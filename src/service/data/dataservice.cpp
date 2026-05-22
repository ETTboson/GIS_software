// src/service/dataservice.cpp
#include "dataservice.h"

#include "dataformatrouter.h"
#include "spatialdatabaseservice.h"

#include <QDebug>
#include <QFileInfo>

DataService::DataService(QObject* _pParent)
    : QObject(_pParent)
    , mpSpatialDatabaseService(new SpatialDatabaseService())
{
}

DataService::~DataService()
{
    delete mpSpatialDatabaseService;
    mpSpatialDatabaseService = nullptr;
}

void DataService::loadLayerToMap(const QString& _strFilePath)
{
    if (_strFilePath.isEmpty()) {
        emit dataLoadFailed(tr("文件路径为空"));
        return;
    }

    const QFileInfo _fiInfo(_strFilePath);
    const QString _strExt = _fiInfo.suffix().toLower();
    if (!DataFormatRouter::canLoadAsMapLayer(_strFilePath)) {
        emit dataLoadFailed(tr("添加图层仅支持矢量/栅格图层格式：%1").arg(_strExt));
        return;
    }

    bool _bSuccess = false;
    QString _strError;
    QString _strDatabasePath;
    QString _strTableName;

    if (SpatialDatabaseService::parseLayerSourceUri(
        _strFilePath, _strDatabasePath, _strTableName)) {
        _bSuccess = parseSpatialDatabaseTable(
            _strDatabasePath, _strTableName, _strError);
    } else if (SpatialDatabaseService::isSupportedDatabasePath(_strFilePath)) {
        _bSuccess = parseSpatialDatabase(_strFilePath, _strError);
    } else if (_strExt == "shp") {
        _bSuccess = parseShapefile(_strFilePath);
    } else if (_strExt == "tif" || _strExt == "tiff" || _strExt == "img") {
        _bSuccess = parseGeoTiff(_strFilePath);
    } else if (_strExt == "geojson") {
        _bSuccess = parseGeoJson(_strFilePath);
    } else {
        emit dataLoadFailed(tr("当前不支持的地图图层格式：%1").arg(_strExt));
        return;
    }

    if (!_bSuccess) {
        emit dataLoadFailed(_strError.isEmpty()
            ? tr("解析失败: %1").arg(_strFilePath)
            : _strError);
        return;
    }

    if (SpatialDatabaseService::isSupportedDatabasePath(_strFilePath)) {
        return;
    }

    publishLayer(buildLayerInfo(_strFilePath), false);
}

void DataService::loadAnalysisResultLayer(const AnalysisResult& _resultInput)
{
    if (!_resultInput.bHasOutputLayer
        || _resultInput.strOutputPath.trimmed().isEmpty()) {
        emit dataLoadFailed(tr("当前分析结果没有可加载的输出图层"));
        return;
    }

    publishLayer(buildLayerInfo(_resultInput), false);
}

void DataService::openDataForAnalysis(const QString& _strFilePath)
{
    QList<AnalysisDataAsset> _vAssetsReady;
    QString _strError;
    if (!DataFormatRouter::routeAnalysisInputs(
        _strFilePath, _vAssetsReady, _strError)) {
        emit dataLoadFailed(_strError);
        return;
    }

    for (const AnalysisDataAsset& _assetReady : _vAssetsReady) {
        emit analysisAssetReady(_assetReady);

        const QString _strVectorSourceUri =
            _assetReady.dataVector.strSourceUri.trimmed();
        const QString _strMapSource = _strVectorSourceUri.isEmpty()
            ? _assetReady.strSourcePath
            : _strVectorSourceUri;
        if (_assetReady.bCanAddToMap
            && DataFormatRouter::canLoadAsMapLayer(_strMapSource)) {
            publishLayer(buildLayerInfo(_assetReady), true);
        }
    }
}

bool DataService::buildAlternateAsset(const AnalysisDataAsset& _assetInput,
    DataAssetType _eTargetType,
    AnalysisDataAsset& _outAsset,
    QString& _strError) const
{
    return DataFormatRouter::buildAlternateAsset(
        _assetInput, _eTargetType, _outAsset, _strError);
}

void DataService::clearAllLayers()
{
    mvLayers.clear();
}

QList<LayerInfo> DataService::getLayers() const
{
    return mvLayers;
}

void DataService::removeLayerRecord(const QString& _strFilePath,
    const QString& _strLayerName)
{
    const QString _strNormalizedPath = QFileInfo(_strFilePath).absoluteFilePath();
    QString _strDatabasePath;
    QString _strTableName;
    const bool _bInputIsDatabaseUri = SpatialDatabaseService::parseLayerSourceUri(
        _strFilePath, _strDatabasePath, _strTableName);
    for (int _nLayerIdx = 0; _nLayerIdx < mvLayers.size(); ++_nLayerIdx) {
        const LayerInfo& _layerInfoCurrent = mvLayers.at(_nLayerIdx);
        const QString _strStoredPath =
            QFileInfo(_layerInfoCurrent.strFilePath).absoluteFilePath();
        const QString _strStoredUri = _layerInfoCurrent.strSourceUri.trimmed();
        const bool _bPathMatched =
            (_strStoredPath == _strNormalizedPath)
            || (!_strStoredUri.isEmpty() && _strStoredUri == _strFilePath)
            || _strFilePath.startsWith(_layerInfoCurrent.strFilePath)
            || _layerInfoCurrent.strFilePath.startsWith(_strFilePath)
            || (_bInputIsDatabaseUri
                && _layerInfoCurrent.strDatabasePath == _strDatabasePath
                && _layerInfoCurrent.strTableName == _strTableName);
        if (_bPathMatched
            && _layerInfoCurrent.strName == _strLayerName) {
            mvLayers.removeAt(_nLayerIdx);
            return;
        }
    }
}

bool DataService::parseShapefile(const QString& _strFilePath)
{
    qDebug() << "[DataService] parseShapefile:" << _strFilePath;
    return true;
}

bool DataService::parseGeoTiff(const QString& _strFilePath)
{
    qDebug() << "[DataService] parseGeoTiff:" << _strFilePath;
    return true;
}

bool DataService::parseGeoJson(const QString& _strFilePath)
{
    qDebug() << "[DataService] parseGeoJson:" << _strFilePath;
    return true;
}

bool DataService::parseSpatialDatabase(const QString& _strDatabasePath,
    QString& _strError)
{
    const QList<SpatialTableInfo> _vTables =
        mpSpatialDatabaseService->listSpatialTables(_strDatabasePath, _strError);
    if (!_strError.trimmed().isEmpty()) {
        return false;
    }

    if (_vTables.isEmpty()) {
        _strError = tr("空间数据库中没有可加载的空间表：%1").arg(_strDatabasePath);
        return false;
    }

    for (const SpatialTableInfo& _tableInfo : _vTables) {
        publishLayer(buildLayerInfo(_tableInfo), false);
    }
    return true;
}

bool DataService::parseSpatialDatabaseTable(const QString& _strDatabasePath,
    const QString& _strTableName,
    QString& _strError)
{
    SpatialTableInfo _tableInfo;
    if (!mpSpatialDatabaseService->getSpatialTableInfo(
        _strDatabasePath, _strTableName, _tableInfo, _strError)) {
        return false;
    }

    publishLayer(buildLayerInfo(_tableInfo), false);
    return true;
}

LayerInfo DataService::buildLayerInfo(const QString& _strFilePath) const
{
    const QFileInfo _fiInfo(_strFilePath);
    const QString _strExt = _fiInfo.suffix().toLower();

    QString _strType = "vector";
    if (_strExt == "tif" || _strExt == "tiff" || _strExt == "img") {
        _strType = "raster";
    }

    LayerInfo _layerInfo;
    _layerInfo.strFilePath = _strFilePath;
    _layerInfo.strSourceUri = _strFilePath;
    _layerInfo.strProviderKey = (_strType == "raster")
        ? QStringLiteral("gdal")
        : QStringLiteral("ogr");
    _layerInfo.strName = _fiInfo.fileName();
    _layerInfo.strType = _strType;
    _layerInfo.bVisible = true;
    return _layerInfo;
}

LayerInfo DataService::buildLayerInfo(const AnalysisDataAsset& _assetInput) const
{
    if (_assetInput.eAssetType == DataAssetType::Vector
        && !_assetInput.dataVector.strDatabasePath.trimmed().isEmpty()) {
        LayerInfo _layerInfo;
        _layerInfo.strFilePath = _assetInput.dataVector.strDatabasePath;
        _layerInfo.strSourceUri = _assetInput.dataVector.strSourceUri.trimmed().isEmpty()
            ? SpatialDatabaseService::buildLayerSourceUri(
                _assetInput.dataVector.strDatabasePath,
                _assetInput.dataVector.strTableName)
            : _assetInput.dataVector.strSourceUri;
        _layerInfo.strProviderKey = _assetInput.dataVector.strProviderKey.trimmed().isEmpty()
            ? QStringLiteral("ogr")
            : _assetInput.dataVector.strProviderKey;
        _layerInfo.strDatabasePath = _assetInput.dataVector.strDatabasePath;
        _layerInfo.strTableName = _assetInput.dataVector.strTableName;
        _layerInfo.strGeometryColumn = _assetInput.dataVector.strGeometryColumn;
        _layerInfo.strName = _assetInput.dataVector.strTableName.trimmed().isEmpty()
            ? _assetInput.strName
            : _assetInput.dataVector.strTableName;
        _layerInfo.strType = QStringLiteral("vector");
        _layerInfo.bVisible = true;
        return _layerInfo;
    }

    LayerInfo _layerInfo = buildLayerInfo(_assetInput.strSourcePath);
    if (_assetInput.eAssetType == DataAssetType::Vector
        && !_assetInput.dataVector.strSourceUri.trimmed().isEmpty()) {
        _layerInfo.strSourceUri = _assetInput.dataVector.strSourceUri;
        _layerInfo.strProviderKey = _assetInput.dataVector.strProviderKey
            .trimmed().isEmpty()
            ? QStringLiteral("ogr")
            : _assetInput.dataVector.strProviderKey;
        _layerInfo.strType = QStringLiteral("vector");
    }
    if (!_assetInput.strName.trimmed().isEmpty()) {
        _layerInfo.strName = _assetInput.strName;
    }
    return _layerInfo;
}

LayerInfo DataService::buildLayerInfo(const AnalysisResult& _resultInput) const
{
    const QString _strSourceUri = _resultInput.strOutputPath.trimmed();
    QString _strDatabasePath;
    QString _strTableName;

    LayerInfo _layerInfo;
    _layerInfo.strSourceUri = _strSourceUri;
    _layerInfo.strProviderKey = QStringLiteral("ogr");
    _layerInfo.strType = _resultInput.strOutputLayerType.trimmed().isEmpty()
        ? QStringLiteral("vector")
        : _resultInput.strOutputLayerType;
    _layerInfo.strName = _resultInput.strOutputLayerName.trimmed().isEmpty()
        ? QFileInfo(_strSourceUri).completeBaseName()
        : _resultInput.strOutputLayerName;
    _layerInfo.bVisible = true;
    _layerInfo.bIsAnalysisResult = true;
    _layerInfo.bUseHighlightStyle = _resultInput.strToolId == QStringLiteral("attribute_query")
        || _resultInput.strToolId == QStringLiteral("spatial_query")
        || _resultInput.strToolId == QStringLiteral("proximity_query");

    if (SpatialDatabaseService::parseLayerSourceUri(
            _strSourceUri, _strDatabasePath, _strTableName)) {
        _layerInfo.strFilePath = _strDatabasePath;
        _layerInfo.strDatabasePath = _strDatabasePath;
        _layerInfo.strTableName = _strTableName;
    } else {
        _layerInfo.strFilePath = _strSourceUri;
    }

    return _layerInfo;
}

LayerInfo DataService::buildLayerInfo(const SpatialTableInfo& _tableInfo) const
{
    LayerInfo _layerInfo;
    _layerInfo.strFilePath = _tableInfo.strDatabasePath;
    _layerInfo.strSourceUri = _tableInfo.strSourceUri;
    _layerInfo.strProviderKey = _tableInfo.strProviderKey.isEmpty()
        ? QStringLiteral("ogr")
        : _tableInfo.strProviderKey;
    _layerInfo.strDatabasePath = _tableInfo.strDatabasePath;
    _layerInfo.strTableName = _tableInfo.strTableName;
    _layerInfo.strGeometryColumn = _tableInfo.strGeometryColumn;
    _layerInfo.strName = _tableInfo.strTableName;
    _layerInfo.strType = QStringLiteral("vector");
    _layerInfo.bVisible = true;
    return _layerInfo;
}

bool DataService::publishLayer(const LayerInfo& _layerInfo,
    bool _bSkipExisting)
{
    if (_bSkipExisting && hasLayerRecord(_layerInfo)) {
        return false;
    }

    mvLayers.append(_layerInfo);
    emit layerLoaded(_layerInfo);
    return true;
}

bool DataService::hasLayerRecord(const LayerInfo& _layerInfo) const
{
    const QString _strTargetKey = BuildLayerKey(_layerInfo);
    for (const LayerInfo& _layerInfoCurrent : mvLayers) {
        if (BuildLayerKey(_layerInfoCurrent) == _strTargetKey) {
            return true;
        }
    }
    return false;
}

QString DataService::BuildLayerKey(const LayerInfo& _layerInfo)
{
    const QString _strSource = _layerInfo.strSourceUri.trimmed().isEmpty()
        ? _layerInfo.strFilePath
        : _layerInfo.strSourceUri;

    QString _strDatabasePath;
    QString _strTableName;
    if (SpatialDatabaseService::parseLayerSourceUri(
            _strSource, _strDatabasePath, _strTableName)) {
        return QStringLiteral("%1|layername=%2")
            .arg(QFileInfo(_strDatabasePath).absoluteFilePath().toLower(),
                _strTableName.trimmed().toLower());
    }

    return QFileInfo(_strSource).absoluteFilePath().toLower();
}
