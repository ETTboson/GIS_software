#ifndef DATAREPOSITORY_H_728394A5B6C7
#define DATAREPOSITORY_H_728394A5B6C7

#include <QObject>
#include <QList>
#include <QString>

#include "model/dto/analysisdataasset.h"

// ════════════════════════════════════════════════════════
//  DataRepository
//  统一分析资产仓库
//  负责维护分析资产列表、当前选择资产与稳定资产 ID，
//  供 MainWindow、AnalysisWorkspaceDockWidget 与 AI 上下文读取共享状态。
// ════════════════════════════════════════════════════════
class DataRepository : public QObject
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数
     * @param_1 _pParent: 父对象指针
     */
    explicit DataRepository(QObject* _pParent = nullptr);

    /*
     * @brief 析构函数
     */
    ~DataRepository() override = default;

    /*
     * @brief 新增或更新分析资产，并返回仓库内实际保存的版本
     * @param_1 _assetInput: 待写入的分析资产 DTO
     */
    AnalysisDataAsset upsertAsset(const AnalysisDataAsset& _assetInput);

    /*
     * @brief 清空全部资产并重置当前选择
     */
    void clearAssets();

    /*
     * @brief 返回当前全部资产列表
     */
    QList<AnalysisDataAsset> getAssets() const;

    /*
     * @brief 按资产 ID 查找资产
     * @param_1 _strAssetId: 资产唯一标识
     */
    AnalysisDataAsset findAssetById(const QString& _strAssetId) const;

    /*
     * @brief 判断当前是否存在分析资产
     */
    bool hasAssets() const;

    /*
     * @brief 返回当前选择的分析资产
     */
    AnalysisDataAsset currentAsset() const;

    /*
     * @brief 判断当前是否存在已选择资产
     */
    bool hasCurrentAsset() const;

    /*
     * @brief 按资产 ID 切换当前选择
     * @param_1 _strAssetId: 目标资产 ID
     */
    bool selectAssetById(const QString& _strAssetId);

signals:
    /*
     * @brief 资产列表发生变更时触发
     */
    void assetListChanged();

    /*
     * @brief 单个资产写入仓库后触发
     * @param_1 _assetStored: 仓库中实际保存的资产 DTO
     */
    void assetUpserted(const AnalysisDataAsset& _assetStored);

    /*
     * @brief 当前选择资产变更时触发
     * @param_1 _assetCurrent: 当前选中的资产 DTO
     */
    void currentAssetChanged(const AnalysisDataAsset& _assetCurrent);

    /*
     * @brief 当前选择被清空时触发
     */
    void currentAssetCleared();

private:
    /*
     * @brief 构造资产稳定键，供同源同类型资产复用 ID
     * @param_1 _assetInput: 待计算键的资产 DTO
     */
    static QString BuildAssetKey(const AnalysisDataAsset& _assetInput);

    QList<AnalysisDataAsset> mvAssets;     // 当前仓库中的全部分析资产
    QString                  mstrCurrentAssetId; // 当前选中的资产 ID
};

#endif // DATAREPOSITORY_H_728394A5B6C7
