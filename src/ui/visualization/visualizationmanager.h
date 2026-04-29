#ifndef VISUALIZATIONMANAGER_H_D9E0F1A2B3C4
#define VISUALIZATIONMANAGER_H_D9E0F1A2B3C4

#include <QObject>
#include <QString>

#include "model/dto/analysisresult.h"
#include "model/dto/visualizationpoint.h"

class AnalysisWorkspaceDockWidget;
class BarChartWidget;
class LineChartWidget;

// ════════════════════════════════════════════════════════
//  VisualizationManager
//  可视化协调器
//  负责把 AnalysisResult 中的结构化图表 DTO 分发到统一分析工作区的结果页，
//  并维护图表详情区文本
// ════════════════════════════════════════════════════════
class VisualizationManager : public QObject
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数
     * @param_1 _pParent: 父对象指针
     */
    explicit VisualizationManager(QObject* _pParent = nullptr);

    /*
     * @brief 析构函数
     */
    ~VisualizationManager() override = default;

    /*
     * @brief 绑定统一分析工作区中的结果展示宿主
     * @param_1 _pWorkspaceDock: 分析工作区 Dock
     */
    void attachWorkspace(AnalysisWorkspaceDockWidget* _pWorkspaceDock);

    /*
     * @brief 使用分析结果刷新图表展示
     * @param_1 _result: 分析结果 DTO
     */
    void updateFromResult(const AnalysisResult& _result);

    /*
     * @brief 清空当前图表并显示占位文本
     * @param_1 _strMessage: 占位或错误提示文本
     */
    void clearView(const QString& _strMessage = QString());

private slots:
    /*
     * @brief 响应图表点位点击，刷新详情区
     * @param_1 _point: 当前被点击的数据点
     */
    void onPointSelected(const VisualizationPoint& _point);

private:
    /*
     * @brief 构造默认摘要详情文本
     * @param_1 _result: 分析结果 DTO
     */
    QString buildSummaryText(const AnalysisResult& _result) const;

    /*
     * @brief 构造当前点击点位的详情文本
     * @param_1 _point: 当前被点击的数据点
     */
    QString buildPointDetailText(const VisualizationPoint& _point) const;

    AnalysisWorkspaceDockWidget* mpWorkspaceDock;  // 当前绑定的分析工作区
    BarChartWidget*          mpctrlBarChartWidget; // 柱状图控件
    LineChartWidget*         mpctrlLineChartWidget; // 折线图控件
    AnalysisResult           mResultCurrent;       // 当前已展示的分析结果
};

#endif // VISUALIZATIONMANAGER_H_D9E0F1A2B3C4
