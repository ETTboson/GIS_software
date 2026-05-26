#ifndef ANALYSISWORKSPACEDOCKWIDGET_H_C9D0E1F2A3B4
#define ANALYSISWORKSPACEDOCKWIDGET_H_C9D0E1F2A3B4

#include <QDockWidget>
#include <QPoint>

#include "model/dto/analysisdataasset.h"
#include "model/dto/analysisresult.h"
#include "model/enums/overlayoperationtype.h"
#include "model/enums/visualizationcharttype.h"

class DataRepository;
class AntButton;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QSpinBox;
class QStackedWidget;
class QTabWidget;
class QTextEdit;
class QWidget;

// ════════════════════════════════════════════════════════
//  AnalysisWorkspaceDockWidget
//  统一分析工作区 Dock
//  固定包含 Data、Tools、Results 三页，
//  负责展示资产列表、能力工具入口、文本结果、图表结果与历史记录。
// ════════════════════════════════════════════════════════
class AnalysisWorkspaceDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数
     * @param_1 _pParent: 父控件指针
     */
    explicit AnalysisWorkspaceDockWidget(QWidget* _pParent = nullptr);

    /*
     * @brief 析构函数
     */
    ~AnalysisWorkspaceDockWidget() override = default;

    /*
     * @brief 绑定统一分析资产仓库
     * @param_1 _pRepository: 分析资产仓库
     */
    void setDataRepository(DataRepository* _pRepository);

    /*
     * @brief 切换到 Data 页
     * @param_1 _strHint: 可选提示文本
     */
    void showDataPage(const QString& _strHint = QString());

    /*
     * @brief 切换到 Tools 页
     * @param_1 _strHint: 可选提示文本
     */
    void showToolsPage(const QString& _strHint = QString());

    /*
     * @brief 切换到 Results 页
     */
    void showResultsPage();

    /*
     * @brief 聚焦指定工具并切换到 Tools 页
     * @param_1 _strToolId: 工具标识
     * @param_2 _strHint: 工具区提示文本
     */
    void focusTool(const QString& _strToolId, const QString& _strHint = QString());

    /*
     * @brief 更新当前结果文本区域
     * @param_1 _result: 最新分析结果
     */
    void setCurrentResult(const AnalysisResult& _result);

    /*
     * @brief 清空当前结果展示区
     * @param_1 _strMessage: 占位或提示文本
     */
    void clearCurrentResult(const QString& _strMessage);

    /*
     * @brief 向历史列表追加一条结果记录
     * @param_1 _result: 需要追加的分析结果
     */
    void addResultHistory(const AnalysisResult& _result);

    /*
     * @brief 为指定图表类型注册展示控件
     * @param_1 _chartType: 图表类型
     * @param_2 _pWidget: 图表控件
     */
    void setChartWidget(VisualizationChartType _chartType, QWidget* _pWidget);

    /*
     * @brief 切换到指定图表页面
     * @param_1 _chartType: 图表类型
     * @param_2 _strTitle: 图表标题
     */
    void showChart(VisualizationChartType _chartType, const QString& _strTitle);

    /*
     * @brief 切换到图表占位页面
     * @param_1 _strMessage: 占位提示文本
     */
    void showVisualizationPlaceholder(const QString& _strMessage);

    /*
     * @brief 更新图表详情区文本
     * @param_1 _strDetailText: 详情文本
     */
    void setVisualizationDetailText(const QString& _strDetailText);

    /*
     * @brief 只更新 Data 页顶部提示文本，不切换当前页签
     * @param_1 _strHint: 新的提示文本
     */
    void setDataHint(const QString& _strHint);

signals:
    /*
     * @brief 用户请求执行基础统计
     */
    void basicStatisticsRequested();

    /*
     * @brief 用户请求执行频率统计
     * @param_1 _nBinCount: 分箱数
     */
    void frequencyStatisticsRequested(int _nBinCount);

    /*
     * @brief 用户请求执行邻域分析
     * @param_1 _nWindowSize: 邻域窗口大小
     */
    void neighborhoodAnalysisRequested(int _nWindowSize);

    /*
     * @brief 用户请求执行缓冲区分析
     * @param_1 _dDistance: 缓冲距离，单位为源图层 CRS 单位
     * @param_2 _nSegments: 圆弧分段数
     */
    void bufferAnalysisRequested(double _dDistance, int _nSegments);

    /*
     * @brief 用户请求执行矢量叠加分析
     * @param_1 _strOverlayAssetId: 第二个叠加矢量资产 ID
     * @param_2 _eOperation: 叠加操作类型
     */
    void overlayAnalysisRequested(const QString& _strOverlayAssetId,
        OverlayOperationType _eOperation);

    /*
     * @brief 用户请求执行空间关系查询
     * @param_1 _strTargetAssetId: 指定区域矢量资产 ID
     * @param_2 _strRelationId: 空间关系标识
     */
    void spatialQueryRequested(const QString& _strTargetAssetId,
        const QString& _strRelationId);

    /*
     * @brief 用户请求执行属性查询
     * @param_1 _strFieldName: 字段名
     * @param_2 _strOperatorId: 运算符标识
     * @param_3 _strValueText: 查询值文本
     */
    void attributeQueryRequested(const QString& _strFieldName,
        const QString& _strOperatorId,
        const QString& _strValueText);

    /*
     * @brief 用户请求删除指定分析资产记录
     * @param_1 _strAssetId: 目标分析资产 ID
     */
    void analysisAssetDeleteRequested(const QString& _strAssetId);

private slots:
    void onAssetListCurrentItemChanged(QListWidgetItem* _pCurrent,
        QListWidgetItem* _pPrevious);
    void onAssetListContextMenuRequested(const QPoint& _posMenu);
    void onCurrentAssetChanged(const AnalysisDataAsset& _assetCurrent);
    void onCurrentAssetCleared();
    void onHistoryItemCurrentRowChanged(int _nCurrentRow);
    void onToolSelectorCurrentIndexChanged(int _nCurrentIndex);

private:
    /*
     * @brief 刷新 Data 页资产列表
     */
    void refreshAssetList();

    /*
     * @brief 根据当前资产刷新摘要与工具状态
     * @param_1 _assetCurrent: 当前选中的资产
     */
    void updateCurrentAssetView(const AnalysisDataAsset& _assetCurrent);

    /*
     * @brief 按当前资产能力更新工具按钮状态
     * @param_1 _assetCurrent: 当前选中的资产
     */
    void updateToolStates(const AnalysisDataAsset& _assetCurrent);

    /*
     * @brief 刷新叠加分析第二资产下拉框
     * @param_1 _assetCurrent: 当前源资产
     */
    void refreshOverlayAssetOptions(const AnalysisDataAsset& _assetCurrent);

    /*
     * @brief 刷新属性查询字段下拉框
     * @param_1 _assetCurrent: 当前源资产
     */
    void refreshAttributeFieldOptions(const AnalysisDataAsset& _assetCurrent);

    /*
     * @brief 刷新空间查询目标区域资产下拉框
     * @param_1 _assetCurrent: 当前源资产
     */
    void refreshSpatialQueryTargetOptions(const AnalysisDataAsset& _assetCurrent);

    /*
     * @brief 按工具标识选择 Tools 参数页
     * @param_1 _strToolId: 工具标识
     */
    void selectToolPage(const QString& _strToolId);

    /*
     * @brief 返回图表类型对应的容器页
     * @param_1 _chartType: 图表类型
     */
    QWidget* pageForChartType(VisualizationChartType _chartType) const;

    /*
     * @brief 根据工具标识返回展示名称
     * @param_1 _strToolId: 工具标识
     */
    static QString ToolDisplayName(const QString& _strToolId);

    /*
     * @brief 判断资产是否适合作为空间查询区域目标
     * @param_1 _assetInput: 候选分析资产
     */
    static bool IsAreaVectorAsset(const AnalysisDataAsset& _assetInput);

    DataRepository* mpDataRepository;     // 统一分析资产仓库
    QWidget*        mpctrlContainer;      // Dock 主容器
    QTabWidget*     mpctrlTabWidget;      // 三页工作区容器

    QWidget*        mpctrlPageData;       // Data 页
    QLabel*         mpctrlLabelDataHint;  // Data 页提示标签
    QListWidget*    mpctrlAssetList;      // 资产列表
    QTextEdit*      mpctrlAssetSummaryView; // 资产摘要视图

    QWidget*        mpctrlPageTools;      // Tools 页
    QLabel*         mpctrlLabelToolsHint; // Tools 页提示标签
    QComboBox*      mpctrlComboToolSelector; // 分析方法选择框
    QStackedWidget* mpctrlToolStack;      // 分析方法参数页堆栈
    AntButton*      mpctrlBtnBasicStatistics; // 基础统计按钮
    AntButton*      mpctrlBtnFrequencyStatistics; // 频率统计按钮
    QSpinBox*       mpctrlSpinFrequencyBins; // 频率统计分箱数
    AntButton*      mpctrlBtnNeighborhood; // 邻域分析按钮
    QSpinBox*       mpctrlSpinNeighborhoodWindow; // 邻域窗口大小
    QDoubleSpinBox* mpctrlSpinBufferDistance; // 缓冲距离
    QSpinBox*       mpctrlSpinBufferSegments; // 缓冲圆弧分段数
    AntButton*      mpctrlBtnBuffer;      // 缓冲分析按钮
    QComboBox*      mpctrlComboOverlayOperation; // 叠加操作类型选择框
    QComboBox*      mpctrlComboOverlayTarget; // 叠加分析第二矢量资产选择框
    AntButton*      mpctrlBtnOverlay;     // 叠加分析按钮
    QComboBox*      mpctrlComboSpatialRelation; // 空间查询关系选择框
    QComboBox*      mpctrlComboSpatialTarget; // 空间查询指定区域资产选择框
    AntButton*      mpctrlBtnSpatialQuery; // 空间查询按钮
    AntButton*      mpctrlBtnRasterCalc;  // 栅格计算占位按钮
    QComboBox*      mpctrlComboAttributeField; // 属性查询字段选择框
    QComboBox*      mpctrlComboAttributeOperator; // 属性查询运算符选择框
    QLineEdit*      mpctrlEditAttributeValue; // 属性查询值输入框
    AntButton*      mpctrlBtnAttributeQuery; // 属性查询按钮

    QWidget*        mpctrlPageResults;    // Results 页
    QTextEdit*      mpctrlResultView;     // 文本结果区
    QLabel*         mpctrlLabelChartTitle; // 图表标题
    QStackedWidget* mpctrlChartStack;     // 图表堆栈
    QWidget*        mpctrlPagePlaceholder; // 图表占位页
    QWidget*        mpctrlPageBarChart;   // 柱状图页
    QWidget*        mpctrlPageLineChart;  // 折线图页
    QLabel*         mpctrlLabelPlaceholder; // 图表占位文本
    QTextEdit*      mpctrlVisualizationDetailView; // 图表详情区
    QListWidget*    mpctrlHistoryList;    // 结果历史列表

    AnalysisResult  mResultCurrent;       // 当前展示中的结果
};

#endif // ANALYSISWORKSPACEDOCKWIDGET_H_C9D0E1F2A3B4
