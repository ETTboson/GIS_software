#include "qgsappinitializer.h"

#include <qgsapplication.h>
#include <QDebug>

// QGIS 安装根目录（对应 D:\OSGeo4W\apps\qgis-ltr）
static const char* kszQgisPrefixPath = "D:/OSGeo4W/apps/qgis-ltr";

// ════════════════════════════════════════════════════════
//  构造：依次完成 QGIS 初始化三步
// ════════════════════════════════════════════════════════
QgsAppInitializer::QgsAppInitializer()
    : mbValid(false)
{
    // 1. 设置 QGIS 安装前缀路径，让 QGIS 找到插件、资源等
    QgsApplication::setPrefixPath(kszQgisPrefixPath, true);

    // 2. 初始化 QGIS 核心（加载 provider 注册表等）
    QgsApplication::init();

    // 3. 初始化 QGIS 完整运行时（数据提供者、投影库等）
    QgsApplication::initQgis();
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
