#include "datamodule.h"
#include <QFileInfo>
#include <QDebug>

DataModule::DataModule(QObject *_pParent)
    : QObject(_pParent)
{
}

DataModule::~DataModule()
{
    // mvstrLoadedPaths 是值类型，无需手动释放
}

// ════════════════════════════════════════════════════════
//  loadData — 根据扩展名分发到对应解析函数
// ════════════════════════════════════════════════════════
void DataModule::loadData(const QString &_strFilePath)
{
    if (_strFilePath.isEmpty()) {
        emit dataLoadFailed(tr("文件路径为空"));
        return;
    }

    QFileInfo _fiInfo(_strFilePath);
    QString   _strExt = _fiInfo.suffix().toLower();
    bool      _bSuccess = false;

    if (_strExt == "shp") {
        _bSuccess = parseShapefile(_strFilePath);
    } else if (_strExt == "tif" || _strExt == "tiff") {
        _bSuccess = parseGeoTiff(_strFilePath);
    } else if (_strExt == "geojson") {
        _bSuccess = parseGeoJson(_strFilePath);
    } else {
        emit dataLoadFailed(tr("不支持的格式: %1").arg(_strExt));
        return;
    }

    if (_bSuccess) {
        mvstrLoadedPaths.append(_strFilePath);
        emit dataLoaded(_strFilePath);
    } else {
        emit dataLoadFailed(tr("解析失败: %1").arg(_strFilePath));
    }
}

void DataModule::clearAllLayers()
{
    mvstrLoadedPaths.clear();
}

// ── 私有解析函数（骨架，后续接入 GDAL/OGR）──────────────
bool DataModule::parseShapefile(const QString &_strFilePath)
{
    // TODO: 接入 OGR/GDAL 读取 Shapefile
    qDebug() << "[DataModule] parseShapefile:" << _strFilePath;
    return true;  // 临时返回成功
}

bool DataModule::parseGeoTiff(const QString &_strFilePath)
{
    // TODO: 接入 GDAL 读取 GeoTIFF
    qDebug() << "[DataModule] parseGeoTiff:" << _strFilePath;
    return true;
}

bool DataModule::parseGeoJson(const QString &_strFilePath)
{
    // TODO: 接入 OGR 读取 GeoJSON
    qDebug() << "[DataModule] parseGeoJson:" << _strFilePath;
    return true;
}
