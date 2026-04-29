#include "dataformatrouter.h"

#include "csvinspector.h"
#include "numericdatareader.h"
#include "xmlinspector.h"

#include <QFileInfo>
#include <QObject>
#include <QMetaType>


#include <qgsfeature.h>
#include <qgsfeatureiterator.h>
#include <qgsfield.h>
#include <qgsfields.h>
#include <qgsvectorlayer.h>
#include <qgswkbtypes.h>

namespace
{

bool isNumericVariantType(QMetaType::Type _eType)
{
    return _eType == QMetaType::Int
        || _eType == QMetaType::UInt
        || _eType == QMetaType::LongLong
        || _eType == QMetaType::ULongLong
        || _eType == QMetaType::Double;
}

QString buildVectorSummary(const AnalysisDataAsset& _assetVector)
{
    return QObject::tr(
        "矢量资产：%1\n"
        "几何类型：%2\n"
        "要素数量：%3\n"
        "字段数量：%4\n"
        "数值字段：%5")
        .arg(_assetVector.strName)
        .arg(_assetVector.dataVector.strGeometryType)
        .arg(_assetVector.dataVector.nFeatureCount)
        .arg(_assetVector.dataVector.vFieldNames.size())
        .arg(_assetVector.dataVector.vNumericFieldNames.isEmpty()
            ? QObject::tr("无")
            : _assetVector.dataVector.vNumericFieldNames.join(", "));
}

bool buildVectorAsset(const QString& _strFilePath,
    AnalysisDataAsset& _outAsset,
    QString& _strError)
{
    QgsVectorLayer _layerVector(
        _strFilePath,
        QFileInfo(_strFilePath).baseName(),
        QStringLiteral("ogr"));
    if (!_layerVector.isValid()) {
        _strError = QObject::tr("无法读取矢量数据：%1").arg(_strFilePath);
        return false;
    }

    _outAsset = AnalysisDataAsset();
    _outAsset.strName = QFileInfo(_strFilePath).fileName();
    _outAsset.strSourcePath = _strFilePath;
    _outAsset.strSourceFormat = QFileInfo(_strFilePath).suffix().toLower();
    _outAsset.eAssetType = DataAssetType::Vector;
    _outAsset.bCanAddToMap = true;
    _outAsset.flagsCapabilities |= AnalysisCapability::SpatialVector;
    _outAsset.flagsCapabilities |= AnalysisCapability::AttributeQuery;
    _outAsset.dataVector.strGeometryType = QgsWkbTypes::displayString(_layerVector.wkbType());
    _outAsset.dataVector.nFeatureCount = static_cast<int>(_layerVector.featureCount());

    const QgsFields _fields = _layerVector.fields();
    QVector<int> _vnNumericFieldIndices;
    QStringList _vNumericFieldNames;
    for (int _nFieldIdx = 0; _nFieldIdx < _fields.size(); ++_nFieldIdx) {
        const QgsField _fieldCurrent = _fields.at(_nFieldIdx);
        _outAsset.dataVector.vFieldNames.append(_fieldCurrent.name());
        if (isNumericVariantType(_fieldCurrent.type())) {
            _vnNumericFieldIndices.append(_nFieldIdx);
            _vNumericFieldNames.append(_fieldCurrent.name());
        }
    }
    _outAsset.dataVector.vNumericFieldNames = _vNumericFieldNames;

    if (!_vnNumericFieldIndices.isEmpty() && _outAsset.dataVector.nFeatureCount > 0) {
        _outAsset.bHasNumericDataset = true;
        _outAsset.flagsCapabilities |= AnalysisCapability::Statistical;
        _outAsset.dataNumeric.strSourcePath = _strFilePath;
        _outAsset.dataNumeric.strName = QFileInfo(_strFilePath).fileName();
        _outAsset.dataNumeric.strFormat = _outAsset.strSourceFormat;
        _outAsset.dataNumeric.nRows = _outAsset.dataVector.nFeatureCount;
        _outAsset.dataNumeric.nCols = _vnNumericFieldIndices.size();
        _outAsset.dataNumeric.vdValues.reserve(
            _outAsset.dataNumeric.nRows * _outAsset.dataNumeric.nCols);

        QgsFeature _featureCurrent;
        QgsFeatureIterator _itFeature = _layerVector.getFeatures();
        while (_itFeature.nextFeature(_featureCurrent)) {
            for (int _nFieldIdx : _vnNumericFieldIndices) {
                const QVariant _valueCurrent = _featureCurrent.attribute(_nFieldIdx);
                bool _bOk = false;
                double _dValue = _valueCurrent.toDouble(&_bOk);
                _outAsset.dataNumeric.vdValues.append(_bOk ? _dValue : 0.0);
            }
        }
    }

    _outAsset.dataVector.strPreviewSummary = buildVectorSummary(_outAsset);
    _outAsset.strSummary = _outAsset.dataVector.strPreviewSummary;
    return true;
}

bool buildRasterAsset(const QString& _strFilePath,
    AnalysisDataAsset& _outAsset,
    QString& _strError)
{
    const QString _strExt = QFileInfo(_strFilePath).suffix().toLower();

    NumericDataset _dataNumeric;
    int _nBandCount = 0;
    bool _bHasNoDataValue = false;
    double _dNoDataValue = 0.0;
    bool _bSuccess = false;

    if (_strExt == "asc" || _strExt == "txt") {
        _bSuccess = NumericDataReader::readSimpleRasterFile(
            _strFilePath, _dataNumeric, _strError);
        _nBandCount = 1;
    } else {
        _bSuccess = NumericDataReader::readRasterFile(
            _strFilePath,
            _dataNumeric,
            &_nBandCount,
            &_bHasNoDataValue,
            &_dNoDataValue,
            _strError);
    }

    if (!_bSuccess) {
        return false;
    }

    _outAsset = AnalysisDataAsset();
    _outAsset.strName = QFileInfo(_strFilePath).fileName();
    _outAsset.strSourcePath = _strFilePath;
    _outAsset.strSourceFormat = _strExt;
    _outAsset.eAssetType = DataAssetType::Raster;
    _outAsset.bCanAddToMap = true;
    _outAsset.bHasNumericDataset = true;
    _outAsset.flagsCapabilities |= AnalysisCapability::SpatialRaster;
    _outAsset.flagsCapabilities |= AnalysisCapability::Statistical;
    _outAsset.dataNumeric = _dataNumeric;
    _outAsset.dataRaster.strFormat = _strExt;
    _outAsset.dataRaster.nRows = _dataNumeric.nRows;
    _outAsset.dataRaster.nCols = _dataNumeric.nCols;
    _outAsset.dataRaster.nBandCount = _nBandCount;
    _outAsset.dataRaster.bHasNoDataValue = _bHasNoDataValue;
    _outAsset.dataRaster.dNoDataValue = _dNoDataValue;
    _outAsset.dataRaster.strPreviewSummary = QObject::tr(
        "栅格资产：%1\n"
        "尺寸：%2 行 × %3 列\n"
        "波段数：%4\n"
        "像元值分析：已准备")
        .arg(_outAsset.strName)
        .arg(_outAsset.dataRaster.nRows)
        .arg(_outAsset.dataRaster.nCols)
        .arg(_outAsset.dataRaster.nBandCount);
    if (_outAsset.dataRaster.bHasNoDataValue) {
        _outAsset.dataRaster.strPreviewSummary += QObject::tr("\nNoData：%1")
            .arg(_outAsset.dataRaster.dNoDataValue, 0, 'f', 6);
    }
    _outAsset.strSummary = _outAsset.dataRaster.strPreviewSummary;
    return true;
}

} // namespace

bool DataFormatRouter::canLoadAsMapLayer(const QString& _strFilePath)
{
    const QString _strExt = QFileInfo(_strFilePath).suffix().toLower();
    return _strExt == "shp"
        || _strExt == "geojson"
        || _strExt == "tif"
        || _strExt == "tiff"
        || _strExt == "img";
}

bool DataFormatRouter::routeAnalysisInput(const QString& _strFilePath,
    AnalysisDataAsset& _outAsset,
    QString& _strError)
{
    if (_strFilePath.trimmed().isEmpty()) {
        _strError = QObject::tr("文件路径为空");
        return false;
    }

    const QString _strExt = QFileInfo(_strFilePath).suffix().toLower();
    if (_strExt == "csv") {
        return CsvInspector::inspect(_strFilePath, _outAsset, _strError);
    }
    if (_strExt == "xml") {
        return XmlInspector::inspect(_strFilePath, _outAsset, _strError);
    }
    if (_strExt == "asc" || _strExt == "txt"
        || _strExt == "tif" || _strExt == "tiff" || _strExt == "img") {
        return buildRasterAsset(_strFilePath, _outAsset, _strError);
    }
    if (_strExt == "shp" || _strExt == "geojson") {
        return buildVectorAsset(_strFilePath, _outAsset, _strError);
    }

    _strError = QObject::tr("当前版本暂不支持该分析输入格式：%1").arg(_strExt);
    return false;
}

bool DataFormatRouter::buildAlternateAsset(const AnalysisDataAsset& _assetInput,
    DataAssetType _eTargetType,
    AnalysisDataAsset& _outAsset,
    QString& _strError)
{
    if (_assetInput.eAssetType == _eTargetType) {
        _outAsset = _assetInput;
        return true;
    }

    if (_assetInput.eAssetType == DataAssetType::Table
        && _eTargetType == DataAssetType::Vector) {
        return CsvInspector::buildPointAsset(_assetInput, _outAsset, _strError);
    }

    _strError = QObject::tr("当前资产暂不支持转换到目标类型");
    return false;
}
