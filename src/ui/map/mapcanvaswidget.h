#ifndef MAPCANVASWIDGET_H_A1B2C3D4E5F6
#define MAPCANVASWIDGET_H_A1B2C3D4E5F6

#include <QWidget>
#include <QList>
#include <QString>
#include <QStringList>
#include <qgspointxy.h>
#include <qgsrectangle.h>

#include "model/dto/layerinfo.h"
#include "model/enums/maptooltype.h"

// QGIS 前向声明
class QgsMapCanvas;
class QgsMapLayer;
class QgsMapToolPan;
class QgsMapToolZoom;
class QgsRasterLayer;
class QgsSymbol;
class QgsVectorLayer;
class QColor;

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
     * @brief 从图层信息 DTO 加载图层，支持文件图层与空间数据库表
     * @param_1 _layerInfo: 图层元信息 DTO
     */
    void loadLayer(const LayerInfo& _layerInfo);

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
     * @brief 缩放画布到指定图层的范围
     * @param_1 _strLayerId: 目标图层的 QGIS 图层 ID
     */
    void zoomToLayerExtent(const QString& _strLayerId);

    /*
     * @brief 按照给定 ID 顺序重新排列画布图层（列表首项在最上层渲染）
     * @param_1 _vstrLayerIds: 图层 ID 有序列表
     */
    void setLayerOrder(const QStringList& _vstrLayerIds);

    /*
     * @brief 为指定矢量图层应用简单符号样式
     * @param_1 _strLayerId: 目标图层的 QGIS 图层 ID
     * @param_2 _colorSymbol: 符号主颜色
     * @param_3 _dSizeValue: 线宽或点大小，面图层用于描边宽度
     * @param_4 _dOpacity: 透明度，范围 0.0 到 1.0
     */
    void applyVectorSimpleStyle(const QString& _strLayerId,
        const QColor& _colorSymbol,
        double _dSizeValue,
        double _dOpacity);

    /*
     * @brief 按字段类型自动为矢量图层应用分类或分级渲染
     * @param_1 _strLayerId: 目标图层的 QGIS 图层 ID
     * @param_2 _strFieldName: 渲染字段名
     * @param_3 _nClassCount: 数值分级数量
     */
    void applyVectorFieldRenderer(const QString& _strLayerId,
        const QString& _strFieldName,
        int _nClassCount);

    /*
     * @brief 为单波段栅格应用灰度拉伸
     * @param_1 _strLayerId: 目标图层的 QGIS 图层 ID
     */
    void applyRasterGrayRenderer(const QString& _strLayerId);

    /*
     * @brief 为单波段栅格应用伪彩色渲染
     * @param_1 _strLayerId: 目标图层的 QGIS 图层 ID
     */
    void applyRasterPseudoColorRenderer(const QString& _strLayerId);

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

    /*
     * @brief 为查询结果矢量图层应用高亮样式
     * @param_1 _pLayerInput: 待设置样式的矢量图层
     */
    void applyHighlightStyle(QgsVectorLayer* _pLayerInput);

    /*
     * @brief 按图层 ID 查找当前画布中的图层
     * @param_1 _strLayerId: 目标图层 ID
     */
    QgsMapLayer* findLayerById(const QString& _strLayerId) const;

    /*
     * @brief 为矢量符号应用通用颜色、透明度与尺寸
     * @param_1 _pSymbol: 目标符号
     * @param_2 _colorSymbol: 符号颜色
     * @param_3 _dSizeValue: 线宽或点大小
     * @param_4 _dOpacity: 透明度
     */
    void applyCommonSymbolStyle(QgsSymbol* _pSymbol,
        const QColor& _colorSymbol,
        double _dSizeValue,
        double _dOpacity) const;

    /*
     * @brief 判断字段是否为数值类型
     * @param_1 _pLayerInput: 矢量图层
     * @param_2 _strFieldName: 字段名
     */
    bool isNumericField(QgsVectorLayer* _pLayerInput,
        const QString& _strFieldName) const;

    /*
     * @brief 把图层原始范围转换为当前画布 CRS 下的范围
     * @param_1 _pLayerInput: 目标图层
     */
    QgsRectangle layerExtentInCanvasCrs(QgsMapLayer* _pLayerInput) const;

    // ── QGIS 核心对象 ─────────────────────────────────
    QgsMapCanvas*   mpCanvas;       // QGIS 画布主体
    QgsMapToolPan*  mpToolPan;      // 平移工具
    QgsMapToolZoom* mpToolZoomIn;   // 放大工具
    QgsMapToolZoom* mpToolZoomOut;  // 缩小工具

    // ── 图层管理 ──────────────────────────────────────
    QList<QgsMapLayer*> mpvLayers;  // 当前画布持有的图层列表（有序，首项在最上层）
};

#endif // MAPCANVASWIDGET_H_A1B2C3D4E5F6
