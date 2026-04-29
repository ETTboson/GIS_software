// src/service/dataservice.cpp
#include "dataservice.h"

#include "dataformatrouter.h"

#include <QDebug>
#include <QFileInfo>

DataService::DataService(QObject* _pParent)
    : QObject(_pParent)
{
}

DataService::~DataService()
{
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

    if (_strExt == "shp") {
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

    const LayerInfo _layerInfo = buildLayerInfo(_strFilePath);
    mvLayers.append(_layerInfo);
    emit layerLoaded(_layerInfo);
}

void DataService::openDataForAnalysis(const QString& _strFilePath)
{
    AnalysisDataAsset _assetReady;
    QString _strError;
    if (!DataFormatRouter::routeAnalysisInput(_strFilePath, _assetReady, _strError)) {
        emit dataLoadFailed(_strError);
        return;
    }

    emit analysisAssetReady(_assetReady);
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
    for (int _nLayerIdx = 0; _nLayerIdx < mvLayers.size(); ++_nLayerIdx) {
        const LayerInfo& _layerInfoCurrent = mvLayers.at(_nLayerIdx);
        const QString _strStoredPath =
            QFileInfo(_layerInfoCurrent.strFilePath).absoluteFilePath();
        const bool _bPathMatched =
            (_strStoredPath == _strNormalizedPath)
            || _strFilePath.startsWith(_layerInfoCurrent.strFilePath)
            || _layerInfoCurrent.strFilePath.startsWith(_strFilePath);
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
    _layerInfo.strName = _fiInfo.fileName();
    _layerInfo.strType = _strType;
    _layerInfo.bVisible = true;
    return _layerInfo;
}
