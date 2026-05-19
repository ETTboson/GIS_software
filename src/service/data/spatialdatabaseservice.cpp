#include "spatialdatabaseservice.h"

#include <memory>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include <cpl_error.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>

#include <qgsfeature.h>
#include <qgsfeatureiterator.h>
#include <qgsproject.h>
#include <qgsvectorlayer.h>

namespace
{

bool isNumericOgrField(OGRFieldType _eFieldType)
{
    return _eFieldType == OFTInteger
        || _eFieldType == OFTInteger64
        || _eFieldType == OFTReal;
}

QString safeUtf8(const char* _pszValue)
{
    return (_pszValue == nullptr)
        ? QString()
        : QString::fromUtf8(_pszValue);
}

QString spatialRefAuthId(const OGRSpatialReference* _poSrs)
{
    if (_poSrs == nullptr) {
        return QString();
    }

    const char* _pszAuthorityName = _poSrs->GetAuthorityName(nullptr);
    const char* _pszAuthorityCode = _poSrs->GetAuthorityCode(nullptr);
    if (_pszAuthorityName == nullptr || _pszAuthorityCode == nullptr) {
        return QString();
    }
    return QStringLiteral("%1:%2")
        .arg(QString::fromUtf8(_pszAuthorityName),
            QString::fromUtf8(_pszAuthorityCode));
}

QVariant featureFieldValue(OGRFeature* _poFeature,
    const OGRFieldDefn* _poFieldDefn,
    int _nFieldIdx)
{
    if (_poFeature == nullptr || _poFieldDefn == nullptr
        || !_poFeature->IsFieldSetAndNotNull(_nFieldIdx)) {
        return QVariant();
    }

    switch (_poFieldDefn->GetType()) {
    case OFTInteger:
        return QVariant(_poFeature->GetFieldAsInteger(_nFieldIdx));
    case OFTInteger64:
        return QVariant(static_cast<qlonglong>(
            _poFeature->GetFieldAsInteger64(_nFieldIdx)));
    case OFTReal:
        return QVariant(_poFeature->GetFieldAsDouble(_nFieldIdx));
    default:
        return QString::fromUtf8(_poFeature->GetFieldAsString(_nFieldIdx));
    }
}

QVariantMap featureToVariantMap(OGRFeature* _poFeature)
{
    QVariantMap _mapFeature;
    if (_poFeature == nullptr) {
        return _mapFeature;
    }

    const OGRFeatureDefn* _poDefn = _poFeature->GetDefnRef();
    if (_poDefn == nullptr) {
        return _mapFeature;
    }

    for (int _nFieldIdx = 0; _nFieldIdx < _poDefn->GetFieldCount();
        ++_nFieldIdx) {
        const OGRFieldDefn* _poFieldDefn = _poDefn->GetFieldDefn(_nFieldIdx);
        if (_poFieldDefn == nullptr) {
            continue;
        }

        _mapFeature.insert(
            QString::fromUtf8(_poFieldDefn->GetNameRef()),
            featureFieldValue(_poFeature, _poFieldDefn, _nFieldIdx));
    }
    return _mapFeature;
}

QVector<QVariantMap> readFeatureRows(OGRLayer* _poLayer, int _nLimit)
{
    QVector<QVariantMap> _vRows;
    if (_poLayer == nullptr) {
        return _vRows;
    }

    _poLayer->ResetReading();
    int _nReadCount = 0;
    OGRFeature* _poFeature = nullptr;
    while ((_poFeature = _poLayer->GetNextFeature()) != nullptr) {
        _vRows.append(featureToVariantMap(_poFeature));
        OGRFeature::DestroyFeature(_poFeature);

        ++_nReadCount;
        if (_nLimit > 0 && _nReadCount >= _nLimit) {
            break;
        }
    }
    return _vRows;
}

QStringList resultFieldNames(OGRLayer* _poLayer)
{
    QStringList _vFieldNames;
    if (_poLayer == nullptr || _poLayer->GetLayerDefn() == nullptr) {
        return _vFieldNames;
    }

    const OGRFeatureDefn* _poDefn = _poLayer->GetLayerDefn();
    for (int _nFieldIdx = 0; _nFieldIdx < _poDefn->GetFieldCount();
        ++_nFieldIdx) {
        const OGRFieldDefn* _poFieldDefn = _poDefn->GetFieldDefn(_nFieldIdx);
        if (_poFieldDefn != nullptr) {
            _vFieldNames.append(QString::fromUtf8(_poFieldDefn->GetNameRef()));
        }
    }
    return _vFieldNames;
}

QStringList featureToStringRow(OGRFeature* _poFeature,
    const QStringList& _vFieldNames)
{
    QStringList _vRow;
    if (_poFeature == nullptr) {
        return _vRow;
    }

    for (int _nFieldIdx = 0; _nFieldIdx < _vFieldNames.size(); ++_nFieldIdx) {
        if (!_poFeature->IsFieldSetAndNotNull(_nFieldIdx)) {
            _vRow.append(QString());
        } else {
            _vRow.append(QString::fromUtf8(
                _poFeature->GetFieldAsString(_nFieldIdx)));
        }
    }
    return _vRow;
}

} // namespace

SpatialDatabaseService::SpatialDatabaseService()
{
}

SpatialDatabaseService::~SpatialDatabaseService()
{
    closeAllConnections();
}

bool SpatialDatabaseService::openConnection(const QString& _strDatabasePath,
    bool _bReadOnly,
    QString& _strError)
{
    const QString _strKey = NormalizeDatabasePath(_strDatabasePath);
    if (_strKey.trimmed().isEmpty()) {
        _strError = QStringLiteral("空间数据库路径为空");
        return false;
    }

    closeConnection(_strKey);

    SpatialDatabaseConnection* _pConnection = new SpatialDatabaseConnection();
    if (!_pConnection->open(_strKey, _bReadOnly, _strError)) {
        delete _pConnection;
        return false;
    }

    mmapConnections.insert(_strKey, _pConnection);
    return true;
}

void SpatialDatabaseService::closeConnection(const QString& _strDatabasePath)
{
    const QString _strKey = NormalizeDatabasePath(_strDatabasePath);
    SpatialDatabaseConnection* _pConnection = mmapConnections.take(_strKey);
    delete _pConnection;
}

void SpatialDatabaseService::closeAllConnections()
{
    qDeleteAll(mmapConnections);
    mmapConnections.clear();
}

bool SpatialDatabaseService::hasConnection(const QString& _strDatabasePath) const
{
    SpatialDatabaseConnection* _pConnection = connection(_strDatabasePath);
    return _pConnection != nullptr && _pConnection->isOpen();
}

SpatialDatabaseConnection* SpatialDatabaseService::connection(
    const QString& _strDatabasePath) const
{
    return mmapConnections.value(NormalizeDatabasePath(_strDatabasePath), nullptr);
}

QList<SpatialTableInfo> SpatialDatabaseService::listSpatialTables(
    const QString& _strDatabasePath,
    QString& _strError) const
{
    SpatialDatabaseConnection _connectionTemp;
    if (!_connectionTemp.open(_strDatabasePath, true, _strError)) {
        return QList<SpatialTableInfo>();
    }

    return listSpatialTables(_connectionTemp, _strError);
}

QList<SpatialTableInfo> SpatialDatabaseService::listSpatialTables(
    SpatialDatabaseConnection& _connection,
    QString& _strError) const
{
    QList<SpatialTableInfo> _vTables;
    _strError.clear();

    if (!_connection.isOpen() || _connection.dataset() == nullptr) {
        _strError = QStringLiteral("空间数据库连接未打开");
        return _vTables;
    }

    GDALDataset* _poDataset = _connection.dataset();
    const int _nLayerCount = _poDataset->GetLayerCount();
    for (int _nLayerIdx = 0; _nLayerIdx < _nLayerCount; ++_nLayerIdx) {
        OGRLayer* _poLayer = _poDataset->GetLayer(_nLayerIdx);
        if (_poLayer == nullptr || _poLayer->GetLayerDefn() == nullptr) {
            continue;
        }

        if (wkbFlatten(_poLayer->GetGeomType()) == wkbNone) {
            continue;
        }

        const OGRFeatureDefn* _poDefn = _poLayer->GetLayerDefn();
        SpatialTableInfo _tableInfo;
        _tableInfo.strDatabasePath = _connection.databasePath();
        _tableInfo.strTableName = QString::fromUtf8(_poLayer->GetName());
        _tableInfo.strGeometryColumn = safeUtf8(_poLayer->GetGeometryColumn());
        if (_tableInfo.strGeometryColumn.trimmed().isEmpty()
            && _poDefn->GetGeomFieldCount() > 0
            && _poDefn->GetGeomFieldDefn(0) != nullptr) {
            _tableInfo.strGeometryColumn =
                safeUtf8(_poDefn->GetGeomFieldDefn(0)->GetNameRef());
        }
        _tableInfo.strSourceUri = buildLayerSourceUri(
            _tableInfo.strDatabasePath, _tableInfo.strTableName);
        _tableInfo.strProviderKey = QStringLiteral("ogr");
        _tableInfo.strCrsAuthId = spatialRefAuthId(_poLayer->GetSpatialRef());
        _tableInfo.strGeometryType =
            safeUtf8(OGRGeometryTypeToName(_poLayer->GetGeomType()));
        _tableInfo.nFeatureCount =
            static_cast<int>(_poLayer->GetFeatureCount(TRUE));

        OGREnvelope _envelope;
        if (_poLayer->GetExtent(&_envelope, TRUE) == OGRERR_NONE) {
            _tableInfo.bHasExtent = true;
            _tableInfo.dMinX = _envelope.MinX;
            _tableInfo.dMinY = _envelope.MinY;
            _tableInfo.dMaxX = _envelope.MaxX;
            _tableInfo.dMaxY = _envelope.MaxY;
        }

        for (int _nFieldIdx = 0; _nFieldIdx < _poDefn->GetFieldCount();
            ++_nFieldIdx) {
            const OGRFieldDefn* _poFieldDefn = _poDefn->GetFieldDefn(_nFieldIdx);
            if (_poFieldDefn == nullptr) {
                continue;
            }

            const QString _strFieldName =
                QString::fromUtf8(_poFieldDefn->GetNameRef());
            _tableInfo.vFieldNames.append(_strFieldName);
            if (isNumericOgrField(_poFieldDefn->GetType())) {
                _tableInfo.vNumericFieldNames.append(_strFieldName);
            }
        }

        _vTables.append(_tableInfo);
    }

    return _vTables;
}

bool SpatialDatabaseService::getSpatialTableInfo(
    const QString& _strDatabasePath,
    const QString& _strTableName,
    SpatialTableInfo& _outTableInfo,
    QString& _strError) const
{
    const QList<SpatialTableInfo> _vTables =
        listSpatialTables(_strDatabasePath, _strError);
    const QString _strRequestedTableName = _strTableName.trimmed();
    const QString _strSafeTableName = sanitizeIdentifier(_strRequestedTableName);

    for (const SpatialTableInfo& _tableInfo : _vTables) {
        if (_tableInfo.strTableName == _strRequestedTableName) {
            _outTableInfo = _tableInfo;
            return true;
        }
    }

    for (const SpatialTableInfo& _tableInfo : _vTables) {
        if (_tableInfo.strTableName.compare(
                _strRequestedTableName, Qt::CaseInsensitive) == 0
            || _tableInfo.strTableName.compare(
                _strSafeTableName, Qt::CaseInsensitive) == 0) {
            _outTableInfo = _tableInfo;
            return true;
        }
    }

    if (_strError.trimmed().isEmpty()) {
        _strError = QStringLiteral("空间表不存在：%1").arg(_strTableName);
    }
    return false;
}

bool SpatialDatabaseService::queryFeaturesByAttribute(
    const SpatialDatabaseQuery& _queryInput,
    QVector<QVariantMap>& _vFeatures,
    QString& _strError) const
{
    _vFeatures.clear();
    SpatialDatabaseConnection _connectionTemp;
    if (!_connectionTemp.open(_queryInput.strDatabasePath, true, _strError)) {
        return false;
    }

    OGRLayer* _poLayer = _connectionTemp.dataset()->GetLayerByName(
        _queryInput.strTableName.toUtf8().constData());
    if (_poLayer == nullptr) {
        _strError = QStringLiteral("空间表不存在：%1").arg(_queryInput.strTableName);
        return false;
    }

    if (!_queryInput.strAttributeWhere.trimmed().isEmpty()
        && _poLayer->SetAttributeFilter(
            _queryInput.strAttributeWhere.toUtf8().constData()) != OGRERR_NONE) {
        _strError = QStringLiteral("属性查询条件无效：%1")
            .arg(_queryInput.strAttributeWhere);
        return false;
    }

    _vFeatures = readFeatureRows(_poLayer, _queryInput.nLimit);
    _poLayer->SetAttributeFilter(nullptr);
    return true;
}

bool SpatialDatabaseService::queryFeaturesByExtent(
    const SpatialDatabaseQuery& _queryInput,
    QVector<QVariantMap>& _vFeatures,
    QString& _strError) const
{
    _vFeatures.clear();
    SpatialDatabaseConnection _connectionTemp;
    if (!_connectionTemp.open(_queryInput.strDatabasePath, true, _strError)) {
        return false;
    }

    OGRLayer* _poLayer = _connectionTemp.dataset()->GetLayerByName(
        _queryInput.strTableName.toUtf8().constData());
    if (_poLayer == nullptr) {
        _strError = QStringLiteral("空间表不存在：%1").arg(_queryInput.strTableName);
        return false;
    }

    if (!_queryInput.strAttributeWhere.trimmed().isEmpty()
        && _poLayer->SetAttributeFilter(
            _queryInput.strAttributeWhere.toUtf8().constData()) != OGRERR_NONE) {
        _strError = QStringLiteral("属性查询条件无效：%1")
            .arg(_queryInput.strAttributeWhere);
        return false;
    }

    if (_queryInput.bHasSpatialExtent) {
        _poLayer->SetSpatialFilterRect(
            _queryInput.dMinX,
            _queryInput.dMinY,
            _queryInput.dMaxX,
            _queryInput.dMaxY);
    }

    _vFeatures = readFeatureRows(_poLayer, _queryInput.nLimit);
    _poLayer->SetSpatialFilter(nullptr);
    _poLayer->SetAttributeFilter(nullptr);
    return true;
}

SpatialSqlResult SpatialDatabaseService::executeSpatialSql(
    const QString& _strDatabasePath,
    const QString& _strSql,
    int _nLimit) const
{
    SpatialSqlResult _result;
    QString _strError;
    SpatialDatabaseConnection _connectionTemp;
    if (!_connectionTemp.open(_strDatabasePath, false, _strError)) {
        _result.strError = _strError;
        return _result;
    }

    CPLErrorReset();
    OGRLayer* _poResultLayer = _connectionTemp.dataset()->ExecuteSQL(
        _strSql.toUtf8().constData(), nullptr, "SQLITE");
    if (_poResultLayer == nullptr) {
        const QString _strGdalError = QString::fromUtf8(CPLGetLastErrorMsg());
        if (!_strGdalError.trimmed().isEmpty()) {
            _result.strError = _strGdalError;
            return _result;
        }

        _result.bSuccess = true;
        return _result;
    }

    _result.vFieldNames = resultFieldNames(_poResultLayer);

    int _nReadCount = 0;
    OGRFeature* _poFeature = nullptr;
    _poResultLayer->ResetReading();
    while ((_poFeature = _poResultLayer->GetNextFeature()) != nullptr) {
        _result.vRows.append(featureToStringRow(_poFeature, _result.vFieldNames));
        OGRFeature::DestroyFeature(_poFeature);

        ++_nReadCount;
        if (_nLimit > 0 && _nReadCount >= _nLimit) {
            break;
        }
    }

    _result.nFeatureCount = _result.vRows.size();
    _result.bSuccess = true;
    _connectionTemp.dataset()->ReleaseResultSet(_poResultLayer);
    return _result;
}

bool SpatialDatabaseService::writeVectorLayerToDatabase(
    QgsVectorLayer* _pLayerInput,
    const QString& _strDatabasePath,
    const QString& _strTableName,
    QString& _strSourceUri,
    QString& _strError) const
{
    _strSourceUri.clear();
    _strError.clear();

    if (_pLayerInput == nullptr || !_pLayerInput->isValid()) {
        _strError = QStringLiteral("待写入的矢量图层无效");
        return false;
    }

    const QFileInfo _fiDatabase(_strDatabasePath);
    QDir _dirParent = _fiDatabase.absoluteDir();
    if (!_dirParent.exists() && !_dirParent.mkpath(QStringLiteral("."))) {
        _strError = QStringLiteral("无法创建空间数据库目录：%1")
            .arg(_dirParent.absolutePath());
        return false;
    }

    const QString _strSafeTableName = sanitizeIdentifier(_strTableName);
    QgsVectorFileWriter::SaveVectorOptions _options =
        buildSpatialiteSaveOptions(_strSafeTableName, _fiDatabase.exists());

    std::unique_ptr<QgsVectorFileWriter> _pWriter(
        QgsVectorFileWriter::create(
            _fiDatabase.absoluteFilePath(),
            _pLayerInput->fields(),
            _pLayerInput->wkbType(),
            _pLayerInput->crs(),
            QgsProject::instance()->transformContext(),
            _options));

    if (!_pWriter || _pWriter->hasError() != QgsVectorFileWriter::NoError) {
        _strError = (_pWriter != nullptr && !_pWriter->errorMessage().isEmpty())
            ? _pWriter->errorMessage()
            : QStringLiteral("未知写出错误");
        return false;
    }

    QgsFeature _featureCurrent;
    QgsFeatureIterator _itFeature = _pLayerInput->getFeatures();
    while (_itFeature.nextFeature(_featureCurrent)) {
        if (!_pWriter->addFeature(_featureCurrent)) {
            _strError = QStringLiteral("写入空间数据库要素失败");
            return false;
        }
    }

    _pWriter.reset();
    _strSourceUri = buildLayerSourceUri(
        _fiDatabase.absoluteFilePath(), _strSafeTableName);
    return true;
}

bool SpatialDatabaseService::isSupportedDatabasePath(const QString& _strPath)
{
    QString _strDatabasePath;
    QString _strTableName;
    const QString _strResolvedPath = parseLayerSourceUri(
        _strPath, _strDatabasePath, _strTableName)
        ? _strDatabasePath
        : _strPath;

    const QString _strExt = QFileInfo(_strResolvedPath).suffix().toLower();
    return _strExt == QStringLiteral("sqlite")
        || _strExt == QStringLiteral("db");
}

bool SpatialDatabaseService::parseLayerSourceUri(
    const QString& _strSourceUri,
    QString& _strDatabasePath,
    QString& _strTableName)
{
    _strDatabasePath.clear();
    _strTableName.clear();

    const QString _strNeedle = QStringLiteral("|layername=");
    const int _nLayerPos = _strSourceUri.indexOf(_strNeedle);
    if (_nLayerPos < 0) {
        return false;
    }

    _strDatabasePath = _strSourceUri.left(_nLayerPos);
    _strTableName = _strSourceUri.mid(_nLayerPos + _strNeedle.size());
    const int _nOptionPos = _strTableName.indexOf('|');
    if (_nOptionPos >= 0) {
        _strTableName = _strTableName.left(_nOptionPos);
    }

    return isSupportedDatabasePath(_strDatabasePath)
        && !_strTableName.trimmed().isEmpty();
}

QString SpatialDatabaseService::buildLayerSourceUri(
    const QString& _strDatabasePath,
    const QString& _strTableName)
{
    return QStringLiteral("%1|layername=%2")
        .arg(QFileInfo(_strDatabasePath).absoluteFilePath(),
            _strTableName);
}

QString SpatialDatabaseService::quoteIdentifier(const QString& _strIdentifier)
{
    QString _strQuoted = _strIdentifier;
    _strQuoted.replace('"', QStringLiteral("\"\""));
    return QStringLiteral("\"%1\"").arg(_strQuoted);
}

QString SpatialDatabaseService::quoteSqlString(const QString& _strValue)
{
    QString _strQuoted = _strValue;
    _strQuoted.replace('\'', QStringLiteral("''"));
    return QStringLiteral("'%1'").arg(_strQuoted);
}

QString SpatialDatabaseService::buildAttributeEqualsWhere(
    const QString& _strFieldName,
    const QVariant& _valueExpected)
{
    const QString _strLeft = quoteIdentifier(_strFieldName);
    switch (static_cast<QMetaType::Type>(_valueExpected.type())) {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Double:
        return QStringLiteral("%1 = %2")
            .arg(_strLeft, _valueExpected.toString());
    case QMetaType::Bool:
        return QStringLiteral("%1 = %2")
            .arg(_strLeft, _valueExpected.toBool()
                ? QStringLiteral("1")
                : QStringLiteral("0"));
    default:
        return QStringLiteral("%1 = %2")
            .arg(_strLeft, quoteSqlString(_valueExpected.toString()));
    }
}

QString SpatialDatabaseService::buildBufferSql(const QString& _strTableName,
    const QString& _strGeometryColumn,
    double _dDistance)
{
    return QStringLiteral("SELECT *, ST_Buffer(%1, %2) AS geom_buffer FROM %3")
        .arg(quoteIdentifier(_strGeometryColumn))
        .arg(_dDistance, 0, 'f', 8)
        .arg(quoteIdentifier(_strTableName));
}

QString SpatialDatabaseService::buildIntersectsSql(const QString& _strLeftTable,
    const QString& _strLeftGeometryColumn,
    const QString& _strRightTable,
    const QString& _strRightGeometryColumn)
{
    return QStringLiteral(
        "SELECT a.* FROM %1 a JOIN %2 b ON ST_Intersects(a.%3, b.%4)")
        .arg(quoteIdentifier(_strLeftTable),
            quoteIdentifier(_strRightTable),
            quoteIdentifier(_strLeftGeometryColumn),
            quoteIdentifier(_strRightGeometryColumn));
}

QString SpatialDatabaseService::buildWithinSql(const QString& _strLeftTable,
    const QString& _strLeftGeometryColumn,
    const QString& _strRightTable,
    const QString& _strRightGeometryColumn)
{
    return QStringLiteral(
        "SELECT a.* FROM %1 a JOIN %2 b ON ST_Within(a.%3, b.%4)")
        .arg(quoteIdentifier(_strLeftTable),
            quoteIdentifier(_strRightTable),
            quoteIdentifier(_strLeftGeometryColumn),
            quoteIdentifier(_strRightGeometryColumn));
}

QString SpatialDatabaseService::sanitizeIdentifier(const QString& _strIdentifier)
{
    QString _strSafe = _strIdentifier.trimmed();
    _strSafe.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_]+")),
        QStringLiteral("_"));
    _strSafe.replace(QRegularExpression(QStringLiteral("_+")),
        QStringLiteral("_"));
    _strSafe = _strSafe.trimmed();
    while (_strSafe.startsWith('_')) {
        _strSafe.remove(0, 1);
    }
    while (_strSafe.endsWith('_')) {
        _strSafe.chop(1);
    }
    if (_strSafe.isEmpty()) {
        _strSafe = QStringLiteral("spatial_result");
    }
    if (_strSafe.at(0).isDigit()) {
        _strSafe.prepend(QStringLiteral("t_"));
    }
    return _strSafe.left(60);
}

QgsVectorFileWriter::SaveVectorOptions
SpatialDatabaseService::buildSpatialiteSaveOptions(
    const QString& _strTableName,
    bool _bDatabaseExists)
{
    QgsVectorFileWriter::SaveVectorOptions _options;
    _options.driverName = QStringLiteral("SQLite");
    _options.layerName = sanitizeIdentifier(_strTableName);
    _options.fileEncoding = QStringLiteral("UTF-8");
    _options.actionOnExistingFile = _bDatabaseExists
        ? QgsVectorFileWriter::CreateOrOverwriteLayer
        : QgsVectorFileWriter::CreateOrOverwriteFile;
    _options.datasourceOptions << QStringLiteral("SPATIALITE=YES");
    _options.layerOptions << QStringLiteral("FORMAT=SPATIALITE")
        << QStringLiteral("GEOMETRY_NAME=geom")
        << QStringLiteral("SPATIAL_INDEX=YES")
        << QStringLiteral("OVERWRITE=YES")
        << QStringLiteral("LAUNDER=NO");
    return _options;
}

QString SpatialDatabaseService::NormalizeDatabasePath(
    const QString& _strDatabasePath)
{
    QString _strPath;
    QString _strTableName;
    if (parseLayerSourceUri(_strDatabasePath, _strPath, _strTableName)) {
        return QFileInfo(_strPath).absoluteFilePath();
    }
    return QFileInfo(_strDatabasePath).absoluteFilePath();
}
