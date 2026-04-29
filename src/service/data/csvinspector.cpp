#include "csvinspector.h"

#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QSet>
#include <QTextStream>

namespace
{

QString normalizeHeaderName(const QString& _strHeader)
{
    QString _strNormalized = _strHeader.trimmed().toLower();
    _strNormalized.remove(' ');
    _strNormalized.remove('_');
    _strNormalized.remove('-');
    return _strNormalized;
}

int findCoordinateColumn(const QStringList& _vHeaders,
    const QSet<QString>& _setCandidates)
{
    for (int _nColIdx = 0; _nColIdx < _vHeaders.size(); ++_nColIdx) {
        if (_setCandidates.contains(normalizeHeaderName(_vHeaders[_nColIdx]))) {
            return _nColIdx;
        }
    }
    return -1;
}

QStringList buildDefaultHeaders(int _nColCount)
{
    QStringList _vHeaders;
    for (int _nColIdx = 0; _nColIdx < _nColCount; ++_nColIdx) {
        _vHeaders.append(QObject::tr("Field_%1").arg(_nColIdx + 1));
    }
    return _vHeaders;
}

QString buildTableSummary(const QString& _strDisplayName,
    int _nRowCount,
    int _nColCount,
    const QStringList& _vNumericFields,
    bool _bHasCoordinateColumns,
    const QString& _strXField,
    const QString& _strYField)
{
    QString _strSummary = QObject::tr(
        "表格资产：%1\n"
        "数据规模：%2 行 × %3 列")
        .arg(_strDisplayName)
        .arg(_nRowCount)
        .arg(_nColCount);

    _strSummary += QObject::tr("\n数值列：%1")
        .arg(_vNumericFields.isEmpty()
            ? QObject::tr("无")
            : _vNumericFields.join(", "));

    if (_bHasCoordinateColumns) {
        _strSummary += QObject::tr("\n检测到坐标列：%1 / %2")
            .arg(_strXField, _strYField);
    }

    return _strSummary;
}

} // namespace

bool CsvInspector::inspect(const QString& _strFilePath,
    AnalysisDataAsset& _outAsset,
    QString& _strError)
{
    QFile _fiCsv(_strFilePath);
    if (!_fiCsv.open(QIODevice::ReadOnly | QIODevice::Text)) {
        _strError = QObject::tr("无法打开 CSV 文件：%1").arg(_strFilePath);
        return false;
    }

    QTextStream _stream(&_fiCsv);
    _stream.setCodec("UTF-8");

    QStringList _vLines;
    while (!_stream.atEnd()) {
        const QString _strLine = _stream.readLine().trimmed();
        if (!_strLine.isEmpty()) {
            _vLines.append(_strLine);
        }
    }

    if (_vLines.isEmpty()) {
        _strError = QObject::tr("CSV 文件为空：%1").arg(_strFilePath);
        return false;
    }

    const QStringList _vFirstTokens = splitCsvLine(_vLines.first());
    if (_vFirstTokens.isEmpty()) {
        _strError = QObject::tr("CSV 首行为空：%1").arg(_strFilePath);
        return false;
    }

    const bool _bHeaderPresent = !isNumericRow(_vFirstTokens);
    QStringList _vHeaders = _bHeaderPresent ? _vFirstTokens : buildDefaultHeaders(_vFirstTokens.size());
    const int _nDataStartLineIdx = _bHeaderPresent ? 1 : 0;
    const int _nRowCount = _vLines.size() - _nDataStartLineIdx;
    const int _nColCount = _vFirstTokens.size();
    if (_nRowCount <= 0) {
        _strError = QObject::tr("CSV 中未找到有效数据行：%1").arg(_strFilePath);
        return false;
    }

    QVector<QStringList> _vvTokens;
    _vvTokens.reserve(_nRowCount);
    for (int _nLineIdx = _nDataStartLineIdx; _nLineIdx < _vLines.size(); ++_nLineIdx) {
        const QStringList _vTokens = splitCsvLine(_vLines[_nLineIdx]);
        if (_vTokens.size() != _nColCount) {
            _strError = QObject::tr("CSV 列数不一致，无法识别：%1").arg(_strFilePath);
            return false;
        }
        _vvTokens.append(_vTokens);
    }

    QVector<bool> _vbNumericColumn(_nColCount, true);
    for (const QStringList& _vTokens : _vvTokens) {
        for (int _nColIdx = 0; _nColIdx < _nColCount; ++_nColIdx) {
            bool _bOk = false;
            _vTokens[_nColIdx].trimmed().toDouble(&_bOk);
            if (!_bOk) {
                _vbNumericColumn[_nColIdx] = false;
            }
        }
    }

    QStringList _vNumericFieldNames;
    QVector<int> _vnNumericColumnIndices;
    for (int _nColIdx = 0; _nColIdx < _nColCount; ++_nColIdx) {
        if (_vbNumericColumn[_nColIdx]) {
            _vNumericFieldNames.append(_vHeaders[_nColIdx]);
            _vnNumericColumnIndices.append(_nColIdx);
        }
    }

    const QSet<QString> _setXHeaders{
        "x", "coordx", "lon", "lng", "longitude", QString::fromUtf8("经度")
    };
    const QSet<QString> _setYHeaders{
        "y", "coordy", "lat", "latitude", QString::fromUtf8("纬度")
    };

    const int _nXColIdx = findCoordinateColumn(_vHeaders, _setXHeaders);
    const int _nYColIdx = findCoordinateColumn(_vHeaders, _setYHeaders);
    const bool _bHasCoordinateColumns =
        (_nXColIdx >= 0 && _nYColIdx >= 0
            && _vbNumericColumn[_nXColIdx]
            && _vbNumericColumn[_nYColIdx]);

    NumericDataset _dataNumeric;
    if (!_vnNumericColumnIndices.isEmpty()) {
        _dataNumeric.strSourcePath = _strFilePath;
        _dataNumeric.strName = QFileInfo(_strFilePath).fileName();
        _dataNumeric.strFormat = "csv";
        _dataNumeric.nRows = _nRowCount;
        _dataNumeric.nCols = _vnNumericColumnIndices.size();
        _dataNumeric.vdValues.reserve(_nRowCount * _vnNumericColumnIndices.size());

        for (const QStringList& _vTokens : _vvTokens) {
            for (int _nColIdx : _vnNumericColumnIndices) {
                _dataNumeric.vdValues.append(_vTokens[_nColIdx].trimmed().toDouble());
            }
        }
    }

    _outAsset = AnalysisDataAsset();
    _outAsset.strName = QFileInfo(_strFilePath).fileName();
    _outAsset.strSourcePath = _strFilePath;
    _outAsset.strSourceFormat = "csv";
    _outAsset.eAssetType = DataAssetType::Table;
    _outAsset.flagsCapabilities |= AnalysisCapability::AttributeQuery;
    _outAsset.bHasNumericDataset = !_vnNumericColumnIndices.isEmpty();
    if (_outAsset.bHasNumericDataset) {
        _outAsset.flagsCapabilities |= AnalysisCapability::Statistical;
        _outAsset.dataNumeric = _dataNumeric;
    }

    _outAsset.dataTable.nRowCount = _nRowCount;
    _outAsset.dataTable.nColCount = _nColCount;
    _outAsset.dataTable.vFieldNames = _vHeaders;
    _outAsset.dataTable.vNumericFieldNames = _vNumericFieldNames;
    _outAsset.dataTable.bHasCoordinateColumns = _bHasCoordinateColumns;
    _outAsset.dataTable.strCoordinateXField =
        (_nXColIdx >= 0) ? _vHeaders[_nXColIdx] : QString();
    _outAsset.dataTable.strCoordinateYField =
        (_nYColIdx >= 0) ? _vHeaders[_nYColIdx] : QString();
    _outAsset.dataTable.strPreviewSummary = buildTableSummary(
        _outAsset.strName,
        _nRowCount,
        _nColCount,
        _vNumericFieldNames,
        _bHasCoordinateColumns,
        _outAsset.dataTable.strCoordinateXField,
        _outAsset.dataTable.strCoordinateYField);

    _outAsset.strSummary = _outAsset.dataTable.strPreviewSummary;

    if (_bHasCoordinateColumns) {
        _outAsset.bNeedsUserChoice = true;
        _outAsset.strChoicePrompt = QObject::tr(
            "检测到坐标列“%1 / %2”，共 %3 行数据。\n"
            "请选择导入方式：作为表格资产打开，或转换为点矢量资产。")
            .arg(_outAsset.dataTable.strCoordinateXField,
                _outAsset.dataTable.strCoordinateYField)
            .arg(_nRowCount);
        _outAsset.strSummary += QObject::tr(
            "\n该 CSV 可作为表格资产使用，也可转换为点矢量资产。");
    }

    return true;
}

bool CsvInspector::buildPointAsset(const AnalysisDataAsset& _assetTable,
    AnalysisDataAsset& _outAsset,
    QString& _strError)
{
    if (_assetTable.eAssetType != DataAssetType::Table
        || !_assetTable.dataTable.bHasCoordinateColumns) {
        _strError = QObject::tr("当前表格资产不具备坐标列，无法转换为点矢量资产");
        return false;
    }

    _outAsset = AnalysisDataAsset();
    _outAsset.strName = _assetTable.strName;
    _outAsset.strSourcePath = _assetTable.strSourcePath;
    _outAsset.strSourceFormat = _assetTable.strSourceFormat;
    _outAsset.eAssetType = DataAssetType::Vector;
    _outAsset.bCanAddToMap = false;
    _outAsset.flagsCapabilities |= AnalysisCapability::SpatialVector;
    _outAsset.flagsCapabilities |= AnalysisCapability::AttributeQuery;
    _outAsset.bHasNumericDataset = _assetTable.bHasNumericDataset;
    if (_assetTable.bHasNumericDataset) {
        _outAsset.flagsCapabilities |= AnalysisCapability::Statistical;
        _outAsset.dataNumeric = _assetTable.dataNumeric;
    }

    _outAsset.dataVector.strGeometryType = "Point";
    _outAsset.dataVector.nFeatureCount = _assetTable.dataTable.nRowCount;
    _outAsset.dataVector.vFieldNames = _assetTable.dataTable.vFieldNames;
    _outAsset.dataVector.vNumericFieldNames = _assetTable.dataTable.vNumericFieldNames;
    _outAsset.dataVector.bDerivedFromTable = true;
    _outAsset.dataVector.strCoordinateXField = _assetTable.dataTable.strCoordinateXField;
    _outAsset.dataVector.strCoordinateYField = _assetTable.dataTable.strCoordinateYField;
    _outAsset.dataVector.strPreviewSummary = QObject::tr(
        "点矢量资产：%1\n"
        "来源：CSV 坐标列转换\n"
        "要素数量：%2\n"
        "坐标列：%3 / %4\n"
        "数值属性列：%5")
        .arg(_assetTable.strName)
        .arg(_assetTable.dataTable.nRowCount)
        .arg(_assetTable.dataTable.strCoordinateXField,
            _assetTable.dataTable.strCoordinateYField)
        .arg(_assetTable.dataTable.vNumericFieldNames.isEmpty()
            ? QObject::tr("无")
            : _assetTable.dataTable.vNumericFieldNames.join(", "));
    _outAsset.strSummary = _outAsset.dataVector.strPreviewSummary;
    return true;
}

QStringList CsvInspector::splitCsvLine(const QString& _strLine)
{
    if (_strLine.contains(';') && !_strLine.contains(',')) {
        return _strLine.split(';', Qt::KeepEmptyParts);
    }
    return _strLine.split(',', Qt::KeepEmptyParts);
}

bool CsvInspector::isNumericRow(const QStringList& _vTokens)
{
    if (_vTokens.isEmpty()) {
        return false;
    }

    for (const QString& _strToken : _vTokens) {
        bool _bOk = false;
        _strToken.trimmed().toDouble(&_bOk);
        if (!_bOk) {
            return false;
        }
    }
    return true;
}

QString CsvInspector::normalizeHeader(const QString& _strHeader)
{
    return normalizeHeaderName(_strHeader);
}
