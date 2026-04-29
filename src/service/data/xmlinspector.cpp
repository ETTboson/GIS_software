#include "xmlinspector.h"

#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QTextStream>
#include <QXmlStreamReader>

namespace
{

bool containsCoordinatePattern(const QString& _strContentLower)
{
    return (_strContentLower.contains("<x>") && _strContentLower.contains("<y>"))
        || (_strContentLower.contains("<lon>") && _strContentLower.contains("<lat>"))
        || (_strContentLower.contains("<longitude>") && _strContentLower.contains("<latitude>"))
        || (_strContentLower.contains(" x=\"") && _strContentLower.contains(" y=\""))
        || (_strContentLower.contains(" lon=\"") && _strContentLower.contains(" lat=\""))
        || (_strContentLower.contains(" longitude=\"") && _strContentLower.contains(" latitude=\""));
}

QString buildUnsupportedXmlHint(const QString& _strRootName,
    const QString& _strPreview)
{
    return QObject::tr(
        "暂不支持该 XML 结构。\n"
        "根节点：%1\n"
        "结构摘要：%2")
        .arg(_strRootName, _strPreview);
}

} // namespace

bool XmlInspector::inspect(const QString& _strFilePath,
    AnalysisDataAsset& _outAsset,
    QString& _strError)
{
    QFile _fiXml(_strFilePath);
    if (!_fiXml.open(QIODevice::ReadOnly | QIODevice::Text)) {
        _strError = QObject::tr("无法打开 XML 文件：%1").arg(_strFilePath);
        return false;
    }

    QTextStream _stream(&_fiXml);
    _stream.setCodec("UTF-8");
    const QString _strContent = _stream.readAll();
    if (_strContent.trimmed().isEmpty()) {
        _strError = QObject::tr("XML 文件为空：%1").arg(_strFilePath);
        return false;
    }

    QXmlStreamReader _xmlReader(_strContent);
    QString _strRootName;
    QStringList _vElementNames;
    while (!_xmlReader.atEnd()) {
        _xmlReader.readNext();
        if (_xmlReader.isStartElement()) {
            const QString _strElementName = _xmlReader.name().toString();
            if (_strRootName.isEmpty()) {
                _strRootName = _strElementName;
            }
            if (_vElementNames.size() < 8) {
                _vElementNames.append(_strElementName);
            }
        }
    }

    if (_xmlReader.hasError() || _strRootName.isEmpty()) {
        _strError = QObject::tr("未检测到有效 XML 根节点：%1").arg(_strFilePath);
        return false;
    }

    const QString _strPreview = _vElementNames.join(" -> ");
    const QString _strContentLower = _strContent.toLower();

    _outAsset = AnalysisDataAsset();
    _outAsset.strName = QFileInfo(_strFilePath).fileName();
    _outAsset.strSourcePath = _strFilePath;
    _outAsset.strSourceFormat = "xml";

    if (containsCoordinatePattern(_strContentLower)) {
        _outAsset.eAssetType = DataAssetType::Vector;
        _outAsset.flagsCapabilities |= AnalysisCapability::SpatialVector;
        _outAsset.flagsCapabilities |= AnalysisCapability::AttributeQuery;
        _outAsset.dataVector.strGeometryType = "Point";
        _outAsset.dataVector.strPreviewSummary = QObject::tr(
            "矢量资产：%1\n"
            "来源：XML 坐标结构\n"
            "根节点：%2\n"
            "结构摘要：%3")
            .arg(_outAsset.strName, _strRootName, _strPreview);
        _outAsset.strSummary = _outAsset.dataVector.strPreviewSummary;
        return true;
    }

    const bool _bLooksLikeMetadata =
        _strContentLower.contains("<metadata")
        || _strContentLower.contains("<property")
        || _strContentLower.contains("<item")
        || _strContentLower.contains("<field")
        || _strContentLower.contains("<value");

    if (_bLooksLikeMetadata) {
        _outAsset.eAssetType = DataAssetType::MetadataDocument;
        _outAsset.dataMetadata.strRootName = _strRootName;
        _outAsset.dataMetadata.vSummaryLines = QStringList{
            QObject::tr("根节点：%1").arg(_strRootName),
            QObject::tr("结构摘要：%1").arg(_strPreview)
        };
        _outAsset.dataMetadata.strPreviewSummary = QObject::tr(
            "元数据文档：%1\n"
            "根节点：%2\n"
            "结构摘要：%3\n"
            "当前版本仅提供文档摘要与后续转换入口。")
            .arg(_outAsset.strName, _strRootName, _strPreview);
        _outAsset.strSummary = _outAsset.dataMetadata.strPreviewSummary;
        return true;
    }

    _strError = buildUnsupportedXmlHint(_strRootName, _strPreview);
    return false;
}
