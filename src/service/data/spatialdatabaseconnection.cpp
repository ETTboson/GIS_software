#include "spatialdatabaseconnection.h"

#include <QDir>
#include <QFileInfo>

#include <cpl_conv.h>
#include <cpl_error.h>
#include <gdal_priv.h>

SpatialDatabaseConnection::SpatialDatabaseConnection()
    : mpoDataset(nullptr)
    , mbReadOnly(true)
{
}

SpatialDatabaseConnection::~SpatialDatabaseConnection()
{
    close();
}

bool SpatialDatabaseConnection::open(const QString& _strDatabasePath,
    bool _bReadOnly,
    QString& _strError)
{
    close();
    _strError.clear();

    if (_strDatabasePath.trimmed().isEmpty()) {
        _strError = QStringLiteral("空间数据库路径为空");
        return false;
    }

    const QFileInfo _fiDatabase(_strDatabasePath);
    if (_bReadOnly && !_fiDatabase.exists()) {
        _strError = QStringLiteral("空间数据库不存在：%1").arg(_strDatabasePath);
        return false;
    }

    if (!_bReadOnly) {
        QDir _dirParent = _fiDatabase.absoluteDir();
        if (!_dirParent.exists() && !_dirParent.mkpath(QStringLiteral("."))) {
            _strError = QStringLiteral("无法创建空间数据库目录：%1")
                .arg(_dirParent.absolutePath());
            return false;
        }
    }

    GDALAllRegister();
    const QByteArray _baPath = _fiDatabase.absoluteFilePath().toUtf8();
    const int _nOpenFlags = GDAL_OF_VECTOR
        | (_bReadOnly ? GDAL_OF_READONLY : GDAL_OF_UPDATE);
    char** _papszOpenOptions = nullptr;
    _papszOpenOptions = CSLSetNameValue(
        _papszOpenOptions, "PRELUDE_STATEMENTS", "PRAGMA trusted_schema=1");
    mpoDataset = static_cast<GDALDataset*>(
        GDALOpenEx(_baPath.constData(),
            _nOpenFlags,
            nullptr,
            _papszOpenOptions,
            nullptr));
    CSLDestroy(_papszOpenOptions);

    if (mpoDataset == nullptr && !_bReadOnly && !_fiDatabase.exists()) {
        GDALDriver* _poDriver =
            GetGDALDriverManager()->GetDriverByName("SQLite");
        if (_poDriver == nullptr) {
            _strError = QStringLiteral("当前 GDAL 环境缺少 SQLite/SpatiaLite 驱动");
            return false;
        }

        char** _papszOptions = nullptr;
        _papszOptions = CSLSetNameValue(_papszOptions, "SPATIALITE", "YES");
        mpoDataset = _poDriver->Create(
            _baPath.constData(), 0, 0, 0, GDT_Unknown, _papszOptions);
        CSLDestroy(_papszOptions);
    }

    if (mpoDataset == nullptr) {
        const QString _strGdalError = QString::fromUtf8(CPLGetLastErrorMsg());
        _strError = _strGdalError.trimmed().isEmpty()
            ? QStringLiteral("无法打开空间数据库：%1").arg(_strDatabasePath)
            : _strGdalError;
        return false;
    }

    mstrDatabasePath = _fiDatabase.absoluteFilePath();
    mbReadOnly = _bReadOnly;
    return true;
}

void SpatialDatabaseConnection::close()
{
    if (mpoDataset != nullptr) {
        GDALClose(static_cast<GDALDatasetH>(mpoDataset));
        mpoDataset = nullptr;
    }

    mstrDatabasePath.clear();
    mbReadOnly = true;
}

bool SpatialDatabaseConnection::isOpen() const
{
    return mpoDataset != nullptr;
}

GDALDataset* SpatialDatabaseConnection::dataset() const
{
    return mpoDataset;
}

QString SpatialDatabaseConnection::databasePath() const
{
    return mstrDatabasePath;
}

bool SpatialDatabaseConnection::isReadOnly() const
{
    return mbReadOnly;
}
