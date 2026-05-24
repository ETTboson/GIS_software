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
#include "model/enums/overlayoperationtype.h"

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
        double  dBufferDistance = 100.0; // 缓冲距离，单位为源图层 CRS 单位
        int     nBufferSegments = 8; // 缓冲圆弧分段数
        QString strOverlayAssetId;    // 叠加分析第二矢量资产 ID
        OverlayOperationType eOverlayOperation = OverlayOperationType::Intersect; // 叠加操作类型
        QString strQueryFieldName;    // 属性查询字段名
        QString strQueryOperatorId;   // 属性查询运算符标识
        QString strQueryValueText;    // 属性查询值文本
        QString strSpatialTargetAssetId; // 空间查询指定区域资产 ID
        QString strSpatialRelationId; // 空间查询关系标识
        QString strSourceAssetId;      // AI 工具指定的源资产 ID，可为空表示当前资产
        QString strProximityReferenceAssetId; // 邻近查询参考资产 ID
        bool    bProximityInvertMatch = false; // 邻近查询是否返回不在距离内的要素
        QString strSourceSubsetExpression; // 源图层可选子集筛选表达式
        QString strProximityReferenceSubsetExpression; // 参考图层可选子集筛选表达式
        double  dProximityDistance = 50.0; // 邻近查询距离，单位为米
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
    /*
     * @brief 按 QGIS 图层 ID 选中图层树中的图层
     * @param_1 _strLayerId: 目标图层 ID
     */
    void selectLayerById(const QString& _strLayerId);
    DataAssetType resolveAssetChoice(const AnalysisDataAsset& _assetInput);
    void openToolShortcut(const QString& _strToolId);
    bool assetSupportsTool(const AnalysisDataAsset& _assetInput,
        const QString& _strToolId) const;
    void runToolForCurrentAsset(const AnalysisRunConfig& _configRun);
    void runToolForAsset(const AnalysisDataAsset& _assetInput,
        const AnalysisRunConfig& _configRun);
    /*
     * @brief 按资产 ID 或演示别名查找分析资产
     * @param_1 _strAssetRef: 资产 ID、名称片段或演示别名
     */
    AnalysisDataAsset findAssetByIdOrAlias(const QString& _strAssetRef) const;
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
    /*
     * @brief 新建独立 3D DEM 视图窗口
     */
    void onNew3DView();
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
    /*
     * @brief 为图层树当前选中矢量图层应用简单符号样式
     */
    void onApplySimpleSymbol();
    /*
     * @brief 为图层树当前选中矢量图层应用字段分类/分级渲染
     */
    void onApplyFieldRenderer();
    /*
     * @brief 为图层树当前选中栅格图层应用 2%-98% 百分位灰度拉伸
     */
    void onApplyRasterGrayRenderer();
    /*
     * @brief 将图层树当前选中栅格图层恢复为默认灰度渲染
     */
    void onApplyRasterDefaultRenderer();
    /*
     * @brief 为图层树当前选中栅格图层应用伪彩色渲染
     */
    void onApplyRasterPseudoColorRenderer();
    void onBasicStatisticsRequested();
    void onFrequencyStatisticsRequested(int _nFrequencyBins);
    void onNeighborhoodAnalysisRequested(int _nNeighborhoodWindow);
    void onBufferAnalysisRequested(double _dBufferDistance, int _nBufferSegments);
    void onOverlayAnalysisRequested(const QString& _strOverlayAssetId,
        OverlayOperationType _eOperation);
    void onSpatialQueryRequested(const QString& _strTargetAssetId,
        const QString& _strRelationId);
    void onAttributeQueryRequested(const QString& _strFieldName,
        const QString& _strOperatorId,
        const QString& _strValueText);

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
    AnalysisResult              mResultLastSuccessful; // 最近一次成功分析结果，供导出使用
};

#endif // MAINWINDOW_H_A1B2C3D4E5F6
