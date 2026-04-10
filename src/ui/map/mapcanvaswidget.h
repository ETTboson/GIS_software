#ifndef MAPCANVASWIDGET_H_A1B2C3D4E5F6
#define MAPCANVASWIDGET_H_A1B2C3D4E5F6

#include <QWidget>
#include <QList>
#include <QString>
#include <QStringList>
#include <qgspointxy.h>

#include "model/enums/maptooltype.h"

// QGIS 前向声明
class QgsMapCanvas;
class QgsMapLayer;
class QgsMapToolPan;
class QgsMapToolZoom;

// ════════════════════════════════════════════════════════
//  MapCanvasWidget
//  职责：对 QgsMapCanvas 进行封装，提供符合项目分层规范的
//        信号槽接口，隔离 QGIS API 细节。
//  位于 ui/map/ 层，只通过信号槽与上层 MainWindow 通信，
//  不直接持有任何 Service 或业务对象。
// ════════════════════════════════════════════════════════
class MapCanvasWidget : public QWidget
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数，创建并布局内部 QgsMapCanvas，初始化地图工具
     * @param_1 _pParent: 父控件指针
     */
    explicit MapCanvasWidget(QWidget* _pParent = nullptr);

    /*
     * @brief 析构函数，释放地图工具对象
     */
    ~MapCanvasWidget() override;

    /*
     * @brief 返回内部 QgsMapCanvas 裸指针，供 QgsLayerTreeMapCanvasBridge 等使用
     */
    QgsMapCanvas* mapCanvas() const;

signals:
    /*
     * @brief 鼠标在画布上移动时，发出当前地理坐标（WGS84 经纬度）
     * @param _dLon: 经度
     * @param _dLat: 纬度
     */
    void coordChanged(double _dLon, double _dLat);

    /*
     * @brief 地图缩放完成后，发出当前比例尺分母
     * @param _dScale: 比例尺分母，例如 50000.0 表示 1:50000
     */
    void scaleChanged(double _dScale);

    /*
     * @brief 用户点击画布上某图层要素时发出（预留）
     * @param _strLayerId: 被点击图层的 QGIS 图层 ID
     */
    void layerClicked(const QString& _strLayerId);

public slots:
    /*
     * @brief 从文件路径加载图层（自动识别矢量/栅格），加入画布并刷新
     * @param_1 _strFilePath: 空间数据文件的绝对路径
     */
    void loadFromPath(const QString& _strFilePath);

    /*
     * @brief 从画布中移除指定图层
     * @param_1 _strLayerId: 目标图层的 QGIS 图层 ID
     */
    void removeLayer(const QString& _strLayerId);

    /*
     * @brief 设置指定图层的可见性
     * @param_1 _strLayerId: 目标图层的 QGIS 图层 ID
     * @param_2 _bVisible:   true 为显示，false 为隐藏
     */
    void setLayerVisible(const QString& _strLayerId, bool _bVisible);

    /*
     * @brief 切换当前地图交互工具
     * @param_1 _eToolType: 工具类型枚举值
     */
    void setMapTool(MapToolType _eToolType);

    /*
     * @brief 缩放画布至所有图层的合并范围
     */
    void zoomToFullExtent();

    /*
     * @brief 按照给定 ID 顺序重新排列画布图层（列表首项在最上层渲染）
     * @param_1 _vstrLayerIds: 图层 ID 有序列表
     */
    void setLayerOrder(const QStringList& _vstrLayerIds);

private slots:
    /*
     * @brief 响应 QgsMapCanvas::xyCoordinates 信号，转换并发出 coordChanged
     * @param_1 _point: QGIS 内部坐标点（地图 CRS）
     */
    void onXYCoordinates(const QgsPointXY& _point);

    /*
     * @brief 响应 QgsMapCanvas::scaleChanged 信号，转发 scaleChanged
     * @param_1 _dScale: 新比例尺分母
     */
    void onScaleChanged(double _dScale);

private:
    /*
     * @brief 内部初始化：创建 QgsMapCanvas、设置 CRS、布局、地图工具
     */
    void initCanvas();

    /*
     * @brief 根据当前 mpvLayers 重新设置画布图层列表并刷新
     */
    void refreshCanvasLayers();

    // ── QGIS 核心对象 ─────────────────────────────────
    QgsMapCanvas*   mpCanvas;       // QGIS 画布主体
    QgsMapToolPan*  mpToolPan;      // 平移工具
    QgsMapToolZoom* mpToolZoomIn;   // 放大工具
    QgsMapToolZoom* mpToolZoomOut;  // 缩小工具

    // ── 图层管理 ──────────────────────────────────────
    QList<QgsMapLayer*> mpvLayers;  // 当前画布持有的图层列表（有序，首项在最上层）
};

#endif // MAPCANVASWIDGET_H_A1B2C3D4E5F6
