#ifndef LINECHARTWIDGET_H_F1A2B3C4D5E6
#define LINECHARTWIDGET_H_F1A2B3C4D5E6

#include "ui/visualization/charts/chartwidget.h"

#include <QPair>
#include <QVector>
#include <QPointF>
#include <QColor>

// ════════════════════════════════════════════════════════
//  LineChartWidget
//  折线图控件
//  基于 QPainter 绘制趋势折线，并提供点位命中测试
// ════════════════════════════════════════════════════════
class LineChartWidget : public ChartWidget
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数
     * @param_1 _pParent: 父控件指针
     */
    explicit LineChartWidget(QWidget* _pParent = nullptr);

    /*
     * @brief 析构函数
     */
    ~LineChartWidget() override = default;

protected:
    /*
     * @brief 绘制折线图内容
     * @param_1 _painter: 画笔对象
     * @param_2 _rcPlotArea: 图表绘制区域
     */
    void drawChart(QPainter& _painter, const QRectF& _rcPlotArea) override;

    /*
     * @brief 命中测试当前鼠标所在点位
     * @param_1 _ptPos: 当前鼠标位置
     */
    int hitTestPoint(const QPoint& _ptPos) const override;

private:
    /*
     * @brief 计算各点位的屏幕坐标
     * @param_1 _rcPlotArea: 图表绘制区域
     */
    QVector<QPointF> buildPointPositions(const QRectF& _rcPlotArea) const;

    /*
     * @brief 计算当前图表值域
     */
    QPair<double, double> valueDomain() const;

    /*
     * @brief 把数据值映射到 Y 坐标
     * @param_1 _dValue: 数据值
     * @param_2 _rcPlotArea: 图表绘制区域
     * @param_3 _dDomainMin: 值域下界
     * @param_4 _dDomainMax: 值域上界
     */
    double valueToY(double _dValue, const QRectF& _rcPlotArea,
        double _dDomainMin, double _dDomainMax) const;

    /*
     * @brief 绘制坐标轴、刻度与轴标题
     * @param_1 _painter: 画笔对象
     * @param_2 _rcPlotArea: 图表绘制区域
     * @param_3 _dDomainMin: 值域下界
     * @param_4 _dDomainMax: 值域上界
     */
    void drawAxes(QPainter& _painter, const QRectF& _rcPlotArea,
        double _dDomainMin, double _dDomainMax) const;

    /*
     * @brief 返回当前系列颜色
     */
    QColor seriesColor() const;

    /*
     * @brief 格式化坐标轴与点位显示文本
     * @param_1 _dValue: 数值
     */
    QString formatValueText(double _dValue) const;
};

#endif // LINECHARTWIDGET_H_F1A2B3C4D5E6
