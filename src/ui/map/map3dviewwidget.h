#ifndef MAP3DVIEWWIDGET_H_B4C5D6E7F8A9
#define MAP3DVIEWWIDGET_H_B4C5D6E7F8A9

#include <QString>
#include <QWidget>

#include <qgscoordinatereferencesystem.h>
#include <qgsrectangle.h>

class Qgs3DMapCanvas;
class QgsMapLayer;
class QgsRasterLayer;

// ═══════════════════════════════════════════════════════════════════════════════
//  Map3DViewWidget
//  职责：封装 QGIS 3D 地图画布，并从用户选择的 DEM/栅格创建独立 3D 地形视图。
//        该控件只负责 3D 场景显示，不承载分析算法，不自动加载演示数据。
//  位于 ui/map/ 层，供主窗口“新建3D视图”菜单入口创建顶层 3D 窗口。
// ═══════════════════════════════════════════════════════════════════════════════
class Map3DViewWidget : public QWidget
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数，创建 QGIS 3D QWindow 容器
     * @param_1 _pParent: 父控件指针
     */
    explicit Map3DViewWidget(QWidget* _pParent = nullptr);

    /*
     * @brief 析构函数，释放 3D 场景引用并移除内部隐藏 DEM 图层
     */
    ~Map3DViewWidget() override;

    /*
     * @brief 从 DEM/栅格文件初始化 3D 地形视图
     * @param_1 _strDemPath: DEM/栅格文件路径
     */
    bool initializeFromDem(const QString& _strDemPath);

    /*
     * @brief 将 3D 相机重置到当前 DEM 场景范围
     */
    void resetView();

private:
    /*
     * @brief 创建并校验栅格图层
     * @param_1 _strFilePath: 栅格文件路径
     * @param_2 _strLayerName: 图层显示名
     */
    QgsRasterLayer* createRasterLayer(const QString& _strFilePath,
        const QString& _strLayerName) const;

    /*
     * @brief 延迟创建 QGIS 3D 画布和 QWidget 容器
     */
    bool createCanvasContainer();

    /*
     * @brief 为 DEM 选择适合 3D 地形的场景 CRS
     * @param_1 _pLayerInput: DEM 栅格图层
     */
    QgsCoordinateReferenceSystem sceneCrsForLayer(
        QgsMapLayer* _pLayerInput) const;

    /*
     * @brief 对 3D 场景范围做最小扩展，避免零面积或过小范围
     * @param_1 _rectInput: 原始场景范围
     */
    QgsRectangle normalizedSceneExtent(const QgsRectangle& _rectInput) const;

    /*
     * @brief 注册图层到 QgsProject，但不加入二维图层树
     * @param_1 _pLayerInput: 目标图层
     */
    void registerProjectLayer(QgsMapLayer* _pLayerInput);

    /*
     * @brief 移除当前 3D 视图私有使用的隐藏图层
     */
    void removeRegisteredLayer();

    /*
     * @brief 计算图层范围在目标 CRS 中的包围框
     * @param_1 _pLayerInput: 目标图层
     * @param_2 _crsTarget: 目标坐标参考系
     */
    QgsRectangle transformedLayerExtent(QgsMapLayer* _pLayerInput,
        const QgsCoordinateReferenceSystem& _crsTarget) const;

    Qgs3DMapCanvas* mpCanvas3D;       // QGIS 3D 画布窗口
    QWidget*        mpctrlContainer;  // QWindow 的 QWidget 容器
    QgsRectangle    mrectSceneExtent; // 缓存的 3D 场景范围
    QString         mstrDemLayerId;   // 注册到 QgsProject 的隐藏 DEM 图层 ID
    bool            mbSceneInitialized; // 当前 3D 场景是否已初始化
};

#endif // MAP3DVIEWWIDGET_H_B4C5D6E7F8A9
