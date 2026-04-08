// src/model/datamodule.h
#ifndef DATAMODULE_H_B2C3D4E5F6A1
#define DATAMODULE_H_B2C3D4E5F6A1

#include <QObject>
#include <QString>

// ════════════════════════════════════════════════════════
//  DataModule
//  职责：数据加载、格式解析、图层管理
//  通过信号对外通知结果，不持有任何 UI 指针
// ════════════════════════════════════════════════════════
class DataModule : public QObject
{
    Q_OBJECT

public:
    explicit DataModule(QObject *_pParent = nullptr);
    ~DataModule();

    // 公开接口
    void loadData(const QString &_strFilePath);
    void clearAllLayers();

signals:
    // 数据加载成功，携带文件路径
    void dataLoaded(const QString &_strFilePath);
    // 数据加载失败，携带错误描述
    void dataLoadFailed(const QString &_strErrorMsg);

private:
    // 内部解析函数
    bool parseShapefile(const QString &_strFilePath);
    bool parseGeoTiff(const QString &_strFilePath);
    bool parseGeoJson(const QString &_strFilePath);

    // 成员变量
    QStringList  mvstrLoadedPaths;    // 已加载文件路径列表
};

#endif // DATAMODULE_H_B2C3D4E5F6A1
