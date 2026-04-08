// src/service/dataservice.cpp
#include "dataservice.h"

#include "numericdatareader.h"

#include <QFileInfo>
#include <QDebug>

DataService::DataService(QObject* _pParent)
    : QObject(_pParent)
{
}

DataService::~DataService()
{
}

void DataService::loadData(const QString& _strFilePath)
{
    if (_strFilePath.isEmpty()) {
        emit dataLoadFailed(tr("文件路径为空"));
        return;
    }

    const QFileInfo _fiInfo(_strFilePath);
    const QString _strExt = _fiInfo.suffix().toLower();
    bool _bSuccess = false;
    QString _strError;
    NumericDataset _dataSet;

    if (_strExt == "shp") {
        _bSuccess = parseShapefile(_strFilePath);
    } else if (_strExt == "tif" || _strExt == "tiff") {
        _bSuccess = parseGeoTiff(_strFilePath);
    } else if (_strExt == "geojson") {
        _bSuccess = parseGeoJson(_strFilePath);
    } else if (_strExt == "csv") {
        _bSuccess = parseCsv(_strFilePath, _dataSet, _strError);
    } else if (_strExt == "asc" || _strExt == "txt") {
        _bSuccess = parseSimpleRaster(_strFilePath, _dataSet, _strError);
    } else {
        emit dataLoadFailed(tr("不支持的格式: %1").arg(_strExt));
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
    emit dataLoaded(_layerInfo);

    if (_strExt == "csv" || _strExt == "asc" || _strExt == "txt") {
        emit numericDataLoaded(_dataSet);
    }
}

void DataService::clearAllLayers()
{
    mvLayers.clear();
}

QList<LayerInfo> DataService::getLayers() const
{
    return mvLayers;
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

bool DataService::parseCsv(const QString& _strFilePath, NumericDataset& _outDataSet,
    QString& _strError)
{
    return NumericDataReader::readCsvFile(_strFilePath, _outDataSet, _strError);
}

bool DataService::parseSimpleRaster(const QString& _strFilePath,
    NumericDataset& _outDataSet, QString& _strError)
{
    return NumericDataReader::readSimpleRasterFile(_strFilePath, _outDataSet, _strError);
}

LayerInfo DataService::buildLayerInfo(const QString& _strFilePath) const
{
    const QFileInfo _fiInfo(_strFilePath);
    const QString _strExt = _fiInfo.suffix().toLower();

    QString _strType = "vector";
    if (_strExt == "tif" || _strExt == "tiff") {
        _strType = "raster";
    } else if (_strExt == "csv") {
        _strType = "table";
    } else if (_strExt == "asc" || _strExt == "txt") {
        _strType = "simple_raster";
    }

    LayerInfo _layerInfo;
    _layerInfo.strFilePath = _strFilePath;
    _layerInfo.strName = _fiInfo.fileName();
    _layerInfo.strType = _strType;
    _layerInfo.bVisible = true;
    return _layerInfo;
}
