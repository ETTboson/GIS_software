#ifndef SPATIALDATABASESERVICE_H_B2C3D4E5F6A1
#define SPATIALDATABASESERVICE_H_B2C3D4E5F6A1

#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QVector>

#include <qgsvectorfilewriter.h>

#include "service/data/spatialdatabaseconnection.h"
#include "service/data/spatialdatabasequery.h"

class QgsVectorLayer;

// ════════════════════════════════════════════════════════
//  SpatialDatabaseService
//  职责：为空间分析和数据查询提供统一的 SpatiaLite/SQLite
//        数据访问接口，封装连接管理、空间表元数据、属性/空间查询、
//        空间 SQL 片段与分析结果入库。
//  位于 service/data/ 层，不持有 UI 对象。
// ════════════════════════════════════════════════════════
class SpatialDatabaseService
{
public:
    /*
     * @brief 构造空间数据库服务
     */
    SpatialDatabaseService();

    /*
     * @brief 析构函数，释放服务持有的全部连接
     */
    ~SpatialDatabaseService();

    /*
     * @brief 打开并托管一个空间数据库连接
     * @param_1 _strDatabasePath: 数据库路径
     * @param_2 _bReadOnly: 是否只读
     * @param_3 _strError: 失败时写出的错误信息
     */
    bool openConnection(const QString& _strDatabasePath,
        bool _bReadOnly,
        QString& _strError);

    /*
     * @brief 关闭指定数据库连接
     * @param_1 _strDatabasePath: 数据库路径
     */
    void closeConnection(const QString& _strDatabasePath);

    /*
     * @brief 关闭全部托管连接
     */
    void closeAllConnections();

    /*
     * @brief 判断指定数据库连接是否已托管并打开
     * @param_1 _strDatabasePath: 数据库路径
     */
    bool hasConnection(const QString& _strDatabasePath) const;

    /*
     * @brief 返回指定数据库的托管连接
     * @param_1 _strDatabasePath: 数据库路径
     */
    SpatialDatabaseConnection* connection(const QString& _strDatabasePath) const;

    /*
     * @brief 枚举数据库中的空间表
     * @param_1 _strDatabasePath: 数据库路径
     * @param_2 _strError: 失败时写出的错误信息
     */
    QList<SpatialTableInfo> listSpatialTables(const QString& _strDatabasePath,
        QString& _strError) const;

    /*
     * @brief 枚举已打开连接中的空间表
     * @param_1 _connection: 已打开的空间数据库连接
     * @param_2 _strError: 失败时写出的错误信息
     */
    QList<SpatialTableInfo> listSpatialTables(
        SpatialDatabaseConnection& _connection,
        QString& _strError) const;

    /*
     * @brief 读取指定空间表元数据
     * @param_1 _strDatabasePath: 数据库路径
     * @param_2 _strTableName: 空间表名
     * @param_3 _outTableInfo: 成功时写出的表信息
     * @param_4 _strError: 失败时写出的错误信息
     */
    bool getSpatialTableInfo(const QString& _strDatabasePath,
        const QString& _strTableName,
        SpatialTableInfo& _outTableInfo,
        QString& _strError) const;

    /*
     * @brief 按属性条件查询空间表要素
     * @param_1 _queryInput: 查询条件 DTO
     * @param_2 _vFeatures: 成功时写出的属性行列表
     * @param_3 _strError: 失败时写出的错误信息
     */
    bool queryFeaturesByAttribute(const SpatialDatabaseQuery& _queryInput,
        QVector<QVariantMap>& _vFeatures,
        QString& _strError) const;

    /*
     * @brief 按空间范围查询空间表要素
     * @param_1 _queryInput: 查询条件 DTO
     * @param_2 _vFeatures: 成功时写出的属性行列表
     * @param_3 _strError: 失败时写出的错误信息
     */
    bool queryFeaturesByExtent(const SpatialDatabaseQuery& _queryInput,
        QVector<QVariantMap>& _vFeatures,
        QString& _strError) const;

    /*
     * @brief 执行空间 SQL 并返回表格化结果
     * @param_1 _strDatabasePath: 数据库路径
     * @param_2 _strSql: SQL 文本
     * @param_3 _nLimit: 最大读取行数，0 表示不限制
     */
    SpatialSqlResult executeSpatialSql(const QString& _strDatabasePath,
        const QString& _strSql,
        int _nLimit = 0) const;

    /*
     * @brief 将 QGIS 矢量图层写入 SpatiaLite 数据库
     * @param_1 _pLayerInput: 待写入的 QGIS 矢量图层
     * @param_2 _strDatabasePath: 目标数据库路径
     * @param_3 _strTableName: 目标空间表名
     * @param_4 _strSourceUri: 成功时写出的目标图层 URI
     * @param_5 _strError: 失败时写出的错误信息
     */
    bool writeVectorLayerToDatabase(QgsVectorLayer* _pLayerInput,
        const QString& _strDatabasePath,
        const QString& _strTableName,
        QString& _strSourceUri,
        QString& _strError) const;

    /*
     * @brief 判断路径是否为当前支持的空间数据库格式
     * @param_1 _strPath: 文件路径
     */
    static bool isSupportedDatabasePath(const QString& _strPath);

    /*
     * @brief 判断并解析 QGIS 数据库图层 URI
     * @param_1 _strSourceUri: 输入 source URI
     * @param_2 _strDatabasePath: 成功时写出的数据库路径
     * @param_3 _strTableName: 成功时写出的表名
     */
    static bool parseLayerSourceUri(const QString& _strSourceUri,
        QString& _strDatabasePath,
        QString& _strTableName);

    /*
     * @brief 构造 QGIS/OGR 数据库图层 URI
     * @param_1 _strDatabasePath: 数据库路径
     * @param_2 _strTableName: 空间表名
     */
    static QString buildLayerSourceUri(const QString& _strDatabasePath,
        const QString& _strTableName);

    /*
     * @brief 构造适合 SQL 的双引号标识符
     * @param_1 _strIdentifier: 原始标识符
     */
    static QString quoteIdentifier(const QString& _strIdentifier);

    /*
     * @brief 构造 SQL 字符串字面量
     * @param_1 _strValue: 原始字符串值
     */
    static QString quoteSqlString(const QString& _strValue);

    /*
     * @brief 构造属性等值查询条件
     * @param_1 _strFieldName: 字段名
     * @param_2 _valueExpected: 目标值
     */
    static QString buildAttributeEqualsWhere(const QString& _strFieldName,
        const QVariant& _valueExpected);

    /*
     * @brief 构造 ST_Buffer 查询 SQL
     * @param_1 _strTableName: 输入空间表名
     * @param_2 _strGeometryColumn: 几何列名
     * @param_3 _dDistance: 缓冲距离
     */
    static QString buildBufferSql(const QString& _strTableName,
        const QString& _strGeometryColumn,
        double _dDistance);

    /*
     * @brief 构造 ST_Intersects 查询 SQL
     * @param_1 _strLeftTable: 左表名
     * @param_2 _strLeftGeometryColumn: 左表几何列名
     * @param_3 _strRightTable: 右表名
     * @param_4 _strRightGeometryColumn: 右表几何列名
     */
    static QString buildIntersectsSql(const QString& _strLeftTable,
        const QString& _strLeftGeometryColumn,
        const QString& _strRightTable,
        const QString& _strRightGeometryColumn);

    /*
     * @brief 构造 ST_Within 查询 SQL
     * @param_1 _strLeftTable: 被判断表名
     * @param_2 _strLeftGeometryColumn: 被判断几何列名
     * @param_3 _strRightTable: 容器表名
     * @param_4 _strRightGeometryColumn: 容器几何列名
     */
    static QString buildWithinSql(const QString& _strLeftTable,
        const QString& _strLeftGeometryColumn,
        const QString& _strRightTable,
        const QString& _strRightGeometryColumn);

    /*
     * @brief 清洗数据库表名或列名
     * @param_1 _strIdentifier: 原始标识符
     */
    static QString sanitizeIdentifier(const QString& _strIdentifier);

    /*
     * @brief 构造 SpatiaLite 写出选项
     * @param_1 _strTableName: 目标空间表名
     * @param_2 _bDatabaseExists: 目标数据库是否已存在
     */
    static QgsVectorFileWriter::SaveVectorOptions buildSpatialiteSaveOptions(
        const QString& _strTableName,
        bool _bDatabaseExists);

private:
    /*
     * @brief 规范化数据库路径作为连接表键
     * @param_1 _strDatabasePath: 数据库路径
     */
    static QString NormalizeDatabasePath(const QString& _strDatabasePath);

    QMap<QString, SpatialDatabaseConnection*> mmapConnections; // 已托管的数据库连接表
};

#endif // SPATIALDATABASESERVICE_H_B2C3D4E5F6A1
