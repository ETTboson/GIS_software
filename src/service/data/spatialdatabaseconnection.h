#ifndef SPATIALDATABASECONNECTION_H_A1B2C3D4E5F6
#define SPATIALDATABASECONNECTION_H_A1B2C3D4E5F6

#include <QString>

class GDALDataset;

// ════════════════════════════════════════════════════════
//  SpatialDatabaseConnection
//  职责：以 RAII 方式管理 SpatiaLite/SQLite 空间数据库连接，
//        统一封装 GDALDataset 的创建、持有与释放。
//  位于 service/data/ 层，不包含 UI 与具体空间分析算法。
// ════════════════════════════════════════════════════════
class SpatialDatabaseConnection
{
public:
    /*
     * @brief 构造空连接
     */
    SpatialDatabaseConnection();

    /*
     * @brief 析构函数，自动释放数据库连接
     */
    ~SpatialDatabaseConnection();

    /*
     * @brief 打开空间数据库
     * @param_1 _strDatabasePath: SpatiaLite/SQLite 数据库路径
     * @param_2 _bReadOnly: 是否以只读方式打开
     * @param_3 _strError: 失败时写出的错误信息
     */
    bool open(const QString& _strDatabasePath,
        bool _bReadOnly,
        QString& _strError);

    /*
     * @brief 释放当前数据库连接
     */
    void close();

    /*
     * @brief 判断当前连接是否已打开
     */
    bool isOpen() const;

    /*
     * @brief 返回底层 GDALDataset 指针
     */
    GDALDataset* dataset() const;

    /*
     * @brief 返回当前连接的数据库路径
     */
    QString databasePath() const;

    /*
     * @brief 返回当前连接是否为只读连接
     */
    bool isReadOnly() const;

    SpatialDatabaseConnection(const SpatialDatabaseConnection&) = delete;
    SpatialDatabaseConnection& operator=(const SpatialDatabaseConnection&) = delete;

private:
    GDALDataset* mpoDataset;     // 底层 GDAL 数据集连接
    QString      mstrDatabasePath; // 数据库文件路径
    bool         mbReadOnly;     // 当前连接是否只读
};

#endif // SPATIALDATABASECONNECTION_H_A1B2C3D4E5F6
