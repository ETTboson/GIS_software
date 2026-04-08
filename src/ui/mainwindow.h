#ifndef MAINWINDOW_H_A1B2C3D4E5F6
#define MAINWINDOW_H_A1B2C3D4E5F6

#include <QMainWindow>
#include <QLabel>
#include <QDockWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QTableView>

#include <qgslayertreeview.h>

#include "model/layerinfo.h"
#include "model/analysisresult.h"
#include "model/maptooltype.h"

class DataService;
class AnalysisService;
class AIManager;
class ImageButton;
class AIDockWidget;
class MapCanvasManager;
class MapCanvasWidget;
class QComboBox;
class QSpinBox;
class QPushButton;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
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

private:
    void initModules();
    void initRibbonButtons();
    void initDockWidgets();
    void initStatusBar();
    void initConnections();
    void reconnectCanvasSignals(MapCanvasWidget* _pCanvas);
    void createLayerDock();
    void createAIDock();
    void createAttributeDock();
    void createAnalysisDock();
    void createLogDock();

    /*
     * @brief 根据当前界面选择执行一次数据分析
     */
    void runSelectedAnalysis();

    /*
     * @brief 根据分析方法更新参数控件可用状态
     */
    void updateAnalysisParameterWidgets();

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
    void onDataLoaded(const LayerInfo& _layerInfo);
    void onDataLoadFailed(const QString& _strErrorMsg);
    void onAnalysisFinished(const AnalysisResult& _result);
    void onAnalysisFailed(const AnalysisResult& _result);
    void onAnalysisProgress(int _nPercent);
    void onCoordChanged(double _dLon, double _dLat);
    void onScaleChanged(double _dScale);
    void onActiveCanvasChanged(MapCanvasWidget* _pCanvas);

    /*
     * @brief 分析方法切换时更新参数区
     * @param_1 _nIndex: 当前选中方法索引
     */
    void onAnalysisMethodChanged(int _nIndex);

    /*
     * @brief 用户点击“执行分析”按钮时触发
     */
    void onRunAnalysisClicked();

private:
    Ui::MainWindow* mpUI;
    DataService* mpDataService;
    AnalysisService* mpAnalysisService;
    AIManager* mpAIManager;
    MapCanvasManager* mpMapCanvasManager;
    QDockWidget* mpctrlDockLayer;
    AIDockWidget* mpctrlDockAI;
    QDockWidget* mpctrlDockAttribute;
    QDockWidget* mpctrlDockAnalysis;
    QDockWidget* mpctrlDockLog;
    QgsLayerTreeView* mpctrlLayerTreeView;
    QTableView* mpctrlAttrTable;
    QTextEdit* mpctrlLogView;
    QLabel* mpctrlLabelCoord;
    QLabel* mpctrlLabelScale;
    QLabel* mpctrlLabelStatus;

    QLabel* mpctrlLabelAnalysisData;          // 当前可分析数据提示
    QComboBox* mpctrlComboAnalysisMethod;     // 分析方法下拉框
    QSpinBox* mpctrlSpinFrequencyBins;        // 频率统计分箱数
    QSpinBox* mpctrlSpinNeighborhoodWindow;   // 邻域分析窗口大小
    QPushButton* mpctrlBtnRunAnalysis;        // 执行分析按钮
    QTextEdit* mpctrlAnalysisResultView;      // 分析结果展示区
};

#endif // MAINWINDOW_H_A1B2C3D4E5F6
