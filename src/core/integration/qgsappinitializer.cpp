#include "qgsappinitializer.h"

#include <qgsapplication.h>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

#ifndef OOP_CMAKE_OSGEO4W_ROOT
#define OOP_CMAKE_OSGEO4W_ROOT ""
#endif

namespace
{
struct RuntimeRootResolution
{
    QString strRuntimeRoot; // 已选择的运行时根目录
    QStringList vTriedRoots; // 已检查的候选根目录
    bool bResolved = false; // 是否找到有效 QGIS 运行时
};

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

QString CompileTimeOsgeoRoot()
{
    return QString::fromUtf8(OOP_CMAKE_OSGEO4W_ROOT).trimmed();
}

bool HasQgisRuntime(const QString& _strRuntimeRoot)
{
    return DirectoryExists(QDir(_strRuntimeRoot).filePath("apps/qgis-ltr"));
}

RuntimeRootResolution ResolveRuntimeRoot(const QString& _strAppDir)
{
    QStringList _vCandidates;
    _vCandidates << _strAppDir;

#if defined(OOP_ENABLE_DEV_RUNTIME_ROOT)
    const QString _strConfiguredRoot = CompileTimeOsgeoRoot();
    if (!_strConfiguredRoot.isEmpty()) {
        _vCandidates << _strConfiguredRoot;
    }
#endif

    const QString _strEnvRoot = EnvPath("OSGEO4W_ROOT");
    if (!_strEnvRoot.isEmpty()) {
        _vCandidates << _strEnvRoot;
    }

    RuntimeRootResolution _resolution;
    for (const QString& _strCandidate : _vCandidates) {
        if (_strCandidate.trimmed().isEmpty()) {
            continue;
        }

        const QString _strRuntimeRoot = QDir(_strCandidate).absolutePath();
        if (_resolution.vTriedRoots.contains(_strRuntimeRoot)) {
            continue;
        }

        _resolution.vTriedRoots << _strRuntimeRoot;
        if (HasQgisRuntime(_strRuntimeRoot)) {
            _resolution.strRuntimeRoot = _strRuntimeRoot;
            _resolution.bResolved = true;
            return _resolution;
        }
    }

    _resolution.strRuntimeRoot = QDir(_strAppDir).absolutePath();
    return _resolution;
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
    const RuntimeRootResolution _resolutionRuntime =
        ResolveRuntimeRoot(_strAppDir);
    const QString _strRuntimeRoot = _resolutionRuntime.strRuntimeRoot;
    const QDir _dirRuntime(_strRuntimeRoot);
    const QString _strQgisPrefixPath =
        _dirRuntime.filePath("apps/qgis-ltr");
    const QString _strProjDbPath =
        _dirRuntime.filePath("share/proj/proj.db");
    const bool _bHasQgisPrefix = DirectoryExists(_strQgisPrefixPath);
    const bool _bHasProjDb = QFileInfo(_strProjDbPath).isFile();

    if (!_resolutionRuntime.bResolved) {
        qWarning() << "[QgsAppInitializer] QGIS runtime root not found. Tried:"
                   << _resolutionRuntime.vTriedRoots;
    }
    if (!_bHasQgisPrefix) {
        qWarning() << "[QgsAppInitializer] QGIS prefix not found:"
                   << _strQgisPrefixPath;
    }
    if (!_bHasProjDb) {
        qWarning() << "[QgsAppInitializer] PROJ database not found:"
                   << _strProjDbPath;
    }

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
    mbValid = _bHasQgisPrefix && _bHasProjDb;
    qDebug() << "[QgsAppInitializer] Runtime root:" << _strRuntimeRoot;
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
