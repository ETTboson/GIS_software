#ifndef VISUALIZATIONDOCKWIDGET_H_C8D9E0F1A2B3
#define VISUALIZATIONDOCKWIDGET_H_C8D9E0F1A2B3

#include <QDockWidget>

#include "model/enums/visualizationcharttype.h"

class QLabel;
class QTextEdit;
class QWidget;
class QStackedWidget;

// ════════════════════════════════════════════════════════
//  VisualizationDockWidget
//  可视化结果停靠面板
//  负责承载占位页、柱状图页、折线图页以及交互详情区
// ════════════════════════════════════════════════════════
class VisualizationDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数
     * @param_1 _pParent: 父控件指针
     */
    explicit VisualizationDockWidget(QWidget* _pParent = nullptr);

    /*
     * @brief 析构函数
     */
    ~VisualizationDockWidget() override = default;

    /*
     * @brief 为指定图表类型注册展示控件
     * @param_1 _chartType: 图表类型
     * @param_2 _pWidget: 图表控件
     */
    void setChartWidget(VisualizationChartType _chartType, QWidget* _pWidget);

    /*
     * @brief 切换到指定图表页面
     * @param_1 _chartType: 图表类型
     * @param_2 _strTitle: 当前图表标题
     */
    void showChart(VisualizationChartType _chartType, const QString& _strTitle);

    /*
     * @brief 切换到占位页面
     * @param_1 _strMessage: 占位或错误提示文本
     */
    void showPlaceholder(const QString& _strMessage);

    /*
     * @brief 更新下方详情区文本
     * @param_1 _strDetailText: 详情文本
     */
    void setDetailText(const QString& _strDetailText);

private:
    /*
     * @brief 返回图表类型对应的容器页
     * @param_1 _chartType: 图表类型
     */
    QWidget* pageForChartType(VisualizationChartType _chartType) const;

    QWidget*       mpctrlContainer;        // 主容器控件
    QLabel*        mpctrlLabelTitle;       // 图表标题标签
    QStackedWidget* mpctrlChartStack;      // 图表页面堆栈
    QWidget*       mpctrlPagePlaceholder;  // 占位页面
    QWidget*       mpctrlPageBarChart;     // 柱状图页面
    QWidget*       mpctrlPageLineChart;    // 折线图页面
    QLabel*        mpctrlLabelPlaceholder; // 占位提示标签
    QTextEdit*     mpctrlDetailView;       // 数据点详情区
};

#endif // VISUALIZATIONDOCKWIDGET_H_C8D9E0F1A2B3
