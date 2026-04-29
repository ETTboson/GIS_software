#include "analysisworkspacedockwidget.h"

#include "service/data/datarepository.h"
#include "ui/components/antbutton.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLayoutItem>
#include <QListWidget>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QStringList>
#include <QVBoxLayout>

AnalysisWorkspaceDockWidget::AnalysisWorkspaceDockWidget(QWidget* _pParent)
    : QDockWidget(tr("Analysis Workspace"), _pParent)
    , mpDataRepository(nullptr)
    , mpctrlContainer(new QWidget(this))
    , mpctrlTabWidget(new QTabWidget(mpctrlContainer))
    , mpctrlPageData(new QWidget(mpctrlTabWidget))
    , mpctrlLabelDataHint(new QLabel(tr("请选择左侧数据资产，查看其摘要与能力。"), mpctrlPageData))
    , mpctrlAssetList(new QListWidget(mpctrlPageData))
    , mpctrlAssetSummaryView(new QTextEdit(mpctrlPageData))
    , mpctrlPageTools(new QWidget(mpctrlTabWidget))
    , mpctrlLabelToolsHint(new QLabel(tr("请选择一个数据资产后，工具会按能力自动启用。"), mpctrlPageTools))
    , mpctrlBtnBasicStatistics(new AntButton(tr("运行基础统计"), mpctrlPageTools))
    , mpctrlBtnFrequencyStatistics(new AntButton(tr("运行频率统计"), mpctrlPageTools))
    , mpctrlSpinFrequencyBins(new QSpinBox(mpctrlPageTools))
    , mpctrlBtnNeighborhood(new AntButton(tr("运行邻域分析"), mpctrlPageTools))
    , mpctrlSpinNeighborhoodWindow(new QSpinBox(mpctrlPageTools))
    , mpctrlBtnBuffer(new AntButton(tr("缓冲分析（开发中）"), mpctrlPageTools))
    , mpctrlBtnOverlay(new AntButton(tr("叠加分析（开发中）"), mpctrlPageTools))
    , mpctrlBtnSpatialQuery(new AntButton(tr("空间查询（开发中）"), mpctrlPageTools))
    , mpctrlBtnRasterCalc(new AntButton(tr("栅格计算（开发中）"), mpctrlPageTools))
    , mpctrlBtnAttributeQuery(new AntButton(tr("属性查询（即将支持）"), mpctrlPageTools))
    , mpctrlPageResults(new QWidget(mpctrlTabWidget))
    , mpctrlResultView(new QTextEdit(mpctrlPageResults))
    , mpctrlLabelChartTitle(new QLabel(tr("暂无可视化"), mpctrlPageResults))
    , mpctrlChartStack(new QStackedWidget(mpctrlPageResults))
    , mpctrlPagePlaceholder(new QWidget(mpctrlChartStack))
    , mpctrlPageBarChart(new QWidget(mpctrlChartStack))
    , mpctrlPageLineChart(new QWidget(mpctrlChartStack))
    , mpctrlLabelPlaceholder(new QLabel(tr("请先选择数据资产并运行分析。"), mpctrlPagePlaceholder))
    , mpctrlVisualizationDetailView(new QTextEdit(mpctrlPageResults))
    , mpctrlHistoryList(new QListWidget(mpctrlPageResults))
{
    setObjectName("dockAnalysisWorkspace");
    setAllowedAreas(Qt::AllDockWidgetAreas);

    QVBoxLayout* _pMainLayout = new QVBoxLayout(mpctrlContainer);
    _pMainLayout->setContentsMargins(0, 0, 0, 0);
    _pMainLayout->addWidget(mpctrlTabWidget);
    setWidget(mpctrlContainer);

    mpctrlTabWidget->addTab(mpctrlPageData, tr("Data"));
    mpctrlTabWidget->addTab(mpctrlPageTools, tr("Tools"));
    mpctrlTabWidget->addTab(mpctrlPageResults, tr("Results"));

    mpctrlLabelDataHint->setWordWrap(true);
    mpctrlAssetSummaryView->setReadOnly(true);
    mpctrlAssetSummaryView->setPlaceholderText(tr("数据资产摘要将在这里显示。"));

    QHBoxLayout* _pDataBodyLayout = new QHBoxLayout();
    _pDataBodyLayout->setContentsMargins(0, 0, 0, 0);
    _pDataBodyLayout->setSpacing(8);
    _pDataBodyLayout->addWidget(mpctrlAssetList, 2);
    _pDataBodyLayout->addWidget(mpctrlAssetSummaryView, 3);

    QVBoxLayout* _pDataLayout = new QVBoxLayout(mpctrlPageData);
    _pDataLayout->setContentsMargins(8, 8, 8, 8);
    _pDataLayout->setSpacing(8);
    _pDataLayout->addWidget(mpctrlLabelDataHint);
    _pDataLayout->addLayout(_pDataBodyLayout, 1);

    mpctrlLabelToolsHint->setWordWrap(true);
    mpctrlSpinFrequencyBins->setRange(2, 100);
    mpctrlSpinFrequencyBins->setValue(10);
    mpctrlSpinNeighborhoodWindow->setRange(3, 15);
    mpctrlSpinNeighborhoodWindow->setSingleStep(2);
    mpctrlSpinNeighborhoodWindow->setValue(3);
    mpctrlBtnBasicStatistics->setButtonRole(AntButton::Primary);
    mpctrlBtnFrequencyStatistics->setButtonRole(AntButton::Default);
    mpctrlBtnNeighborhood->setButtonRole(AntButton::Default);
    mpctrlBtnBuffer->setButtonRole(AntButton::Quiet);
    mpctrlBtnOverlay->setButtonRole(AntButton::Quiet);
    mpctrlBtnSpatialQuery->setButtonRole(AntButton::Quiet);
    mpctrlBtnRasterCalc->setButtonRole(AntButton::Quiet);
    mpctrlBtnAttributeQuery->setButtonRole(AntButton::Default);

    QFormLayout* _pStatLayout = new QFormLayout();
    _pStatLayout->addRow(tr("基础统计"), mpctrlBtnBasicStatistics);
    _pStatLayout->addRow(tr("频率统计分箱数"), mpctrlSpinFrequencyBins);
    _pStatLayout->addRow(tr("频率统计"), mpctrlBtnFrequencyStatistics);

    QGroupBox* _pGroupStat = new QGroupBox(tr("Statistical Analysis"), mpctrlPageTools);
    _pGroupStat->setLayout(_pStatLayout);

    QFormLayout* _pSpatialLayout = new QFormLayout();
    _pSpatialLayout->addRow(tr("邻域窗口大小"), mpctrlSpinNeighborhoodWindow);
    _pSpatialLayout->addRow(tr("邻域分析"), mpctrlBtnNeighborhood);
    _pSpatialLayout->addRow(tr("缓冲分析"), mpctrlBtnBuffer);
    _pSpatialLayout->addRow(tr("叠加分析"), mpctrlBtnOverlay);
    _pSpatialLayout->addRow(tr("空间查询"), mpctrlBtnSpatialQuery);
    _pSpatialLayout->addRow(tr("栅格计算"), mpctrlBtnRasterCalc);

    QGroupBox* _pGroupSpatial = new QGroupBox(tr("Spatial Analysis"), mpctrlPageTools);
    _pGroupSpatial->setLayout(_pSpatialLayout);

    QFormLayout* _pAttributeLayout = new QFormLayout();
    _pAttributeLayout->addRow(tr("属性查询"), mpctrlBtnAttributeQuery);

    QGroupBox* _pGroupAttribute = new QGroupBox(tr("Attribute Query"), mpctrlPageTools);
    _pGroupAttribute->setLayout(_pAttributeLayout);

    QVBoxLayout* _pToolsLayout = new QVBoxLayout(mpctrlPageTools);
    _pToolsLayout->setContentsMargins(8, 8, 8, 8);
    _pToolsLayout->setSpacing(8);
    _pToolsLayout->addWidget(mpctrlLabelToolsHint);
    _pToolsLayout->addWidget(_pGroupStat);
    _pToolsLayout->addWidget(_pGroupSpatial);
    _pToolsLayout->addWidget(_pGroupAttribute);
    _pToolsLayout->addStretch(1);

    mpctrlBtnBuffer->setEnabled(false);
    mpctrlBtnOverlay->setEnabled(false);
    mpctrlBtnSpatialQuery->setEnabled(false);
    mpctrlBtnRasterCalc->setEnabled(false);

    mpctrlResultView->setReadOnly(true);
    mpctrlResultView->setPlaceholderText(tr("分析文本结果将在这里显示。"));
    mpctrlLabelChartTitle->setStyleSheet("font-weight:600;color:#1F2933;");
    mpctrlLabelChartTitle->setWordWrap(true);

    QVBoxLayout* _pPlaceholderLayout = new QVBoxLayout(mpctrlPagePlaceholder);
    _pPlaceholderLayout->setContentsMargins(16, 16, 16, 16);
    mpctrlLabelPlaceholder->setAlignment(Qt::AlignCenter);
    mpctrlLabelPlaceholder->setWordWrap(true);
    _pPlaceholderLayout->addWidget(mpctrlLabelPlaceholder, 1);

    QVBoxLayout* _pBarLayout = new QVBoxLayout(mpctrlPageBarChart);
    _pBarLayout->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout* _pLineLayout = new QVBoxLayout(mpctrlPageLineChart);
    _pLineLayout->setContentsMargins(0, 0, 0, 0);

    mpctrlChartStack->addWidget(mpctrlPagePlaceholder);
    mpctrlChartStack->addWidget(mpctrlPageBarChart);
    mpctrlChartStack->addWidget(mpctrlPageLineChart);

    mpctrlVisualizationDetailView->setReadOnly(true);
    mpctrlVisualizationDetailView->setPlaceholderText(
        tr("图表详情将在这里显示。"));
    mpctrlHistoryList->setMinimumHeight(120);

    QWidget* _pResultsContent = new QWidget(mpctrlPageResults);
    QVBoxLayout* _pResultsContentLayout = new QVBoxLayout(_pResultsContent);
    _pResultsContentLayout->setContentsMargins(0, 0, 0, 0);
    _pResultsContentLayout->setSpacing(8);
    _pResultsContentLayout->addWidget(mpctrlResultView, 1);
    _pResultsContentLayout->addWidget(mpctrlLabelChartTitle);
    _pResultsContentLayout->addWidget(mpctrlChartStack, 2);
    _pResultsContentLayout->addWidget(mpctrlVisualizationDetailView, 1);

    QWidget* _pHistoryPanel = new QWidget(mpctrlPageResults);
    QVBoxLayout* _pHistoryLayout = new QVBoxLayout(_pHistoryPanel);
    _pHistoryLayout->setContentsMargins(0, 0, 0, 0);
    _pHistoryLayout->setSpacing(4);
    _pHistoryLayout->addWidget(new QLabel(tr("Recent Results"), _pHistoryPanel));
    _pHistoryLayout->addWidget(mpctrlHistoryList, 1);

    QSplitter* _pResultsSplitter = new QSplitter(Qt::Vertical, mpctrlPageResults);
    _pResultsSplitter->addWidget(_pResultsContent);
    _pResultsSplitter->addWidget(_pHistoryPanel);
    _pResultsSplitter->setStretchFactor(0, 4);
    _pResultsSplitter->setStretchFactor(1, 1);

    QVBoxLayout* _pResultsLayout = new QVBoxLayout(mpctrlPageResults);
    _pResultsLayout->setContentsMargins(8, 8, 8, 8);
    _pResultsLayout->setSpacing(8);
    _pResultsLayout->addWidget(_pResultsSplitter, 1);

    connect(mpctrlAssetList, &QListWidget::currentItemChanged,
        this, &AnalysisWorkspaceDockWidget::onAssetListCurrentItemChanged);
    connect(mpctrlBtnBasicStatistics, &QPushButton::clicked,
        this, &AnalysisWorkspaceDockWidget::basicStatisticsRequested);
    connect(mpctrlBtnFrequencyStatistics, &QPushButton::clicked,
        this, [this]() {
            emit frequencyStatisticsRequested(mpctrlSpinFrequencyBins->value());
        });
    connect(mpctrlBtnNeighborhood, &QPushButton::clicked,
        this, [this]() {
            emit neighborhoodAnalysisRequested(mpctrlSpinNeighborhoodWindow->value());
        });
    connect(mpctrlBtnAttributeQuery, &QPushButton::clicked,
        this, &AnalysisWorkspaceDockWidget::attributeQueryRequested);
    connect(mpctrlHistoryList, &QListWidget::currentRowChanged,
        this, &AnalysisWorkspaceDockWidget::onHistoryItemCurrentRowChanged);

    clearCurrentResult(tr("请先选择数据资产并运行分析。"));
}

void AnalysisWorkspaceDockWidget::setDataRepository(DataRepository* _pRepository)
{
    if (mpDataRepository == _pRepository) {
        return;
    }

    if (mpDataRepository != nullptr) {
        disconnect(mpDataRepository, nullptr, this, nullptr);
    }

    mpDataRepository = _pRepository;
    if (mpDataRepository == nullptr) {
        refreshAssetList();
        onCurrentAssetCleared();
        return;
    }

    connect(mpDataRepository, &DataRepository::assetListChanged,
        this, &AnalysisWorkspaceDockWidget::refreshAssetList);
    connect(mpDataRepository, &DataRepository::currentAssetChanged,
        this, &AnalysisWorkspaceDockWidget::onCurrentAssetChanged);
    connect(mpDataRepository, &DataRepository::currentAssetCleared,
        this, &AnalysisWorkspaceDockWidget::onCurrentAssetCleared);

    refreshAssetList();
    if (mpDataRepository->hasCurrentAsset()) {
        onCurrentAssetChanged(mpDataRepository->currentAsset());
    } else {
        onCurrentAssetCleared();
    }
}

void AnalysisWorkspaceDockWidget::showDataPage(const QString& _strHint)
{
    if (!_strHint.trimmed().isEmpty()) {
        mpctrlLabelDataHint->setText(_strHint);
    }
    mpctrlTabWidget->setCurrentWidget(mpctrlPageData);
}

void AnalysisWorkspaceDockWidget::showToolsPage(const QString& _strHint)
{
    if (!_strHint.trimmed().isEmpty()) {
        mpctrlLabelToolsHint->setText(_strHint);
    }
    mpctrlTabWidget->setCurrentWidget(mpctrlPageTools);
}

void AnalysisWorkspaceDockWidget::showResultsPage()
{
    mpctrlTabWidget->setCurrentWidget(mpctrlPageResults);
}

void AnalysisWorkspaceDockWidget::focusTool(const QString& _strToolId,
    const QString& _strHint)
{
    showToolsPage(_strHint.isEmpty()
        ? tr("当前聚焦工具：%1").arg(ToolDisplayName(_strToolId))
        : _strHint);

    if (_strToolId == "basic_statistics") {
        mpctrlBtnBasicStatistics->setFocus();
    } else if (_strToolId == "frequency_statistics") {
        mpctrlBtnFrequencyStatistics->setFocus();
    } else if (_strToolId == "neighborhood_analysis") {
        mpctrlBtnNeighborhood->setFocus();
    } else if (_strToolId == "attribute_query") {
        mpctrlBtnAttributeQuery->setFocus();
    } else if (_strToolId == "buffer_analysis") {
        mpctrlBtnBuffer->setFocus();
    } else if (_strToolId == "overlay_analysis") {
        mpctrlBtnOverlay->setFocus();
    } else if (_strToolId == "spatial_query") {
        mpctrlBtnSpatialQuery->setFocus();
    } else if (_strToolId == "raster_calc") {
        mpctrlBtnRasterCalc->setFocus();
    }
}

void AnalysisWorkspaceDockWidget::setCurrentResult(const AnalysisResult& _result)
{
    mResultCurrent = _result;
    mpctrlResultView->setPlainText(_result.strDesc);
    showResultsPage();
}

void AnalysisWorkspaceDockWidget::clearCurrentResult(const QString& _strMessage)
{
    mResultCurrent = AnalysisResult();
    mpctrlResultView->setPlainText(_strMessage);
    showVisualizationPlaceholder(_strMessage);
    setVisualizationDetailText(_strMessage);
}

void AnalysisWorkspaceDockWidget::addResultHistory(const AnalysisResult& _result)
{
    QListWidgetItem* _pItem = new QListWidgetItem(
        tr("%1 | %2")
            .arg(_result.strSourceAssetName.isEmpty()
                ? tr("未命名资产")
                : _result.strSourceAssetName,
                ToolDisplayName(_result.strToolId)));
    _pItem->setData(Qt::UserRole, _result.strDesc);
    _pItem->setData(Qt::UserRole + 1, _result.strToolId);
    _pItem->setToolTip(_result.strDesc);
    mpctrlHistoryList->insertItem(0, _pItem);
    mpctrlHistoryList->setCurrentItem(_pItem);
}

void AnalysisWorkspaceDockWidget::setChartWidget(VisualizationChartType _chartType,
    QWidget* _pWidget)
{
    QWidget* _pPage = pageForChartType(_chartType);
    if (_pPage == nullptr || _pWidget == nullptr) {
        return;
    }

    QLayout* _pLayout = _pPage->layout();
    if (_pLayout == nullptr) {
        return;
    }

    while (_pLayout->count() > 0) {
        QLayoutItem* _pItem = _pLayout->takeAt(0);
        if (_pItem == nullptr) {
            continue;
        }
        if (_pItem->widget() != nullptr) {
            _pItem->widget()->setParent(nullptr);
        }
        delete _pItem;
    }

    _pLayout->addWidget(_pWidget);
}

void AnalysisWorkspaceDockWidget::showChart(VisualizationChartType _chartType,
    const QString& _strTitle)
{
    mpctrlLabelChartTitle->setText(_strTitle);
    QWidget* _pPage = pageForChartType(_chartType);
    if (_pPage != nullptr) {
        mpctrlChartStack->setCurrentWidget(_pPage);
    }
}

void AnalysisWorkspaceDockWidget::showVisualizationPlaceholder(
    const QString& _strMessage)
{
    mpctrlLabelChartTitle->setText(tr("暂无可视化"));
    mpctrlLabelPlaceholder->setText(_strMessage);
    mpctrlChartStack->setCurrentWidget(mpctrlPagePlaceholder);
}

void AnalysisWorkspaceDockWidget::setVisualizationDetailText(
    const QString& _strDetailText)
{
    mpctrlVisualizationDetailView->setPlainText(_strDetailText);
}

void AnalysisWorkspaceDockWidget::onAssetListCurrentItemChanged(
    QListWidgetItem* _pCurrent,
    QListWidgetItem* _pPrevious)
{
    Q_UNUSED(_pPrevious)
    if (mpDataRepository == nullptr || _pCurrent == nullptr) {
        return;
    }

    mpDataRepository->selectAssetById(_pCurrent->data(Qt::UserRole).toString());
}

void AnalysisWorkspaceDockWidget::onCurrentAssetChanged(
    const AnalysisDataAsset& _assetCurrent)
{
    for (int _nItemIdx = 0; _nItemIdx < mpctrlAssetList->count(); ++_nItemIdx) {
        QListWidgetItem* _pItem = mpctrlAssetList->item(_nItemIdx);
        if (_pItem != nullptr
            && _pItem->data(Qt::UserRole).toString() == _assetCurrent.strAssetId) {
            QSignalBlocker _blocker(mpctrlAssetList);
            mpctrlAssetList->setCurrentItem(_pItem);
            break;
        }
    }

    updateCurrentAssetView(_assetCurrent);
}

void AnalysisWorkspaceDockWidget::onCurrentAssetCleared()
{
    {
        QSignalBlocker _blocker(mpctrlAssetList);
        mpctrlAssetList->clearSelection();
    }
    mpctrlAssetSummaryView->setPlainText(tr("当前没有已选择的分析资产。"));
    updateToolStates(AnalysisDataAsset());
}

void AnalysisWorkspaceDockWidget::onHistoryItemCurrentRowChanged(int _nCurrentRow)
{
    if (_nCurrentRow < 0) {
        return;
    }

    QListWidgetItem* _pItem = mpctrlHistoryList->item(_nCurrentRow);
    if (_pItem == nullptr) {
        return;
    }

    mpctrlResultView->setPlainText(_pItem->data(Qt::UserRole).toString());
}

void AnalysisWorkspaceDockWidget::refreshAssetList()
{
    QSignalBlocker _blocker(mpctrlAssetList);
    mpctrlAssetList->clear();

    if (mpDataRepository == nullptr) {
        return;
    }

    const QList<AnalysisDataAsset> _vAssets = mpDataRepository->getAssets();
    for (const AnalysisDataAsset& _assetCurrent : _vAssets) {
        QListWidgetItem* _pItem = new QListWidgetItem(
            tr("%1 (%2)")
                .arg(_assetCurrent.strName.isEmpty()
                    ? tr("未命名资产")
                    : _assetCurrent.strName,
                    _assetCurrent.strSourceFormat),
            mpctrlAssetList);
        _pItem->setData(Qt::UserRole, _assetCurrent.strAssetId);
        _pItem->setToolTip(_assetCurrent.strSummary);
    }
}

void AnalysisWorkspaceDockWidget::updateCurrentAssetView(
    const AnalysisDataAsset& _assetCurrent)
{
    mpctrlAssetSummaryView->setPlainText(_assetCurrent.strSummary.isEmpty()
        ? tr("当前资产暂无摘要信息。")
        : _assetCurrent.strSummary);
    updateToolStates(_assetCurrent);
}

void AnalysisWorkspaceDockWidget::updateToolStates(
    const AnalysisDataAsset& _assetCurrent)
{
    const bool _bHasAsset = !_assetCurrent.strAssetId.isEmpty();
    const bool _bCanStat = _assetCurrent.flagsCapabilities.testFlag(
        AnalysisCapability::Statistical);
    const bool _bCanRaster = _assetCurrent.flagsCapabilities.testFlag(
        AnalysisCapability::SpatialRaster);
    const bool _bCanVector = _assetCurrent.flagsCapabilities.testFlag(
        AnalysisCapability::SpatialVector);
    const bool _bCanAttributeQuery = _assetCurrent.flagsCapabilities.testFlag(
        AnalysisCapability::AttributeQuery);

    mpctrlBtnBasicStatistics->setEnabled(_bHasAsset && _bCanStat);
    mpctrlBtnFrequencyStatistics->setEnabled(_bHasAsset && _bCanStat);
    mpctrlSpinFrequencyBins->setEnabled(_bHasAsset && _bCanStat);
    mpctrlBtnNeighborhood->setEnabled(_bHasAsset && _bCanRaster);
    mpctrlSpinNeighborhoodWindow->setEnabled(_bHasAsset && _bCanRaster);
    mpctrlBtnAttributeQuery->setEnabled(_bHasAsset && _bCanAttributeQuery);

    const QString _strVectorTip = _bCanVector
        ? tr("当前数据具备矢量空间分析能力边界，但缓冲/叠加/空间查询仍在开发中。")
        : tr("当前所选资产不具备矢量空间分析能力。");
    mpctrlBtnBuffer->setToolTip(_strVectorTip);
    mpctrlBtnOverlay->setToolTip(_strVectorTip);
    mpctrlBtnSpatialQuery->setToolTip(_strVectorTip);

    const QString _strRasterCalcTip = _bCanRaster
        ? tr("当前栅格资产已支持邻域分析，栅格计算仍在开发中。")
        : tr("当前所选资产不具备栅格空间分析能力。");
    mpctrlBtnRasterCalc->setToolTip(_strRasterCalcTip);

    if (!_bHasAsset) {
        mpctrlLabelToolsHint->setText(tr("请先在 Data 页选择一个分析资产。"));
    } else {
        mpctrlLabelToolsHint->setText(tr(
            "当前资产：%1\n"
            "可用能力会自动启用；开发中工具保持占位，避免误执行。")
            .arg(_assetCurrent.strName));
    }
}

QWidget* AnalysisWorkspaceDockWidget::pageForChartType(
    VisualizationChartType _chartType) const
{
    switch (_chartType) {
    case VisualizationChartType::BarChart:
        return mpctrlPageBarChart;
    case VisualizationChartType::LineChart:
        return mpctrlPageLineChart;
    default:
        return nullptr;
    }
}

QString AnalysisWorkspaceDockWidget::ToolDisplayName(const QString& _strToolId)
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
