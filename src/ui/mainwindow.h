#ifndef MAINWINDOW_H_A1B2C3D4E5F6
#define MAINWINDOW_H_A1B2C3D4E5F6

#include <QJsonObject>
#include <QMainWindow>
#include <QMap>
#include <QPoint>

#include <qgslayertreeview.h>

#include "core/interfaces/iaitoolhost.h"
#include "model/dto/analysisdataasset.h"
#include "model/dto/analysisresult.h"
#include "model/dto/layerinfo.h"
#include "model/enums/dataassettype.h"
#include "model/enums/maptooltype.h"

class AIManager;
class AIDockWidget;
class QAction;
class AnalysisWorkspaceDockWidget;
class AttributeQueryService;
class DataRepository;
class DataService;
class ImageButton;
class QLabel;
class MapCanvasManager;
class MapCanvasWidget;
class QDockWidget;
class QTableView;
class QTextEdit;
class QgsMapLayer;
class SpatialAnalysisService;
class StatisticalAnalysisService;
class VisualizationManager;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// ════════════════════════════════════════════════════════
//  MainWindow
//  职责：应用主窗口与 UI 总装配层。
//        负责组织 Ribbon、地图区域、统一分析工作区、AI Dock 与日志面板，
//        并作为 IAIToolHost 将当前分析资产与已实现分析能力桥接给 AI 模块。
//  位于 ui/ 根层，只负责交互编排与展示，不直接承载算法实现。
// ════════════════════════════════════════════════════════
class MainWindow : public QMainWindow, public IAIToolHost
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数，按顺序完成模块初始化、UI 搭建、信号连接
     * @param_1 _pParent: 父对象指针，通常为 nullptr
     */
    explicit MainWindow(QWidget* _pParent = nullptr);

    /*
     * @brief 析构函数，释放模块与 UI 对象
     */
    ~MainWindow();

    /*
     * @brief 返回当前分析上下文快照
     */
    QJsonObject getAnalysisContext() const override;

    /*
     * @brief 执行 AI 分析工具
     * @param_1 _strToolName: 工具名称
     * @param_2 _jsonArgs: 工具参数
     * @param_3 _strResult: 成功时结果文本
     * @param_4 _strError: 失败时错误文本
     */
    bool executeAnalysisTool(const QString& _strToolName,
        const QJsonObject& _jsonArgs,
        QString& _strResult,
        QString& _strError) override;

private:
    struct AnalysisRunConfig
    {
        QString strToolId;            // 工具标识
        int     nFrequencyBins = 10;  // 频率统计分箱数
        int     nNeighborhoodWindow = 3; // 邻域窗口大小
    };

    void initModules();
    void initRibbonButtons();
    void initDockWidgets();
    void initStatusBar();
    void initConnections();
    void reconnectCanvasSignals(MapCanvasWidget* _pCanvas);
    void createLayerDock();
    void createAIDock();
    void createAttributeDock();
    void createAnalysisWorkspaceDock();
    void createLogDock();
    /*
     * @brief 初始化图层树右键菜单行为
     */
    void initLayerTreeContextMenu();
    void ensureWorkspaceVisible();
    /*
     * @brief 返回图层树中当前选中的图层对象
     */
    QgsMapLayer* currentSelectedLayer() const;
    DataAssetType resolveAssetChoice(const AnalysisDataAsset& _assetInput);
    void openToolShortcut(const QString& _strToolId);
    bool assetSupportsTool(const AnalysisDataAsset& _assetInput,
        const QString& _strToolId) const;
    void runToolForCurrentAsset(const AnalysisRunConfig& _configRun);
    void runToolForAsset(const AnalysisDataAsset& _assetInput,
        const AnalysisRunConfig& _configRun);
    void cachePendingRun(const QString& _strAssetId,
        const AnalysisRunConfig& _configRun);
    void clearPendingRun();

private slots:
    void onOpenData();
    void onAddLayer();
    void onSaveProject();
    void onExportResult();
    void onNavPan();
    void onNavZoomIn();
    void onNavZoomOut();
    void onNavFitAll();
    void onBufferAnalysis();
    void onOverlayAnalysis();
    void onSpatialQuery();
    void onRasterCalc();
    void onAIAnalyze();
    void onAIChat();
    void onAbout();
    void onLayerLoaded(const LayerInfo& _layerInfo);
    void onAnalysisAssetReady(const AnalysisDataAsset& _assetReady);
    void onDataLoadFailed(const QString& _strErrorMsg);
    void onAnalysisFinished(const AnalysisResult& _result);
    void onAnalysisFailed(const AnalysisResult& _result);
    void onAnalysisProgress(int _nPercent);
    void onCoordChanged(double _dLon, double _dLat);
    void onScaleChanged(double _dScale);
    void onActiveCanvasChanged(MapCanvasWidget* _pCanvas);
    void onCurrentAnalysisAssetChanged(const AnalysisDataAsset& _assetCurrent);
    void onCurrentAnalysisAssetCleared();
    /*
     * @brief 响应图层树右键菜单请求
     * @param_1 _posMenu: 右键菜单在图层树视图坐标系中的位置
     */
    void onLayerTreeContextMenuRequested(const QPoint& _posMenu);
    /*
     * @brief 移除图层树当前选中的图层
     */
    void onRemoveSelectedLayer();
    /*
     * @brief 将活动画布缩放到当前选中图层范围
     */
    void onZoomToSelectedLayer();
    void onBasicStatisticsRequested();
    void onFrequencyStatisticsRequested(int _nFrequencyBins);
    void onNeighborhoodAnalysisRequested(int _nNeighborhoodWindow);
    void onAttributeQueryRequested();

private:
    Ui::MainWindow*              mpUI;
    DataService*                 mpDataService;
    DataRepository*              mpDataRepository;
    StatisticalAnalysisService*  mpStatisticalAnalysisService;
    SpatialAnalysisService*      mpSpatialAnalysisService;
    AttributeQueryService*       mpAttributeQueryService;
    AIManager*                   mpAIManager;
    MapCanvasManager*            mpMapCanvasManager;
    QDockWidget*                 mpctrlDockLayer;
    AIDockWidget*                mpctrlDockAI;
    QDockWidget*                 mpctrlDockAttribute;
    AnalysisWorkspaceDockWidget* mpctrlDockAnalysisWorkspace;
    QDockWidget*                 mpctrlDockLog;
    QgsLayerTreeView*            mpctrlLayerTreeView;
    QTableView*                  mpctrlAttrTable;
    QTextEdit*                   mpctrlLogView;
    QLabel*                      mpctrlLabelCoord;
    QLabel*                      mpctrlLabelScale;
    QLabel*                      mpctrlLabelStatus;
    VisualizationManager*        mpVisualizationManager;

    bool                         mbHasPendingRun;          // 当前是否存在待确认的运行配置
    QString                      mstrPendingRunAssetId;    // 待确认运行配置对应的资产 ID
    AnalysisRunConfig            mConfigPendingRun;        // 待确认运行配置
    QMap<QString, AnalysisRunConfig> mmapLastSuccessfulRuns; // 按资产缓存的最近成功分析配置
    QString                      mstrDisplayedResultAssetId; // 当前结果区正在显示的资产 ID
};

#endif // MAINWINDOW_H_A1B2C3D4E5F6
