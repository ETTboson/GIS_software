#include "qgsappinitializer.h"

#include <qgsapplication.h>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

namespace
{
QString NormalizeNativePath(const QString& _strPath)
{
    return QDir::toNativeSeparators(QDir::cleanPath(_strPath));
}

QString NormalizeForwardPath(const QString& _strPath)
{
    QString _strNormalized = QDir::cleanPath(_strPath);
    _strNormalized.replace("\\", "/");
    return _strNormalized;
}

bool DirectoryExists(const QString& _strPath)
{
    return QFileInfo(_strPath).isDir();
}

QString EnvPath(const char* _pszName)
{
    return QString::fromLocal8Bit(qgetenv(_pszName)).trimmed();
}

QString ResolveRuntimeRoot(const QString& _strAppDir)
{
    QStringList _vCandidates;
    _vCandidates << _strAppDir;

    const QString _strEnvRoot = EnvPath("OSGEO4W_ROOT");
    if (!_strEnvRoot.isEmpty()) {
        _vCandidates << _strEnvRoot;
    }

    for (const QString& _strCandidate : _vCandidates) {
        if (_strCandidate.trimmed().isEmpty()) {
            continue;
        }

        const QString _strRuntimeRoot = QDir(_strCandidate).absolutePath();
        const QString _strQgisPrefix =
            QDir(_strRuntimeRoot).filePath("apps/qgis-ltr");
        if (DirectoryExists(_strQgisPrefix)) {
            return _strRuntimeRoot;
        }
    }

    return QDir(_strAppDir).absolutePath();
}

void SetEnvPath(const char* _pszName, const QString& _strPath)
{
    if (_strPath.trimmed().isEmpty()) {
        return;
    }

    qputenv(_pszName, NormalizeNativePath(_strPath).toLocal8Bit());
}

void SetEnvPathList(const char* _pszName, const QStringList& _vPaths)
{
    QStringList _vNormalized;
    for (const QString& _strPath : _vPaths) {
        if (!_strPath.trimmed().isEmpty()) {
            _vNormalized << NormalizeNativePath(_strPath);
        }
    }

    if (!_vNormalized.isEmpty()) {
        qputenv(_pszName, _vNormalized.join(";").toLocal8Bit());
    }
}

void PrependPathEnv(const QStringList& _vPaths)
{
    QStringList _vValidPaths;
    for (const QString& _strPath : _vPaths) {
        if (DirectoryExists(_strPath)) {
            _vValidPaths << NormalizeNativePath(_strPath);
        }
    }

    if (_vValidPaths.isEmpty()) {
        return;
    }

    const QString _strOldPath = QString::fromLocal8Bit(qgetenv("PATH"));
    QString _strNewPath = _vValidPaths.join(";");
    if (!_strOldPath.isEmpty()) {
        _strNewPath += ";" + _strOldPath;
    }
    qputenv("PATH", _strNewPath.toLocal8Bit());
}
}

// ════════════════════════════════════════════════════════
//  构造：依次完成 QGIS 初始化三步
// ════════════════════════════════════════════════════════
QgsAppInitializer::QgsAppInitializer()
    : mbValid(false)
{
    const QString _strAppDir = QCoreApplication::applicationDirPath();
    const QString _strRuntimeRoot = ResolveRuntimeRoot(_strAppDir);
    const QDir _dirRuntime(_strRuntimeRoot);
    const QString _strQgisPrefixPath =
        _dirRuntime.filePath("apps/qgis-ltr");
    SetEnvPath("OSGEO4W_ROOT", _strRuntimeRoot);
    qputenv("QGIS_PREFIX_PATH",
        NormalizeForwardPath(_strQgisPrefixPath).toLocal8Bit());
    SetEnvPathList("QT_PLUGIN_PATH", QStringList{
        _strAppDir,
        _dirRuntime.filePath("apps/qgis-ltr/qtplugins"),
        _dirRuntime.filePath("apps/Qt5/plugins")
    });
    SetEnvPath("GDAL_DATA", _dirRuntime.filePath("apps/gdal/share/gdal"));
    SetEnvPath("GDAL_DRIVER_PATH", _dirRuntime.filePath("apps/gdal/lib/gdalplugins"));
    SetEnvPath("PROJ_DATA", _dirRuntime.filePath("share/proj"));
    SetEnvPath("PROJ_LIB", _dirRuntime.filePath("share/proj"));
    SetEnvPath("SSL_CERT_FILE", _dirRuntime.filePath("bin/curl-ca-bundle.crt"));
    qputenv("GDAL_FILENAME_IS_UTF8", "YES");
    qputenv("VSI_CACHE", "TRUE");
    qputenv("VSI_CACHE_SIZE", "1000000");

    PrependPathEnv(QStringList{
        _strAppDir,
        _dirRuntime.filePath("apps/qgis-ltr/bin"),
        _dirRuntime.filePath("apps/Qt5/bin"),
        _dirRuntime.filePath("bin")
    });

    QCoreApplication::addLibraryPath(_strAppDir);
    QCoreApplication::addLibraryPath(
        _dirRuntime.filePath("apps/qgis-ltr/qtplugins"));
    QCoreApplication::addLibraryPath(
        _dirRuntime.filePath("apps/Qt5/plugins"));

    // 1. 设置 QGIS 安装前缀路径，让 QGIS 找到插件、资源等
    QgsApplication::setPrefixPath(NormalizeForwardPath(_strQgisPrefixPath), true);

    // 2. 初始化 QGIS 核心（加载 provider 注册表等）
    QgsApplication::init();

    // 3. 初始化 QGIS 完整运行时（数据提供者、投影库等）
    QgsApplication::initQgis();
    mbValid = DirectoryExists(_strQgisPrefixPath);
    if (!mbValid) {
        qWarning() << "[QgsAppInitializer] QGIS prefix not found:"
                   << _strQgisPrefixPath;
    }
}

// ════════════════════════════════════════════════════════
//  析构：释放所有 QGIS 资源
// ════════════════════════════════════════════════════════
QgsAppInitializer::~QgsAppInitializer()
{
    QgsApplication::exitQgis();
    qDebug() << "[QgsAppInitializer] QGIS exited.";
}

bool QgsAppInitializer::isValid() const
{
    return mbValid;
}
