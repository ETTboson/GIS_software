#ifndef CHARTWIDGET_H_B7C8D9E0F1A2
#define CHARTWIDGET_H_B7C8D9E0F1A2

#include <QWidget>

#include "model/dto/visualizationdata.h"

class QPainter;
class QMouseEvent;
class QPaintEvent;
class QEvent;

// ════════════════════════════════════════════════════════
//  ChartWidget
//  图表基类控件
//  负责维护当前图表 DTO、统一处理悬停/点击交互，并把绘制细节交给子类
// ════════════════════════════════════════════════════════
class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数
     * @param_1 _pParent: 父控件指针
     */
    explicit ChartWidget(QWidget* _pParent = nullptr);

    /*
     * @brief 析构函数
     */
    ~ChartWidget() override = default;

    /*
     * @brief 设置当前图表数据
     * @param_1 _dataVisualization: 结构化图表 DTO
     */
    void setVisualizationData(const VisualizationData& _dataVisualization);

    /*
     * @brief 清空当前图表数据
     */
    void clearVisualization();

    /*
     * @brief 返回当前是否存在可绘制图表数据
     */
    bool hasVisualization() const;

signals:
    /*
     * @brief 用户点击某个数据点时发出
     * @param_1 _point: 被选中的数据点 DTO
     */
    void pointSelected(const VisualizationPoint& _point);

protected:
    /*
     * @brief 重绘控件背景与图表内容
     * @param_1 _pEvent: 绘制事件
     */
    void paintEvent(QPaintEvent* _pEvent) override;

    /*
     * @brief 处理鼠标移动，更新悬停提示
     * @param_1 _pEvent: 鼠标事件
     */
    void mouseMoveEvent(QMouseEvent* _pEvent) override;

    /*
     * @brief 处理鼠标点击，更新当前选中点
     * @param_1 _pEvent: 鼠标事件
     */
    void mousePressEvent(QMouseEvent* _pEvent) override;

    /*
     * @brief 处理鼠标离开，清理悬停状态
     * @param_1 _pEvent: 通用事件对象
     */
    void leaveEvent(QEvent* _pEvent) override;

    /*
     * @brief 由子类绘制具体图表内容
     * @param_1 _painter: 画笔对象
     * @param_2 _rcPlotArea: 图表绘制区域
     */
    virtual void drawChart(QPainter& _painter, const QRectF& _rcPlotArea) = 0;

    /*
     * @brief 由子类命中测试鼠标所在的数据点
     * @param_1 _ptPos: 当前鼠标位置
     */
    virtual int hitTestPoint(const QPoint& _ptPos) const = 0;

    /*
     * @brief 返回当前图表 DTO
     */
    const VisualizationData& visualizationData() const;

    /*
     * @brief 返回主系列数据指针
     */
    const VisualizationSeries* primarySeries() const;

    /*
     * @brief 计算图表主绘制区域
     */
    QRectF buildPlotArea() const;

    /*
     * @brief 返回当前悬停点索引
     */
    int hoveredPointIndex() const;

    /*
     * @brief 返回当前选中点索引
     */
    int selectedPointIndex() const;

    /*
     * @brief 为指定点索引构造 tooltip 文本
     * @param_1 _nPointIdx: 点索引
     */
    QString toolTipForPoint(int _nPointIdx) const;

protected:
    VisualizationData mdataVisualization; // 当前绑定的图表 DTO
    int mnHoveredPointIdx;                // 当前悬停的数据点索引
    int mnSelectedPointIdx;               // 当前选中的数据点索引
};

#endif // CHARTWIDGET_H_B7C8D9E0F1A2
