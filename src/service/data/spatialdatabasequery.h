#ifndef SPATIALDATABASEQUERY_H_D4E5F6A1B2C3
#define SPATIALDATABASEQUERY_H_D4E5F6A1B2C3

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

// ════════════════════════════════════════════════════════
//  SpatialTableInfo / SpatialDatabaseQuery / SpatialSqlResult
//  空间数据库访问层的轻量 DTO
//  仅描述数据库路径、空间表、查询条件与 SQL 结果摘要，
//  不包含连接生命周期或业务算法逻辑。
// ════════════════════════════════════════════════════════
struct SpatialTableInfo
{
    QString     strDatabasePath;   // 空间数据库文件路径
    QString     strTableName;      // 空间表名
    QString     strGeometryColumn; // 几何列名
    QString     strSourceUri;      // QGIS/OGR 可加载的图层 URI
    QString     strProviderKey;    // QGIS provider 标识，当前固定为 ogr
    QString     strCrsAuthId;      // CRS 权威标识，如 EPSG:4326
    QString     strGeometryType;   // OGR 几何类型描述
    QStringList vFieldNames;       // 全部属性字段名
    QStringList vNumericFieldNames; // 数值型属性字段名
    int         nFeatureCount = 0; // 要素数量
    bool        bHasExtent = false; // 是否成功读取空间范围
    double      dMinX = 0.0;       // 空间范围最小 X
    double      dMinY = 0.0;       // 空间范围最小 Y
    double      dMaxX = 0.0;       // 空间范围最大 X
    double      dMaxY = 0.0;       // 空间范围最大 Y
};

struct SpatialDatabaseQuery
{
    QString strDatabasePath;       // 空间数据库文件路径
    QString strTableName;          // 目标空间表名
    QString strGeometryColumn;     // 目标几何列名，可为空
    QString strAttributeWhere;     // OGR/SQLite WHERE 条件，不包含 where 关键字
    bool    bHasSpatialExtent = false; // 是否附带空间范围过滤
    double  dMinX = 0.0;           // 查询范围最小 X
    double  dMinY = 0.0;           // 查询范围最小 Y
    double  dMaxX = 0.0;           // 查询范围最大 X
    double  dMaxY = 0.0;           // 查询范围最大 Y
    int     nLimit = 0;            // 最大返回要素数，0 表示不限制
};

struct SpatialSqlResult
{
    bool                 bSuccess = false; // SQL 是否成功执行
    QString              strError;         // 失败原因
    QStringList          vFieldNames;      // SQL 结果字段名
    QVector<QStringList> vRows;            // SQL 结果行，按字段顺序保存
    int                  nFeatureCount = 0; // 返回行数
};

#endif // SPATIALDATABASEQUERY_H_D4E5F6A1B2C3
