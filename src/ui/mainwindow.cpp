#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "core/ai/aimanager.h"
#include "service/analysis/attributequeryservice.h"
#include "service/analysis/spatialanalysisservice.h"
#include "service/analysis/statisticalanalysisservice.h"
#include "service/data/datarepository.h"
#include "service/data/dataservice.h"
#include "ui/components/imagebutton.h"
#include "ui/docks/aidockwidget.h"
#include "ui/docks/analysisworkspacedockwidget.h"
#include "ui/map/mapcanvasmanager.h"
#include "ui/map/mapcanvaswidget.h"
#include "ui/visualization/visualizationmanager.h"

#include <QAbstractButton>
#include <QAction>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QStringList>
#include <QTableView>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>

#include <qgslayertreemodel.h>
#include <qgslayertreemapcanvasbridge.h>
#include <qgsmaplayer.h>
#include <qgsproject.h>

namespace
{

QString dataAssetTypeDisplayName(DataAssetType _eAssetType)
{
    switch (_eAssetType) {
    case DataAssetType::Raster:
        return QObject::tr("栅格资产");
    case DataAssetType::Vector:
        return QObject::tr("矢量资产");
    case DataAssetType::Table:
        return QObject::tr("表格资产");
    case DataAssetType::MetadataDocument:
        return QObject::tr("元数据文档");
    case DataAssetType::None:
    default:
        return QObject::tr("未指定资产");
    }
}

QString toolDisplayName(const QString& _strToolId)
{
    if (_strToolId == "basic_statistics") {
        return QObject::tr("基础统计");
    }
    if (_strToolId == "frequency_statistics") {
        return QObject::tr("频率统计");
    }
    if (_strToolId == "neighborhood_analysis") {
        return QObject::tr("邻域分析");
    }
    if (_strToolId == "attribute_query") {
        return QObject::tr("属性查询");
    }
    if (_strToolId == "buffer_analysis") {
        return QObject::tr("缓冲分析");
    }
    if (_strToolId == "overlay_analysis") {
        return QObject::tr("叠加分析");
    }
    if (_strToolId == "spatial_query") {
        return QObject::tr("空间查询");
    }
    if (_strToolId == "raster_calc") {
        return QObject::tr("栅格计算");
    }
    return _strToolId;
}

QJsonArray capabilityArrayToJson(AnalysisCapabilities _flagsCapabilities)
{
    QJsonArray _jsonCaps;
    if (_flagsCapabilities.testFlag(AnalysisCapability::Statistical)) {
        _jsonCaps.append("statistical");
    }
    if (_flagsCapabilities.testFlag(AnalysisCapability::SpatialRaster)) {
        _jsonCaps.append("spatial_raster");
    }
    if (_flagsCapabilities.testFlag(AnalysisCapability::SpatialVector)) {
        _jsonCaps.append("spatial_vector");
    }
    if (_flagsCapabilities.testFlag(AnalysisCapability::AttributeQuery)) {
        _jsonCaps.append("attribute_query");
    }
    return _jsonCaps;
}

QString capabilityText(AnalysisCapabilities _flagsCapabilities)
{
    QStringList _vCaps;
    if (_flagsCapabilities.testFlag(AnalysisCapability::Statistical)) {
        _vCaps << QObject::tr("统计分析");
    }
    if (_flagsCapabilities.testFlag(AnalysisCapability::SpatialRaster)) {
        _vCaps << QObject::tr("栅格空间分析");
    }
    if (_flagsCapabilities.testFlag(AnalysisCapability::SpatialVector)) {
        _vCaps << QObject::tr("矢量空间分析");
    }
    if (_flagsCapabilities.testFlag(AnalysisCapability::AttributeQuery)) {
        _vCaps << QObject::tr("属性查询");
    }
    return _vCaps.isEmpty() ? QObject::tr("无") : _vCaps.join(", ");
}

QJsonObject buildAssetContextObject(const AnalysisDataAsset& _assetInput)
{
    QJsonObject _jsonAsset;
    _jsonAsset["id"] = _assetInput.strAssetId;
    _jsonAsset["name"] = _assetInput.strName;
    _jsonAsset["source_path"] = _assetInput.strSourcePath;
    _jsonAsset["source_format"] = _assetInput.strSourceFormat;
    _jsonAsset["asset_type"] = dataAssetTypeDisplayName(_assetInput.eAssetType);
    _jsonAsset["capabilities"] = capabilityArrayToJson(_assetInput.flagsCapabilities);
    _jsonAsset["summary"] = _assetInput.strSummary;
    _jsonAsset["has_numeric_data"] = _assetInput.bHasNumericDataset;
    _jsonAsset["numeric_rows"] = _assetInput.dataNumeric.nRows;
    _jsonAsset["numeric_cols"] = _assetInput.dataNumeric.nCols;
    return _jsonAsset;
}

} // namespace

MainWindow::MainWindow(QWidget* _pParent)
    : QMainWindow(_pParent)
    , mpUI(new Ui::MainWindow)
    , mpDataService(nullptr)
    , mpDataRepository(nullptr)
    , mpStatisticalAnalysisService(nullptr)
    , mpSpatialAnalysisService(nullptr)
    , mpAttributeQueryService(nullptr)
    , mpAIManager(nullptr)
    , mpMapCanvasManager(nullptr)
    , mpctrlDockLayer(nullptr)
    , mpctrlDockAI(nullptr)
    , mpctrlDockAttribute(nullptr)
    , mpctrlDockAnalysisWorkspace(nullptr)
    , mpctrlDockLog(nullptr)
    , mpctrlLayerTreeView(nullptr)
    , mpctrlAttrTable(nullptr)
    , mpctrlLogView(nullptr)
    , mpctrlLabelCoord(nullptr)
    , mpctrlLabelScale(nullptr)
    , mpctrlLabelStatus(nullptr)
    , mpVisualizationManager(nullptr)
    , mbHasPendingRun(false)
{
    mpUI->setupUi(this);

    initModules();
    initRibbonButtons();
    initDockWidgets();
    initStatusBar();
    initConnections();
}

MainWindow::~MainWindow()
{
    delete mpDataService;
    mpDataService = nullptr;

    delete mpDataRepository;
    mpDataRepository = nullptr;

    delete mpStatisticalAnalysisService;
    mpStatisticalAnalysisService = nullptr;

    delete mpSpatialAnalysisService;
    mpSpatialAnalysisService = nullptr;

    delete mpAttributeQueryService;
    mpAttributeQueryService = nullptr;

    delete mpAIManager;
    mpAIManager = nullptr;

    delete mpMapCanvasManager;
    mpMapCanvasManager = nullptr;

    delete mpVisualizationManager;
    mpVisualizationManager = nullptr;

    delete mpUI;
    mpUI = nullptr;
}

void MainWindow::initModules()
{
    mpDataService = new DataService(this);
    mpDataRepository = new DataRepository(this);
    mpStatisticalAnalysisService = new StatisticalAnalysisService(this);
    mpSpatialAnalysisService = new SpatialAnalysisService(this);
    mpAttributeQueryService = new AttributeQueryService(this);
    mpAIManager = new AIManager(this);
    mpMapCanvasManager = new MapCanvasManager(this);
    mpVisualizationManager = new VisualizationManager(this);

    mpAIManager->setToolHost(this);

    MapCanvasWidget* _pCanvas = mpMapCanvasManager->createCanvas(mpUI->wgtMapCanvas);

    QVBoxLayout* _pLayout = new QVBoxLayout(mpUI->wgtMapCanvas);
    _pLayout->setContentsMargins(0, 0, 0, 0);
    _pLayout->setSpacing(0);
    _pLayout->addWidget(_pCanvas);
    mpUI->wgtMapCanvas->setLayout(_pLayout);
}

QJsonObject MainWindow::getAnalysisContext() const
{
    QJsonObject _jsonContext;
    _jsonContext["has_current_asset"] = (mpDataRepository != nullptr)
        ? mpDataRepository->hasCurrentAsset()
        : false;
    _jsonContext["has_numeric_data"] = (mpDataRepository != nullptr
        && mpDataRepository->hasCurrentAsset())
        ? mpDataRepository->currentAsset().bHasNumericDataset
        : false;

    if (mpDataRepository != nullptr && mpDataRepository->hasCurrentAsset()) {
        const AnalysisDataAsset _assetCurrent = mpDataRepository->currentAsset();
        _jsonContext["selected_asset_id"] = _assetCurrent.strAssetId;
        _jsonContext["current_asset"] = buildAssetContextObject(_assetCurrent);
        _jsonContext["ready_data_path"] = _assetCurrent.strSourcePath;
    }

    QJsonArray _jsonAssets;
    if (mpDataRepository != nullptr) {
        const QList<AnalysisDataAsset> _vAssets = mpDataRepository->getAssets();
        for (const AnalysisDataAsset& _assetCurrent : _vAssets) {
            _jsonAssets.append(buildAssetContextObject(_assetCurrent));
        }
    }
    _jsonContext["assets"] = _jsonAssets;

    QJsonArray _jsonLayers;
    if (mpDataService != nullptr) {
        const QList<LayerInfo> _vLayers = mpDataService->getLayers();
        for (const LayerInfo& _layerInfo : _vLayers) {
            QJsonObject _jsonLayer;
            _jsonLayer["name"] = _layerInfo.strName;
            _jsonLayer["type"] = _layerInfo.strType;
            _jsonLayer["file_path"] = _layerInfo.strFilePath;
            _jsonLayer["visible"] = _layerInfo.bVisible;
            _jsonLayers.append(_jsonLayer);
        }
    }
    _jsonContext["layers"] = _jsonLayers;
    return _jsonContext;
}

bool MainWindow::executeAnalysisTool(const QString& _strToolName,
    const QJsonObject& _jsonArgs,
    QString& _strResult,
    QString& _strError)
{
    _strResult.clear();
    _strError.clear();

    if (mpDataRepository == nullptr || !mpDataRepository->hasCurrentAsset()) {
        _strError = tr("当前没有已选择的分析资产，请先导入并选择数据");
        return false;
    }

    AnalysisRunConfig _configRun;
    if (_strToolName == "run_basic_statistics") {
        _configRun.strToolId = "basic_statistics";
    } else if (_strToolName == "run_frequency_statistics") {
        const int _nBinCount = _jsonArgs["bin_count"].toInt(0);
        if (_nBinCount < 2) {
            _strError = tr("参数错误：bin_count 必须大于等于 2");
            return false;
        }
        _configRun.strToolId = "frequency_statistics";
        _configRun.nFrequencyBins = _nBinCount;
    } else if (_strToolName == "run_neighborhood_analysis") {
        const int _nWindowSize = _jsonArgs["window_size"].toInt(0);
        if (_nWindowSize < 3 || (_nWindowSize % 2) == 0) {
            _strError = tr("参数错误：window_size 必须是不小于 3 的奇数");
            return false;
        }
        _configRun.strToolId = "neighborhood_analysis";
        _configRun.nNeighborhoodWindow = _nWindowSize;
    } else {
        _strError = tr("未知分析工具：%1").arg(_strToolName);
        return false;
    }

    const AnalysisDataAsset _assetCurrent = mpDataRepository->currentAsset();
    if (!assetSupportsTool(_assetCurrent, _configRun.strToolId)) {
        _strError = tr("当前资产“%1”不支持工具“%2”")
            .arg(_assetCurrent.strName, toolDisplayName(_configRun.strToolId));
        return false;
    }

    AnalysisResult _resultCaptured;
    bool _bCaptured = false;
    auto _captureResult = [&](const AnalysisResult& _resultCurrent) {
        if (_bCaptured) {
            return;
        }
        _resultCaptured = _resultCurrent;
        _bCaptured = true;
    };

    const QMetaObject::Connection _okConnStat = connect(
        mpStatisticalAnalysisService,
        &StatisticalAnalysisService::analysisFinished,
        this,
        _captureResult);
    const QMetaObject::Connection _failConnStat = connect(
        mpStatisticalAnalysisService,
        &StatisticalAnalysisService::analysisFailed,
        this,
        _captureResult);
    const QMetaObject::Connection _okConnSpatial = connect(
        mpSpatialAnalysisService,
        &SpatialAnalysisService::analysisFinished,
        this,
        _captureResult);
    const QMetaObject::Connection _failConnSpatial = connect(
        mpSpatialAnalysisService,
        &SpatialAnalysisService::analysisFailed,
        this,
        _captureResult);

    runToolForAsset(_assetCurrent, _configRun);

    disconnect(_okConnStat);
    disconnect(_failConnStat);
    disconnect(_okConnSpatial);
    disconnect(_failConnSpatial);

    if (!_bCaptured) {
        _strError = tr("分析执行后未返回结果");
        return false;
    }

    if (_resultCaptured.bSuccess) {
        _strResult = _resultCaptured.strDesc;
        return true;
    }

    _strError = _resultCaptured.strDesc;
    return false;
}

void MainWindow::initRibbonButtons()
{
    mpUI->btnOpenData->setIconName("open_data");
    mpUI->btnAddLayer->setIconName("add_layer");
    mpUI->btnSaveProject->setIconName("save_project");
    mpUI->btnExportResult->setIconName("export_result");
    mpUI->btnBufferAnalysis->setIconName("buffer");
    mpUI->btnOverlayAnalysis->setIconName("overlay");
    mpUI->btnSpatialQuery->setIconName("query");
    mpUI->btnRasterCalc->setIconName("raster_calc");
    mpUI->btnAIAnalyze->setIconName("ai_analyze");
    mpUI->btnAIChat->setIconName("ai_chat");

    mpUI->actionToggleAnalysisPanel->setText(tr("分析工作区"));
    mpUI->actionToggleAnalysisPanel->setCheckable(true);
    mpUI->actionBufferAnalysis->setText(tr("缓冲分析"));
    mpUI->actionOverlayAnalysis->setText(tr("叠加分析"));
    mpUI->actionSpatialQuery->setText(tr("空间查询"));
    mpUI->actionRasterCalc->setText(tr("栅格计算"));

    mpUI->btnBufferAnalysis->setToolTip(tr("打开统一分析工作区并聚焦缓冲分析入口。"));
    mpUI->btnOverlayAnalysis->setToolTip(tr("打开统一分析工作区并聚焦叠加分析入口。"));
    mpUI->btnSpatialQuery->setToolTip(tr("打开统一分析工作区并聚焦空间查询入口。"));
    mpUI->btnRasterCalc->setToolTip(tr("打开统一分析工作区并聚焦栅格计算入口。"));
}

void MainWindow::initDockWidgets()
{
    createLayerDock();
    createAIDock();
    createAttributeDock();
    createAnalysisWorkspaceDock();
    createLogDock();
}

void MainWindow::createLayerDock()
{
    mpctrlDockLayer = new QDockWidget(tr("图层"), this);
    mpctrlDockLayer->setObjectName("dockLayer");
    mpctrlDockLayer->setAllowedAreas(
        Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget* _pctrlContainer = new QWidget(mpctrlDockLayer);
    QVBoxLayout* _pLayerLayout = new QVBoxLayout(_pctrlContainer);
    _pLayerLayout->setContentsMargins(4, 4, 4, 4);
    _pLayerLayout->setSpacing(4);

    mpctrlLayerTreeView = new QgsLayerTreeView(_pctrlContainer);

    QgsLayerTreeModel* _pTreeModel = new QgsLayerTreeModel(
        QgsProject::instance()->layerTreeRoot(), mpctrlLayerTreeView);
    _pTreeModel->setFlag(QgsLayerTreeModel::AllowNodeReorder);
    _pTreeModel->setFlag(QgsLayerTreeModel::AllowNodeRename);
    _pTreeModel->setFlag(QgsLayerTreeModel::AllowNodeChangeVisibility);
    _pTreeModel->setFlag(QgsLayerTreeModel::ShowLegendAsTree);
    mpctrlLayerTreeView->setModel(_pTreeModel);

    MapCanvasWidget* _pActiveCanvas = mpMapCanvasManager->activeCanvas();
    if (_pActiveCanvas != nullptr) {
        QgsLayerTreeMapCanvasBridge* _pBridge = new QgsLayerTreeMapCanvasBridge(
            QgsProject::instance()->layerTreeRoot(),
            _pActiveCanvas->mapCanvas(),
            this);
        Q_UNUSED(_pBridge)
    }

    _pLayerLayout->addWidget(mpctrlLayerTreeView);
    mpctrlDockLayer->setWidget(_pctrlContainer);
    addDockWidget(Qt::LeftDockWidgetArea, mpctrlDockLayer);
    mpctrlDockLayer->setVisible(true);

    connect(mpUI->actionToggleLayerPanel, &QAction::toggled,
        mpctrlDockLayer, &QDockWidget::setVisible);
    connect(mpctrlDockLayer, &QDockWidget::visibilityChanged,
        mpUI->actionToggleLayerPanel, &QAction::setChecked);
}

void MainWindow::createAIDock()
{
    mpctrlDockAI = new AIDockWidget(this);
    mpctrlDockAI->setAIManager(mpAIManager);
    addDockWidget(Qt::RightDockWidgetArea, mpctrlDockAI);
    mpctrlDockAI->setVisible(true);

    connect(mpUI->actionToggleAIPanel, &QAction::toggled,
        mpctrlDockAI, &QDockWidget::setVisible);
    connect(mpctrlDockAI, &QDockWidget::visibilityChanged,
        mpUI->actionToggleAIPanel, &QAction::setChecked);
}

void MainWindow::createAttributeDock()
{
    mpctrlDockAttribute = new QDockWidget(tr("属性表"), this);
    mpctrlDockAttribute->setObjectName("dockAttribute");
    mpctrlDockAttribute->setAllowedAreas(Qt::AllDockWidgetAreas);

    mpctrlAttrTable = new QTableView(mpctrlDockAttribute);
    mpctrlDockAttribute->setWidget(mpctrlAttrTable);

    addDockWidget(Qt::RightDockWidgetArea, mpctrlDockAttribute);
    mpctrlDockAttribute->setVisible(false);

    connect(mpUI->actionToggleAttrPanel, &QAction::toggled,
        mpctrlDockAttribute, &QDockWidget::setVisible);
    connect(mpctrlDockAttribute, &QDockWidget::visibilityChanged,
        mpUI->actionToggleAttrPanel, &QAction::setChecked);
}

void MainWindow::createAnalysisWorkspaceDock()
{
    mpctrlDockAnalysisWorkspace = new AnalysisWorkspaceDockWidget(this);
    mpctrlDockAnalysisWorkspace->setDataRepository(mpDataRepository);
    mpVisualizationManager->attachWorkspace(mpctrlDockAnalysisWorkspace);

    addDockWidget(Qt::RightDockWidgetArea, mpctrlDockAnalysisWorkspace);
    mpctrlDockAnalysisWorkspace->setVisible(false);

    connect(mpUI->actionToggleAnalysisPanel, &QAction::toggled,
        mpctrlDockAnalysisWorkspace, &QDockWidget::setVisible);
    connect(mpctrlDockAnalysisWorkspace, &QDockWidget::visibilityChanged,
        mpUI->actionToggleAnalysisPanel, &QAction::setChecked);
}

void MainWindow::createLogDock()
{
    mpctrlDockLog = new QDockWidget(tr("日志"), this);
    mpctrlDockLog->setObjectName("dockLog");
    mpctrlDockLog->setAllowedAreas(
        Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    mpctrlLogView = new QTextEdit(mpctrlDockLog);
    mpctrlLogView->setReadOnly(true);
    mpctrlLogView->setMaximumHeight(180);

    mpctrlDockLog->setWidget(mpctrlLogView);
    addDockWidget(Qt::BottomDockWidgetArea, mpctrlDockLog);
    mpctrlDockLog->setVisible(false);

    connect(mpUI->actionToggleLogPanel, &QAction::toggled,
        mpctrlDockLog, &QDockWidget::setVisible);
    connect(mpctrlDockLog, &QDockWidget::visibilityChanged,
        mpUI->actionToggleLogPanel, &QAction::setChecked);
}

void MainWindow::initStatusBar()
{
    mpctrlLabelCoord = new QLabel(tr("  经度: --  纬度: --  "), this);
    mpctrlLabelCoord->setMinimumWidth(200);

    mpctrlLabelScale = new QLabel(tr("  比例尺: 1:--  "), this);
    mpctrlLabelScale->setMinimumWidth(120);

    mpctrlLabelStatus = new QLabel(tr("  就绪  "), this);

    statusBar()->addWidget(mpctrlLabelCoord);
    statusBar()->addWidget(mpctrlLabelScale);
    statusBar()->addPermanentWidget(mpctrlLabelStatus);
}

void MainWindow::initConnections()
{
    connect(mpUI->actionOpenData, &QAction::triggered,
        this, &MainWindow::onOpenData);
    connect(mpUI->actionAddLayer, &QAction::triggered,
        this, &MainWindow::onAddLayer);
    connect(mpUI->actionSaveProject, &QAction::triggered,
        this, &MainWindow::onSaveProject);
    connect(mpUI->actionExportResult, &QAction::triggered,
        this, &MainWindow::onExportResult);
    connect(mpUI->actionExit, &QAction::triggered,
        this, &QWidget::close);
    connect(mpUI->actionZoomIn, &QAction::triggered,
        this, &MainWindow::onNavZoomIn);
    connect(mpUI->actionZoomOut, &QAction::triggered,
        this, &MainWindow::onNavZoomOut);
    connect(mpUI->actionFitView, &QAction::triggered,
        this, &MainWindow::onNavFitAll);
    connect(mpUI->actionAIAnalyze, &QAction::triggered,
        this, &MainWindow::onAIAnalyze);
    connect(mpUI->actionAIChat, &QAction::triggered,
        this, &MainWindow::onAIChat);
    connect(mpUI->actionAbout, &QAction::triggered,
        this, &MainWindow::onAbout);
    connect(mpUI->actionBufferAnalysis, &QAction::triggered,
        this, &MainWindow::onBufferAnalysis);
    connect(mpUI->actionOverlayAnalysis, &QAction::triggered,
        this, &MainWindow::onOverlayAnalysis);
    connect(mpUI->actionSpatialQuery, &QAction::triggered,
        this, &MainWindow::onSpatialQuery);
    connect(mpUI->actionRasterCalc, &QAction::triggered,
        this, &MainWindow::onRasterCalc);

    connect(mpUI->btnOpenData, &ImageButton::clicked,
        this, &MainWindow::onOpenData);
    connect(mpUI->btnAddLayer, &ImageButton::clicked,
        this, &MainWindow::onAddLayer);
    connect(mpUI->btnSaveProject, &ImageButton::clicked,
        this, &MainWindow::onSaveProject);
    connect(mpUI->btnExportResult, &ImageButton::clicked,
        this, &MainWindow::onExportResult);
    connect(mpUI->btnAIAnalyze, &ImageButton::clicked,
        this, &MainWindow::onAIAnalyze);
    connect(mpUI->btnAIChat, &ImageButton::clicked,
        this, &MainWindow::onAIChat);
    connect(mpUI->btnBufferAnalysis, &ImageButton::clicked,
        this, &MainWindow::onBufferAnalysis);
    connect(mpUI->btnOverlayAnalysis, &ImageButton::clicked,
        this, &MainWindow::onOverlayAnalysis);
    connect(mpUI->btnSpatialQuery, &ImageButton::clicked,
        this, &MainWindow::onSpatialQuery);
    connect(mpUI->btnRasterCalc, &ImageButton::clicked,
        this, &MainWindow::onRasterCalc);

    connect(mpDataService, &DataService::layerLoaded,
        this, &MainWindow::onLayerLoaded);
    connect(mpDataService, &DataService::analysisAssetReady,
        this, &MainWindow::onAnalysisAssetReady);
    connect(mpDataService, &DataService::dataLoadFailed,
        this, &MainWindow::onDataLoadFailed);

    connect(mpStatisticalAnalysisService, &StatisticalAnalysisService::analysisFinished,
        this, &MainWindow::onAnalysisFinished);
    connect(mpStatisticalAnalysisService, &StatisticalAnalysisService::analysisFailed,
        this, &MainWindow::onAnalysisFailed);
    connect(mpStatisticalAnalysisService, &StatisticalAnalysisService::analysisProgress,
        this, &MainWindow::onAnalysisProgress);

    connect(mpSpatialAnalysisService, &SpatialAnalysisService::analysisFinished,
        this, &MainWindow::onAnalysisFinished);
    connect(mpSpatialAnalysisService, &SpatialAnalysisService::analysisFailed,
        this, &MainWindow::onAnalysisFailed);
    connect(mpSpatialAnalysisService, &SpatialAnalysisService::analysisProgress,
        this, &MainWindow::onAnalysisProgress);

    connect(mpDataRepository, &DataRepository::currentAssetChanged,
        this, &MainWindow::onCurrentAnalysisAssetChanged);
    connect(mpDataRepository, &DataRepository::currentAssetCleared,
        this, &MainWindow::onCurrentAnalysisAssetCleared);

    connect(mpctrlDockAnalysisWorkspace,
        &AnalysisWorkspaceDockWidget::basicStatisticsRequested,
        this, &MainWindow::onBasicStatisticsRequested);
    connect(mpctrlDockAnalysisWorkspace,
        &AnalysisWorkspaceDockWidget::frequencyStatisticsRequested,
        this, &MainWindow::onFrequencyStatisticsRequested);
    connect(mpctrlDockAnalysisWorkspace,
        &AnalysisWorkspaceDockWidget::neighborhoodAnalysisRequested,
        this, &MainWindow::onNeighborhoodAnalysisRequested);
    connect(mpctrlDockAnalysisWorkspace,
        &AnalysisWorkspaceDockWidget::attributeQueryRequested,
        this, &MainWindow::onAttributeQueryRequested);

    connect(mpMapCanvasManager, &MapCanvasManager::activeCanvasChanged,
        this, &MainWindow::onActiveCanvasChanged);

    reconnectCanvasSignals(mpMapCanvasManager->activeCanvas());
    initLayerTreeContextMenu();
}

void MainWindow::initLayerTreeContextMenu()
{
    if (mpctrlLayerTreeView == nullptr) {
        return;
    }

    mpctrlLayerTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mpctrlLayerTreeView, &QWidget::customContextMenuRequested,
        this, &MainWindow::onLayerTreeContextMenuRequested);
}

void MainWindow::reconnectCanvasSignals(MapCanvasWidget* _pCanvas)
{
    for (int _nCanvasIdx = 0; _nCanvasIdx < mpMapCanvasManager->canvasCount(); ++_nCanvasIdx) {
        MapCanvasWidget* _pOldCanvas = mpMapCanvasManager->canvasAt(_nCanvasIdx);
        if (_pOldCanvas != nullptr) {
            disconnect(_pOldCanvas, &MapCanvasWidget::coordChanged,
                this, &MainWindow::onCoordChanged);
            disconnect(_pOldCanvas, &MapCanvasWidget::scaleChanged,
                this, &MainWindow::onScaleChanged);
        }
    }

    if (_pCanvas == nullptr) {
        return;
    }

    connect(_pCanvas, &MapCanvasWidget::coordChanged,
        this, &MainWindow::onCoordChanged);
    connect(_pCanvas, &MapCanvasWidget::scaleChanged,
        this, &MainWindow::onScaleChanged);
}

void MainWindow::ensureWorkspaceVisible()
{
    if (mpctrlDockAnalysisWorkspace == nullptr) {
        return;
    }

    mpctrlDockAnalysisWorkspace->setVisible(true);
    mpctrlDockAnalysisWorkspace->raise();
    {
        QSignalBlocker _blocker(mpUI->actionToggleAnalysisPanel);
        mpUI->actionToggleAnalysisPanel->setChecked(true);
    }
}

QgsMapLayer* MainWindow::currentSelectedLayer() const
{
    if (mpctrlLayerTreeView == nullptr) {
        return nullptr;
    }

    return mpctrlLayerTreeView->currentLayer();
}

DataAssetType MainWindow::resolveAssetChoice(const AnalysisDataAsset& _assetInput)
{
    if (!_assetInput.bNeedsUserChoice) {
        return _assetInput.eAssetType;
    }

    QMessageBox _msgBox(this);
    _msgBox.setWindowTitle(tr("选择导入方式"));
    _msgBox.setIcon(QMessageBox::Question);
    _msgBox.setText(_assetInput.strChoicePrompt.isEmpty()
        ? tr("当前输入存在多种资产解释方式，请选择目标资产类型。")
        : _assetInput.strChoicePrompt);

    QAbstractButton* _pTableButton = _msgBox.addButton(
        dataAssetTypeDisplayName(DataAssetType::Table),
        QMessageBox::AcceptRole);
    QAbstractButton* _pVectorButton = _msgBox.addButton(
        dataAssetTypeDisplayName(DataAssetType::Vector),
        QMessageBox::ActionRole);
    _msgBox.addButton(QMessageBox::Cancel);

    _msgBox.exec();
    if (_msgBox.clickedButton() == _pTableButton) {
        return DataAssetType::Table;
    }
    if (_msgBox.clickedButton() == _pVectorButton) {
        return DataAssetType::Vector;
    }
    return DataAssetType::None;
}

bool MainWindow::assetSupportsTool(const AnalysisDataAsset& _assetInput,
    const QString& _strToolId) const
{
    if (_strToolId == "basic_statistics"
        || _strToolId == "frequency_statistics") {
        return _assetInput.flagsCapabilities.testFlag(AnalysisCapability::Statistical);
    }
    if (_strToolId == "neighborhood_analysis"
        || _strToolId == "raster_calc") {
        return _assetInput.flagsCapabilities.testFlag(AnalysisCapability::SpatialRaster);
    }
    if (_strToolId == "buffer_analysis"
        || _strToolId == "overlay_analysis"
        || _strToolId == "spatial_query") {
        return _assetInput.flagsCapabilities.testFlag(AnalysisCapability::SpatialVector);
    }
    if (_strToolId == "attribute_query") {
        return _assetInput.flagsCapabilities.testFlag(AnalysisCapability::AttributeQuery);
    }
    return false;
}

void MainWindow::openToolShortcut(const QString& _strToolId)
{
    ensureWorkspaceVisible();

    if (mpDataRepository == nullptr || !mpDataRepository->hasCurrentAsset()) {
        mpctrlDockAnalysisWorkspace->showDataPage(
            tr("请先导入并选择支持“%1”的数据资产。").arg(toolDisplayName(_strToolId)));
        return;
    }

    const AnalysisDataAsset _assetCurrent = mpDataRepository->currentAsset();
    if (!assetSupportsTool(_assetCurrent, _strToolId)) {
        mpctrlDockAnalysisWorkspace->showDataPage(
            tr("当前所选资产“%1”不支持“%2”，请先选择兼容数据。")
                .arg(_assetCurrent.strName, toolDisplayName(_strToolId)));
        return;
    }

    mpctrlDockAnalysisWorkspace->focusTool(
        _strToolId,
        tr("当前快捷入口：%1\n资产类型：%2\n资产能力：%3")
            .arg(toolDisplayName(_strToolId),
                dataAssetTypeDisplayName(_assetCurrent.eAssetType),
                capabilityText(_assetCurrent.flagsCapabilities)));
}

void MainWindow::runToolForCurrentAsset(const AnalysisRunConfig& _configRun)
{
    if (mpDataRepository == nullptr || !mpDataRepository->hasCurrentAsset()) {
        ensureWorkspaceVisible();
        mpctrlDockAnalysisWorkspace->showDataPage(
            tr("请先导入并选择一个可执行“%1”的数据资产。")
                .arg(toolDisplayName(_configRun.strToolId)));
        return;
    }

    const AnalysisDataAsset _assetCurrent = mpDataRepository->currentAsset();
    if (!assetSupportsTool(_assetCurrent, _configRun.strToolId)) {
        ensureWorkspaceVisible();
        mpctrlDockAnalysisWorkspace->showDataPage(
            tr("当前所选资产“%1”不支持“%2”，请先选择兼容数据。")
                .arg(_assetCurrent.strName, toolDisplayName(_configRun.strToolId)));
        return;
    }

    runToolForAsset(_assetCurrent, _configRun);
}

void MainWindow::runToolForAsset(const AnalysisDataAsset& _assetInput,
    const AnalysisRunConfig& _configRun)
{
    cachePendingRun(_assetInput.strAssetId, _configRun);
    ensureWorkspaceVisible();
    mpctrlDockAnalysisWorkspace->showResultsPage();
    mpctrlLabelStatus->setText(tr("  分析中...  "));

    if (_configRun.strToolId == "basic_statistics") {
        mpStatisticalAnalysisService->runBasicStatistics(_assetInput);
        return;
    }
    if (_configRun.strToolId == "frequency_statistics") {
        mpStatisticalAnalysisService->runFrequencyStatistics(
            _assetInput, _configRun.nFrequencyBins);
        return;
    }
    if (_configRun.strToolId == "neighborhood_analysis") {
        mpSpatialAnalysisService->runNeighborhoodAnalysis(
            _assetInput, _configRun.nNeighborhoodWindow);
        return;
    }
    if (_configRun.strToolId == "attribute_query") {
        onAnalysisFailed(mpAttributeQueryService->buildPlaceholderResult(_assetInput));
        return;
    }

    clearPendingRun();
    mpctrlDockAnalysisWorkspace->focusTool(
        _configRun.strToolId,
        tr("工具“%1”当前仅作为能力占位展示。").arg(toolDisplayName(_configRun.strToolId)));
    mpctrlLabelStatus->setText(tr("  就绪  "));
}

void MainWindow::cachePendingRun(const QString& _strAssetId,
    const AnalysisRunConfig& _configRun)
{
    mbHasPendingRun = true;
    mstrPendingRunAssetId = _strAssetId;
    mConfigPendingRun = _configRun;
}

void MainWindow::clearPendingRun()
{
    mbHasPendingRun = false;
    mstrPendingRunAssetId.clear();
    mConfigPendingRun = AnalysisRunConfig();
}

void MainWindow::onOpenData()
{
    const QString _strFilePath = QFileDialog::getOpenFileName(
        this,
        tr("打开分析数据"),
        QString(),
        tr("分析数据 (*.csv *.xml *.asc *.txt *.shp *.geojson *.tif *.tiff *.img);;"
           "CSV 数据 (*.csv);;XML 数据 (*.xml);;栅格数据 (*.asc *.txt *.tif *.tiff *.img);;"
           "矢量数据 (*.shp *.geojson);;全部文件 (*)"));
    if (_strFilePath.isEmpty()) {
        return;
    }

    mpctrlLabelStatus->setText(tr("  正在解析分析数据...  "));
    mpDataService->openDataForAnalysis(_strFilePath);
}

void MainWindow::onAddLayer()
{
    const QString _strFilePath = QFileDialog::getOpenFileName(
        this,
        tr("添加图层"),
        QString(),
        tr("空间图层 (*.shp *.geojson *.tif *.tiff *.img);;"
           "矢量图层 (*.shp *.geojson);;栅格图层 (*.tif *.tiff *.img);;全部文件 (*)"));
    if (_strFilePath.isEmpty()) {
        return;
    }

    mpctrlLabelStatus->setText(tr("  正在添加图层...  "));
    mpDataService->loadLayerToMap(_strFilePath);
}

void MainWindow::onSaveProject()
{
}

void MainWindow::onExportResult()
{
}

void MainWindow::onNavPan()
{
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pCanvas != nullptr) {
        _pCanvas->setMapTool(MapToolType::Pan);
    }
    mpctrlLabelStatus->setText(tr("  工具: 平移  "));
}

void MainWindow::onNavZoomIn()
{
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pCanvas != nullptr) {
        _pCanvas->setMapTool(MapToolType::ZoomIn);
    }
    mpctrlLabelStatus->setText(tr("  工具: 放大  "));
}

void MainWindow::onNavZoomOut()
{
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pCanvas != nullptr) {
        _pCanvas->setMapTool(MapToolType::ZoomOut);
    }
    mpctrlLabelStatus->setText(tr("  工具: 缩小  "));
}

void MainWindow::onNavFitAll()
{
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pCanvas != nullptr) {
        _pCanvas->zoomToFullExtent();
    }
    mpctrlLabelStatus->setText(tr("  工具: 全图  "));
}

void MainWindow::onBufferAnalysis()
{
    openToolShortcut("buffer_analysis");
}

void MainWindow::onOverlayAnalysis()
{
    openToolShortcut("overlay_analysis");
}

void MainWindow::onSpatialQuery()
{
    openToolShortcut("spatial_query");
}

void MainWindow::onRasterCalc()
{
    openToolShortcut("raster_calc");
}

void MainWindow::onAIAnalyze()
{
    mpctrlDockAI->focusInput();
}

void MainWindow::onAIChat()
{
    mpctrlDockAI->focusInput();
}

void MainWindow::onAbout()
{
    QMessageBox::about(this,
        tr("关于 GeoAI 智能空间分析平台"),
        tr("GeoAI 智能空间分析平台\n版本：1.0.0\n"));
}

void MainWindow::onLayerLoaded(const LayerInfo& _layerInfo)
{
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pCanvas != nullptr) {
        _pCanvas->loadFromPath(_layerInfo.strFilePath);
    }

    mpctrlLabelStatus->setText(tr("  就绪  "));
    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(
            tr("[图层] 已添加到地图: %1 (%2)")
                .arg(_layerInfo.strFilePath, _layerInfo.strType));
    }
}

void MainWindow::onAnalysisAssetReady(const AnalysisDataAsset& _assetReady)
{
    AnalysisDataAsset _assetResolved = _assetReady;
    if (_assetResolved.bNeedsUserChoice) {
        const DataAssetType _eChosenType = resolveAssetChoice(_assetResolved);
        if (_eChosenType == DataAssetType::None) {
            mpctrlLabelStatus->setText(tr("  已取消导入  "));
            return;
        }

        if (_eChosenType != _assetResolved.eAssetType) {
            AnalysisDataAsset _assetAlternate;
            QString _strError;
            if (!mpDataService->buildAlternateAsset(
                _assetResolved, _eChosenType, _assetAlternate, _strError)) {
                onDataLoadFailed(_strError);
                return;
            }
            _assetResolved = _assetAlternate;
        }

        _assetResolved.bNeedsUserChoice = false;
        _assetResolved.strChoicePrompt.clear();
    }

    const AnalysisDataAsset _assetStored = mpDataRepository->upsertAsset(_assetResolved);
    ensureWorkspaceVisible();
    mpctrlDockAnalysisWorkspace->showDataPage(
        tr("已导入数据资产：%1\n类型：%2\n能力：%3")
            .arg(_assetStored.strName,
                dataAssetTypeDisplayName(_assetStored.eAssetType),
                capabilityText(_assetStored.flagsCapabilities)));

    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(
            tr("[数据资产] %1 -> %2 | 能力：%3")
                .arg(_assetStored.strSourcePath,
                    dataAssetTypeDisplayName(_assetStored.eAssetType),
                    capabilityText(_assetStored.flagsCapabilities)));
    }

    if (mmapLastSuccessfulRuns.contains(_assetStored.strAssetId)
        && assetSupportsTool(_assetStored,
            mmapLastSuccessfulRuns.value(_assetStored.strAssetId).strToolId)) {
        mpctrlLabelStatus->setText(tr("  正在按该资产最近一次成功配置自动刷新...  "));
        runToolForAsset(_assetStored,
            mmapLastSuccessfulRuns.value(_assetStored.strAssetId));
        return;
    }

    const QString _strMessage = tr(
        "已选择数据资产“%1”。\n"
        "请切换到 Tools 页运行兼容分析工具。")
        .arg(_assetStored.strName);
    mpctrlDockAnalysisWorkspace->clearCurrentResult(_strMessage);
    mpVisualizationManager->clearView(_strMessage);
    mstrDisplayedResultAssetId.clear();
    mpctrlLabelStatus->setText(tr("  就绪  "));
}

void MainWindow::onDataLoadFailed(const QString& _strErrorMsg)
{
    mpctrlLabelStatus->setText(tr("  数据加载失败  "));
    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(tr("[错误] %1").arg(_strErrorMsg));
    }
    QMessageBox::warning(this, tr("数据加载失败"), _strErrorMsg);
}

void MainWindow::onAnalysisFinished(const AnalysisResult& _result)
{
    mstrDisplayedResultAssetId = _result.strSourceAssetId;
    mpctrlLabelStatus->setText(tr("  分析完成  "));
    mpctrlDockAnalysisWorkspace->setCurrentResult(_result);
    mpctrlDockAnalysisWorkspace->addResultHistory(_result);

    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(tr("[分析] %1").arg(_result.strDesc));
    }

    mpVisualizationManager->updateFromResult(_result);

    if (mbHasPendingRun
        && mstrPendingRunAssetId == _result.strSourceAssetId) {
        mmapLastSuccessfulRuns[mstrPendingRunAssetId] = mConfigPendingRun;
    }
    clearPendingRun();
}

void MainWindow::onAnalysisFailed(const AnalysisResult& _result)
{
    mstrDisplayedResultAssetId = _result.strSourceAssetId;
    mpctrlLabelStatus->setText(tr("  分析失败  "));
    mpctrlDockAnalysisWorkspace->setCurrentResult(_result);
    mpctrlDockAnalysisWorkspace->addResultHistory(_result);

    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(tr("[分析失败] %1").arg(_result.strDesc));
    }

    mpVisualizationManager->clearView(_result.strDesc);
    clearPendingRun();
}

void MainWindow::onAnalysisProgress(int _nPercent)
{
    mpctrlLabelStatus->setText(tr("  分析中 %1%...  ").arg(_nPercent));
}

void MainWindow::onCoordChanged(double _dLon, double _dLat)
{
    mpctrlLabelCoord->setText(
        tr("  经度: %1  纬度: %2  ")
            .arg(_dLon, 0, 'f', 6)
            .arg(_dLat, 0, 'f', 6));
}

void MainWindow::onScaleChanged(double _dScale)
{
    mpctrlLabelScale->setText(
        tr("  比例尺: 1:%1  ").arg(static_cast<long long>(_dScale)));
}

void MainWindow::onActiveCanvasChanged(MapCanvasWidget* _pCanvas)
{
    reconnectCanvasSignals(_pCanvas);
    mpctrlLabelCoord->setText(tr("  经度: --  纬度: --  "));
    mpctrlLabelScale->setText(tr("  比例尺: 1:--  "));
}

void MainWindow::onCurrentAnalysisAssetChanged(const AnalysisDataAsset& _assetCurrent)
{
    if (_assetCurrent.strAssetId == mstrDisplayedResultAssetId) {
        return;
    }

    const QString _strMessage = tr(
        "当前已切换到数据资产“%1”。\n"
        "结果区不会沿用其他资产的分析结果，请重新运行分析。")
        .arg(_assetCurrent.strName);
    mpctrlDockAnalysisWorkspace->clearCurrentResult(_strMessage);
    mpVisualizationManager->clearView(_strMessage);
    mstrDisplayedResultAssetId.clear();
}

void MainWindow::onCurrentAnalysisAssetCleared()
{
    const QString _strMessage = tr("当前没有已选择的分析资产。");
    mpctrlDockAnalysisWorkspace->clearCurrentResult(_strMessage);
    mpVisualizationManager->clearView(_strMessage);
    mstrDisplayedResultAssetId.clear();
}

void MainWindow::onLayerTreeContextMenuRequested(const QPoint& _posMenu)
{
    if (mpctrlLayerTreeView == nullptr) {
        return;
    }

    const QModelIndex _indexCurrent = mpctrlLayerTreeView->indexAt(_posMenu);
    if (!_indexCurrent.isValid()) {
        return;
    }

    mpctrlLayerTreeView->setCurrentIndex(_indexCurrent);
    QgsMapLayer* _pLayerCurrent = currentSelectedLayer();
    if (_pLayerCurrent == nullptr) {
        return;
    }

    QMenu _menuLayer(mpctrlLayerTreeView);
    QAction* _pActionZoom = _menuLayer.addAction(tr("缩放到图层范围"));
    QAction* _pActionRemove = _menuLayer.addAction(tr("移除图层"));

    QAction* _pActionTriggered = _menuLayer.exec(
        mpctrlLayerTreeView->viewport()->mapToGlobal(_posMenu));
    if (_pActionTriggered == _pActionZoom) {
        onZoomToSelectedLayer();
        return;
    }
    if (_pActionTriggered == _pActionRemove) {
        onRemoveSelectedLayer();
    }
}

void MainWindow::onRemoveSelectedLayer()
{
    QgsMapLayer* _pLayerCurrent = currentSelectedLayer();
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pLayerCurrent == nullptr || _pCanvas == nullptr) {
        return;
    }

    const QString _strLayerId = _pLayerCurrent->id();
    const QString _strLayerName = _pLayerCurrent->name();
    const QString _strLayerSource = _pLayerCurrent->source();

    _pCanvas->removeLayer(_strLayerId);
    if (mpDataService != nullptr) {
        mpDataService->removeLayerRecord(_strLayerSource, _strLayerName);
    }

    mpctrlLabelStatus->setText(tr("  已移除图层: %1  ").arg(_strLayerName));
    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(tr("[图层] 已移除: %1").arg(_strLayerName));
    }
}

void MainWindow::onZoomToSelectedLayer()
{
    QgsMapLayer* _pLayerCurrent = currentSelectedLayer();
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pLayerCurrent == nullptr || _pCanvas == nullptr) {
        return;
    }

    _pCanvas->zoomToLayerExtent(_pLayerCurrent->id());
    mpctrlLabelStatus->setText(
        tr("  已缩放到图层范围: %1  ").arg(_pLayerCurrent->name()));
}

void MainWindow::onBasicStatisticsRequested()
{
    AnalysisRunConfig _configRun;
    _configRun.strToolId = "basic_statistics";
    runToolForCurrentAsset(_configRun);
}

void MainWindow::onFrequencyStatisticsRequested(int _nFrequencyBins)
{
    AnalysisRunConfig _configRun;
    _configRun.strToolId = "frequency_statistics";
    _configRun.nFrequencyBins = _nFrequencyBins;
    runToolForCurrentAsset(_configRun);
}

void MainWindow::onNeighborhoodAnalysisRequested(int _nNeighborhoodWindow)
{
    AnalysisRunConfig _configRun;
    _configRun.strToolId = "neighborhood_analysis";
    _configRun.nNeighborhoodWindow = _nNeighborhoodWindow;
    runToolForCurrentAsset(_configRun);
}

void MainWindow::onAttributeQueryRequested()
{
    AnalysisRunConfig _configRun;
    _configRun.strToolId = "attribute_query";
    runToolForCurrentAsset(_configRun);
}
