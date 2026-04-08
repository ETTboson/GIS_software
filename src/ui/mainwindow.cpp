#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui/style/imagebutton.h"
#include "ui/docks/aidockwidget.h"
#include "ui/widgets/mapcanvaswidget.h"
#include "ui/widgets/mapcanvasmanager.h"
#include "service/dataservice.h"
#include "service/analysisservice.h"
#include "core/aimanager.h"

#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QTableView>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <qgsmapcanvas.h>
#include <qgslayertreemodel.h>
#include <qgslayertreemapcanvasbridge.h>
#include <qgsproject.h>

MainWindow::MainWindow(QWidget* _pParent)
    : QMainWindow(_pParent)
    , mpUI(new Ui::MainWindow)
    , mpDataService(nullptr)
    , mpAnalysisService(nullptr)
    , mpAIManager(nullptr)
    , mpMapCanvasManager(nullptr)
    , mpctrlDockLayer(nullptr)
    , mpctrlDockAI(nullptr)
    , mpctrlDockAttribute(nullptr)
    , mpctrlDockAnalysis(nullptr)
    , mpctrlDockLog(nullptr)
    , mpctrlLayerTreeView(nullptr)
    , mpctrlAttrTable(nullptr)
    , mpctrlLogView(nullptr)
    , mpctrlLabelCoord(nullptr)
    , mpctrlLabelScale(nullptr)
    , mpctrlLabelStatus(nullptr)
    , mpctrlLabelAnalysisData(nullptr)
    , mpctrlComboAnalysisMethod(nullptr)
    , mpctrlSpinFrequencyBins(nullptr)
    , mpctrlSpinNeighborhoodWindow(nullptr)
    , mpctrlBtnRunAnalysis(nullptr)
    , mpctrlAnalysisResultView(nullptr)
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

    delete mpAnalysisService;
    mpAnalysisService = nullptr;

    delete mpAIManager;
    mpAIManager = nullptr;

    delete mpMapCanvasManager;
    mpMapCanvasManager = nullptr;

    delete mpUI;
    mpUI = nullptr;
}

void MainWindow::initModules()
{
    mpDataService = new DataService(this);
    mpAnalysisService = new AnalysisService(this);
    mpAIManager = new AIManager(this);
    mpMapCanvasManager = new MapCanvasManager(this);

    mpAIManager->setAnalysisService(mpAnalysisService);

    MapCanvasWidget* _pCanvas = mpMapCanvasManager->createCanvas(mpUI->wgtMapCanvas);

    QVBoxLayout* _pLayout = new QVBoxLayout(mpUI->wgtMapCanvas);
    _pLayout->setContentsMargins(0, 0, 0, 0);
    _pLayout->setSpacing(0);
    _pLayout->addWidget(_pCanvas);
    mpUI->wgtMapCanvas->setLayout(_pLayout);
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
}

void MainWindow::initDockWidgets()
{
    createLayerDock();
    createAIDock();
    createAttributeDock();
    createAnalysisDock();
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

void MainWindow::createAnalysisDock()
{
    mpctrlDockAnalysis = new QDockWidget(tr("数据分析"), this);
    mpctrlDockAnalysis->setObjectName("dockAnalysis");
    mpctrlDockAnalysis->setAllowedAreas(Qt::AllDockWidgetAreas);

    QWidget* _pctrlContainer = new QWidget(mpctrlDockAnalysis);
    QVBoxLayout* _pLayout = new QVBoxLayout(_pctrlContainer);
    _pLayout->setContentsMargins(8, 8, 8, 8);
    _pLayout->setSpacing(8);

    mpctrlLabelAnalysisData = new QLabel(
        tr("当前数据：未加载 CSV 或简单栅格数据"), _pctrlContainer);
    mpctrlLabelAnalysisData->setWordWrap(true);

    QLabel* _pctrlMethodLabel = new QLabel(tr("分析方法"), _pctrlContainer);
    mpctrlComboAnalysisMethod = new QComboBox(_pctrlContainer);
    mpctrlComboAnalysisMethod->addItem(tr("基础统计"));
    mpctrlComboAnalysisMethod->addItem(tr("频率统计"));
    mpctrlComboAnalysisMethod->addItem(tr("邻域分析"));

    QLabel* _pctrlFreqLabel = new QLabel(tr("频率统计分箱数"), _pctrlContainer);
    mpctrlSpinFrequencyBins = new QSpinBox(_pctrlContainer);
    mpctrlSpinFrequencyBins->setRange(2, 50);
    mpctrlSpinFrequencyBins->setValue(10);
    mpctrlSpinFrequencyBins->setPrefix(tr("bins="));

    QLabel* _pctrlWindowLabel = new QLabel(tr("邻域窗口大小"), _pctrlContainer);
    mpctrlSpinNeighborhoodWindow = new QSpinBox(_pctrlContainer);
    mpctrlSpinNeighborhoodWindow->setRange(3, 15);
    mpctrlSpinNeighborhoodWindow->setSingleStep(2);
    mpctrlSpinNeighborhoodWindow->setValue(3);
    mpctrlSpinNeighborhoodWindow->setPrefix(tr("win="));

    mpctrlBtnRunAnalysis = new QPushButton(tr("执行分析"), _pctrlContainer);
    mpctrlAnalysisResultView = new QTextEdit(_pctrlContainer);
    mpctrlAnalysisResultView->setReadOnly(true);
    mpctrlAnalysisResultView->setPlaceholderText(
        tr("分析结果将在这里显示"));

    _pLayout->addWidget(mpctrlLabelAnalysisData);
    _pLayout->addWidget(_pctrlMethodLabel);
    _pLayout->addWidget(mpctrlComboAnalysisMethod);
    _pLayout->addWidget(_pctrlFreqLabel);
    _pLayout->addWidget(mpctrlSpinFrequencyBins);
    _pLayout->addWidget(_pctrlWindowLabel);
    _pLayout->addWidget(mpctrlSpinNeighborhoodWindow);
    _pLayout->addWidget(mpctrlBtnRunAnalysis);
    _pLayout->addWidget(mpctrlAnalysisResultView, 1);

    mpctrlDockAnalysis->setWidget(_pctrlContainer);
    addDockWidget(Qt::RightDockWidgetArea, mpctrlDockAnalysis);
    mpctrlDockAnalysis->setVisible(false);

    connect(mpUI->actionToggleAnalysisPanel, &QAction::toggled,
        mpctrlDockAnalysis, &QDockWidget::setVisible);
    connect(mpctrlDockAnalysis, &QDockWidget::visibilityChanged,
        mpUI->actionToggleAnalysisPanel, &QAction::setChecked);

    updateAnalysisParameterWidgets();
}

void MainWindow::createLogDock()
{
    mpctrlDockLog = new QDockWidget(tr("日志"), this);
    mpctrlDockLog->setObjectName("dockLog");
    mpctrlDockLog->setAllowedAreas(
        Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    mpctrlLogView = new QTextEdit(mpctrlDockLog);
    mpctrlLogView->setReadOnly(true);
    mpctrlLogView->setMaximumHeight(160);

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
    connect(mpUI->actionOpenData, &QAction::triggered, this, &MainWindow::onOpenData);
    connect(mpUI->actionAddLayer, &QAction::triggered, this, &MainWindow::onAddLayer);
    connect(mpUI->actionSaveProject, &QAction::triggered, this, &MainWindow::onSaveProject);
    connect(mpUI->actionExportResult, &QAction::triggered, this, &MainWindow::onExportResult);
    connect(mpUI->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(mpUI->actionZoomIn, &QAction::triggered, this, &MainWindow::onNavZoomIn);
    connect(mpUI->actionZoomOut, &QAction::triggered, this, &MainWindow::onNavZoomOut);
    connect(mpUI->actionFitView, &QAction::triggered, this, &MainWindow::onNavFitAll);
    connect(mpUI->actionBufferAnalysis, &QAction::triggered, this, &MainWindow::onBufferAnalysis);
    connect(mpUI->actionOverlayAnalysis, &QAction::triggered, this, &MainWindow::onOverlayAnalysis);
    connect(mpUI->actionSpatialQuery, &QAction::triggered, this, &MainWindow::onSpatialQuery);
    connect(mpUI->actionRasterCalc, &QAction::triggered, this, &MainWindow::onRasterCalc);
    connect(mpUI->actionAIAnalyze, &QAction::triggered, this, &MainWindow::onAIAnalyze);
    connect(mpUI->actionAIChat, &QAction::triggered, this, &MainWindow::onAIChat);
    connect(mpUI->actionAbout, &QAction::triggered, this, &MainWindow::onAbout);

    connect(mpUI->btnOpenData, &ImageButton::clicked, this, &MainWindow::onOpenData);
    connect(mpUI->btnAddLayer, &ImageButton::clicked, this, &MainWindow::onAddLayer);
    connect(mpUI->btnSaveProject, &ImageButton::clicked, this, &MainWindow::onSaveProject);
    connect(mpUI->btnExportResult, &ImageButton::clicked, this, &MainWindow::onExportResult);
    connect(mpUI->btnBufferAnalysis, &ImageButton::clicked, this, &MainWindow::onBufferAnalysis);
    connect(mpUI->btnOverlayAnalysis, &ImageButton::clicked, this, &MainWindow::onOverlayAnalysis);
    connect(mpUI->btnSpatialQuery, &ImageButton::clicked, this, &MainWindow::onSpatialQuery);
    connect(mpUI->btnRasterCalc, &ImageButton::clicked, this, &MainWindow::onRasterCalc);
    connect(mpUI->btnAIAnalyze, &ImageButton::clicked, this, &MainWindow::onAIAnalyze);
    connect(mpUI->btnAIChat, &ImageButton::clicked, this, &MainWindow::onAIChat);

    connect(mpDataService, &DataService::dataLoaded,
        this, &MainWindow::onDataLoaded);
    connect(mpDataService, &DataService::dataLoadFailed,
        this, &MainWindow::onDataLoadFailed);

    connect(mpAnalysisService, &AnalysisService::analysisFinished,
        this, &MainWindow::onAnalysisFinished);
    connect(mpAnalysisService, &AnalysisService::analysisFailed,
        this, &MainWindow::onAnalysisFailed);
    connect(mpAnalysisService, &AnalysisService::analysisProgress,
        this, &MainWindow::onAnalysisProgress);

    connect(mpDataService, &DataService::dataLoaded,
        mpAnalysisService, &AnalysisService::onDataReady);
    connect(mpDataService, &DataService::numericDataLoaded,
        mpAnalysisService, &AnalysisService::onNumericDataReady);

    connect(mpMapCanvasManager, &MapCanvasManager::activeCanvasChanged,
        this, &MainWindow::onActiveCanvasChanged);

    connect(mpctrlComboAnalysisMethod,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::onAnalysisMethodChanged);
    connect(mpctrlBtnRunAnalysis, &QPushButton::clicked,
        this, &MainWindow::onRunAnalysisClicked);

    reconnectCanvasSignals(mpMapCanvasManager->activeCanvas());
}

void MainWindow::reconnectCanvasSignals(MapCanvasWidget* _pCanvas)
{
    for (int _nFilesIdx = 0; _nFilesIdx < mpMapCanvasManager->canvasCount(); ++_nFilesIdx) {
        MapCanvasWidget* _pOld = mpMapCanvasManager->canvasAt(_nFilesIdx);
        if (_pOld != nullptr) {
            disconnect(_pOld, &MapCanvasWidget::coordChanged,
                this, &MainWindow::onCoordChanged);
            disconnect(_pOld, &MapCanvasWidget::scaleChanged,
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

void MainWindow::onOpenData()
{
    const QString _strFilePath = QFileDialog::getOpenFileName(
        this, tr("打开数据"), QString(),
        tr("支持数据 (*.csv *.asc *.txt *.shp *.tif *.tiff *.geojson);;"
           "CSV 数据 (*.csv);;简单栅格 (*.asc *.txt);;GIS 数据 (*.shp *.tif *.tiff *.geojson);;全部文件 (*)"));
    if (_strFilePath.isEmpty()) {
        return;
    }

    mpctrlLabelStatus->setText(tr("  正在加载...  "));
    mpDataService->loadData(_strFilePath);
}

void MainWindow::onAddLayer()
{
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
    mpctrlDockAnalysis->setVisible(true);
    mpctrlComboAnalysisMethod->setCurrentIndex(0);
    runSelectedAnalysis();
}

void MainWindow::onOverlayAnalysis()
{
    mpctrlDockAnalysis->setVisible(true);
    mpctrlComboAnalysisMethod->setCurrentIndex(1);
    runSelectedAnalysis();
}

void MainWindow::onSpatialQuery()
{
    mpctrlDockAnalysis->setVisible(true);
    mpctrlComboAnalysisMethod->setCurrentIndex(2);
    runSelectedAnalysis();
}

void MainWindow::onRasterCalc()
{
    mpctrlDockAnalysis->setVisible(true);
    mpctrlComboAnalysisMethod->setCurrentIndex(2);
    runSelectedAnalysis();
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

void MainWindow::onDataLoaded(const LayerInfo& _layerInfo)
{
    if (_layerInfo.strType == "vector" || _layerInfo.strType == "raster") {
        MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
        if (_pCanvas != nullptr) {
            _pCanvas->loadFromPath(_layerInfo.strFilePath);
        }
    }

    if (_layerInfo.strType == "table" || _layerInfo.strType == "simple_raster") {
        mpctrlLabelAnalysisData->setText(
            tr("当前数据：%1 (%2)").arg(_layerInfo.strName, _layerInfo.strType));
        mpctrlAnalysisResultView->setPlainText(
            tr("数据已加载，可以选择分析方法后执行。"));
        mpctrlDockAnalysis->setVisible(true);
    }

    mpctrlLabelStatus->setText(tr("  就绪  "));

    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(
            tr("[数据] 已加载: %1 (%2)").arg(_layerInfo.strFilePath, _layerInfo.strType));
    }
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
    mpctrlLabelStatus->setText(tr("  分析完成  "));
    if (mpctrlAnalysisResultView != nullptr) {
        mpctrlAnalysisResultView->setPlainText(_result.strDesc);
    }
    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(tr("[分析] %1").arg(_result.strDesc));
    }
}

void MainWindow::onAnalysisFailed(const AnalysisResult& _result)
{
    mpctrlLabelStatus->setText(tr("  分析失败  "));
    if (mpctrlAnalysisResultView != nullptr) {
        mpctrlAnalysisResultView->setPlainText(_result.strDesc);
    }
    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(tr("[分析失败] %1").arg(_result.strDesc));
    }
    QMessageBox::warning(this, tr("分析失败"), _result.strDesc);
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

void MainWindow::onAnalysisMethodChanged(int _nIndex)
{
    Q_UNUSED(_nIndex)
    updateAnalysisParameterWidgets();
}

void MainWindow::onRunAnalysisClicked()
{
    runSelectedAnalysis();
}

void MainWindow::runSelectedAnalysis()
{
    if (mpctrlComboAnalysisMethod == nullptr) {
        return;
    }

    mpctrlDockAnalysis->setVisible(true);

    switch (mpctrlComboAnalysisMethod->currentIndex()) {
    case 0:
        mpAnalysisService->runBasicStatistics();
        break;
    case 1:
        mpAnalysisService->runFrequencyStatistics(mpctrlSpinFrequencyBins->value());
        break;
    case 2:
        mpAnalysisService->runNeighborhoodAnalysis(mpctrlSpinNeighborhoodWindow->value());
        break;
    default:
        break;
    }
}

void MainWindow::updateAnalysisParameterWidgets()
{
    const int _nMethodIdx = (mpctrlComboAnalysisMethod == nullptr)
        ? 0
        : mpctrlComboAnalysisMethod->currentIndex();

    const bool _bFrequency = (_nMethodIdx == 1);
    const bool _bNeighborhood = (_nMethodIdx == 2);

    mpctrlSpinFrequencyBins->setEnabled(_bFrequency);
    mpctrlSpinNeighborhoodWindow->setEnabled(_bNeighborhood);
}
