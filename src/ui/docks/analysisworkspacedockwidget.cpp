#include "analysisworkspacedockwidget.h"

#include "service/data/datarepository.h"
#include "ui/components/antbutton.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
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
    , mpctrlComboToolSelector(new QComboBox(mpctrlPageTools))
    , mpctrlToolStack(new QStackedWidget(mpctrlPageTools))
    , mpctrlBtnBasicStatistics(new AntButton(tr("运行基础统计"), mpctrlPageTools))
    , mpctrlBtnFrequencyStatistics(new AntButton(tr("运行频率统计"), mpctrlPageTools))
    , mpctrlSpinFrequencyBins(new QSpinBox(mpctrlPageTools))
    , mpctrlBtnNeighborhood(new AntButton(tr("运行邻域分析"), mpctrlPageTools))
    , mpctrlSpinNeighborhoodWindow(new QSpinBox(mpctrlPageTools))
    , mpctrlSpinBufferDistance(new QDoubleSpinBox(mpctrlPageTools))
    , mpctrlSpinBufferSegments(new QSpinBox(mpctrlPageTools))
    , mpctrlBtnBuffer(new AntButton(tr("运行缓冲分析"), mpctrlPageTools))
    , mpctrlComboOverlayOperation(new QComboBox(mpctrlPageTools))
    , mpctrlComboOverlayTarget(new QComboBox(mpctrlPageTools))
    , mpctrlBtnOverlay(new AntButton(tr("运行叠加分析"), mpctrlPageTools))
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
    mpctrlSpinBufferDistance->setRange(0.000001, 999999999.0);
    mpctrlSpinBufferDistance->setDecimals(6);
    mpctrlSpinBufferDistance->setValue(100.0);
    mpctrlSpinBufferDistance->setSingleStep(10.0);
    mpctrlSpinBufferSegments->setRange(1, 64);
    mpctrlSpinBufferSegments->setValue(8);
    mpctrlComboToolSelector->setMinimumWidth(180);
    mpctrlComboOverlayOperation->addItem(
        tr("Intersect"), static_cast<int>(OverlayOperationType::Intersect));
    mpctrlComboOverlayOperation->addItem(
        tr("Union"), static_cast<int>(OverlayOperationType::Union));
    mpctrlComboOverlayTarget->setMinimumWidth(180);
    mpctrlBtnBasicStatistics->setButtonRole(AntButton::Primary);
    mpctrlBtnFrequencyStatistics->setButtonRole(AntButton::Default);
    mpctrlBtnNeighborhood->setButtonRole(AntButton::Default);
    mpctrlBtnBuffer->setButtonRole(AntButton::Quiet);
    mpctrlBtnOverlay->setButtonRole(AntButton::Quiet);
    mpctrlBtnSpatialQuery->setButtonRole(AntButton::Quiet);
    mpctrlBtnRasterCalc->setButtonRole(AntButton::Quiet);
    mpctrlBtnAttributeQuery->setButtonRole(AntButton::Default);

    auto _fnMakeToolPage = [this](QFormLayout* _pFormLayout) -> QWidget* {
        QWidget* _pPage = new QWidget(mpctrlToolStack);
        QVBoxLayout* _pPageLayout = new QVBoxLayout(_pPage);
        _pPageLayout->setContentsMargins(0, 0, 0, 0);
        _pPageLayout->setSpacing(8);
        _pPageLayout->addLayout(_pFormLayout);
        _pPageLayout->addStretch(1);
        return _pPage;
    };

    auto _fnAddToolPage = [this](const QString& _strToolId,
        const QString& _strToolName,
        QWidget* _pPage) {
        mpctrlComboToolSelector->addItem(_strToolName, _strToolId);
        mpctrlToolStack->addWidget(_pPage);
    };

    QFormLayout* _pBasicLayout = new QFormLayout();
    _pBasicLayout->addRow(tr("基础统计"), mpctrlBtnBasicStatistics);
    _fnAddToolPage("basic_statistics", tr("基础统计"),
        _fnMakeToolPage(_pBasicLayout));

    QFormLayout* _pFrequencyLayout = new QFormLayout();
    _pFrequencyLayout->addRow(tr("分箱数"), mpctrlSpinFrequencyBins);
    _pFrequencyLayout->addRow(tr("频率统计"), mpctrlBtnFrequencyStatistics);
    _fnAddToolPage("frequency_statistics", tr("频率统计"),
        _fnMakeToolPage(_pFrequencyLayout));

    QFormLayout* _pNeighborhoodLayout = new QFormLayout();
    _pNeighborhoodLayout->addRow(tr("窗口大小"), mpctrlSpinNeighborhoodWindow);
    _pNeighborhoodLayout->addRow(tr("邻域分析"), mpctrlBtnNeighborhood);
    _fnAddToolPage("neighborhood_analysis", tr("邻域分析"),
        _fnMakeToolPage(_pNeighborhoodLayout));

    QFormLayout* _pBufferLayout = new QFormLayout();
    _pBufferLayout->addRow(tr("缓冲距离"), mpctrlSpinBufferDistance);
    _pBufferLayout->addRow(tr("圆弧分段数"), mpctrlSpinBufferSegments);
    _pBufferLayout->addRow(tr("缓冲分析"), mpctrlBtnBuffer);
    _fnAddToolPage("buffer_analysis", tr("缓冲分析"),
        _fnMakeToolPage(_pBufferLayout));

    QFormLayout* _pOverlayLayout = new QFormLayout();
    _pOverlayLayout->addRow(tr("操作"), mpctrlComboOverlayOperation);
    _pOverlayLayout->addRow(tr("叠加图层"), mpctrlComboOverlayTarget);
    _pOverlayLayout->addRow(tr("叠加分析"), mpctrlBtnOverlay);
    _fnAddToolPage("overlay_analysis", tr("叠加分析"),
        _fnMakeToolPage(_pOverlayLayout));

    QFormLayout* _pSpatialQueryLayout = new QFormLayout();
    _pSpatialQueryLayout->addRow(tr("空间查询"), mpctrlBtnSpatialQuery);
    _fnAddToolPage("spatial_query", tr("空间查询"),
        _fnMakeToolPage(_pSpatialQueryLayout));

    QFormLayout* _pRasterCalcLayout = new QFormLayout();
    _pRasterCalcLayout->addRow(tr("栅格计算"), mpctrlBtnRasterCalc);
    _fnAddToolPage("raster_calc", tr("栅格计算"),
        _fnMakeToolPage(_pRasterCalcLayout));

    QFormLayout* _pAttributeLayout = new QFormLayout();
    _pAttributeLayout->addRow(tr("属性查询"), mpctrlBtnAttributeQuery);
    _fnAddToolPage("attribute_query", tr("属性查询"),
        _fnMakeToolPage(_pAttributeLayout));

    QFormLayout* _pToolSelectorLayout = new QFormLayout();
    _pToolSelectorLayout->addRow(tr("分析方法"), mpctrlComboToolSelector);

    QVBoxLayout* _pToolsLayout = new QVBoxLayout(mpctrlPageTools);
    _pToolsLayout->setContentsMargins(8, 8, 8, 8);
    _pToolsLayout->setSpacing(8);
    _pToolsLayout->addWidget(mpctrlLabelToolsHint);
    _pToolsLayout->addLayout(_pToolSelectorLayout);
    _pToolsLayout->addWidget(mpctrlToolStack, 1);

    mpctrlSpinBufferDistance->setEnabled(false);
    mpctrlSpinBufferSegments->setEnabled(false);
    mpctrlBtnBuffer->setEnabled(false);
    mpctrlComboOverlayOperation->setEnabled(false);
    mpctrlComboOverlayTarget->setEnabled(false);
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
    connect(mpctrlComboToolSelector,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &AnalysisWorkspaceDockWidget::onToolSelectorCurrentIndexChanged);
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
    connect(mpctrlBtnBuffer, &QPushButton::clicked,
        this, [this]() {
            emit bufferAnalysisRequested(
                mpctrlSpinBufferDistance->value(),
                mpctrlSpinBufferSegments->value());
        });
    connect(mpctrlBtnOverlay, &QPushButton::clicked,
        this, [this]() {
            const OverlayOperationType _eOperation =
                static_cast<OverlayOperationType>(
                    mpctrlComboOverlayOperation->currentData().toInt());
            emit overlayAnalysisRequested(
                mpctrlComboOverlayTarget->currentData().toString(),
                _eOperation);
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

    selectToolPage(_strToolId);

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

void AnalysisWorkspaceDockWidget::onToolSelectorCurrentIndexChanged(
    int _nCurrentIndex)
{
    if (_nCurrentIndex < 0 || _nCurrentIndex >= mpctrlToolStack->count()) {
        return;
    }
    mpctrlToolStack->setCurrentIndex(_nCurrentIndex);
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

    refreshOverlayAssetOptions(_assetCurrent);
    const bool _bHasOverlayTarget =
        !mpctrlComboOverlayTarget->currentData().toString().trimmed().isEmpty();

    mpctrlBtnBasicStatistics->setEnabled(_bHasAsset && _bCanStat);
    mpctrlBtnFrequencyStatistics->setEnabled(_bHasAsset && _bCanStat);
    mpctrlSpinFrequencyBins->setEnabled(_bHasAsset && _bCanStat);
    mpctrlBtnNeighborhood->setEnabled(_bHasAsset && _bCanRaster);
    mpctrlSpinNeighborhoodWindow->setEnabled(_bHasAsset && _bCanRaster);
    mpctrlSpinBufferDistance->setEnabled(_bHasAsset && _bCanVector);
    mpctrlSpinBufferSegments->setEnabled(_bHasAsset && _bCanVector);
    mpctrlBtnBuffer->setEnabled(_bHasAsset && _bCanVector);
    mpctrlComboOverlayOperation->setEnabled(_bHasAsset && _bCanVector);
    mpctrlComboOverlayTarget->setEnabled(_bHasAsset && _bCanVector && _bHasOverlayTarget);
    mpctrlBtnOverlay->setEnabled(_bHasAsset && _bCanVector && _bHasOverlayTarget);
    mpctrlBtnAttributeQuery->setEnabled(_bHasAsset && _bCanAttributeQuery);

    const QString _strVectorTip = _bCanVector
        ? tr("当前矢量资产可执行缓冲区分析；叠加分析需要再选择一个矢量资产。")
        : tr("当前所选资产不具备矢量空间分析能力。");
    mpctrlSpinBufferDistance->setToolTip(tr("缓冲距离按源图层 CRS 单位解释。"));
    mpctrlSpinBufferSegments->setToolTip(tr("圆弧分段数用于近似缓冲边界曲线。"));
    mpctrlBtnBuffer->setToolTip(_strVectorTip);
    mpctrlComboOverlayOperation->setToolTip(tr("选择叠加分析的 Intersect 或 Union 操作。"));
    mpctrlComboOverlayTarget->setToolTip(tr("选择参与叠加分析的第二个矢量资产。"));
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
            "可用能力会自动启用；缓冲距离按源图层 CRS 单位解释。")
            .arg(_assetCurrent.strName));
    }
}

void AnalysisWorkspaceDockWidget::refreshOverlayAssetOptions(
    const AnalysisDataAsset& _assetCurrent)
{
    const QString _strPreviousAssetId =
        mpctrlComboOverlayTarget->currentData().toString();

    QSignalBlocker _blocker(mpctrlComboOverlayTarget);
    mpctrlComboOverlayTarget->clear();

    if (mpDataRepository == nullptr || _assetCurrent.strAssetId.trimmed().isEmpty()) {
        mpctrlComboOverlayTarget->addItem(tr("暂无可用叠加资产"), QString());
        return;
    }

    const QList<AnalysisDataAsset> _vAssets = mpDataRepository->getAssets();
    int _nRestoreIndex = -1;
    for (const AnalysisDataAsset& _assetCandidate : _vAssets) {
        if (_assetCandidate.strAssetId == _assetCurrent.strAssetId
            || !_assetCandidate.flagsCapabilities.testFlag(
                AnalysisCapability::SpatialVector)) {
            continue;
        }

        const QString _strLabel = tr("%1 (%2)")
            .arg(_assetCandidate.strName.isEmpty()
                ? tr("未命名资产")
                : _assetCandidate.strName,
                _assetCandidate.strSourceFormat);
        mpctrlComboOverlayTarget->addItem(_strLabel, _assetCandidate.strAssetId);
        if (_assetCandidate.strAssetId == _strPreviousAssetId) {
            _nRestoreIndex = mpctrlComboOverlayTarget->count() - 1;
        }
    }

    if (mpctrlComboOverlayTarget->count() <= 0) {
        mpctrlComboOverlayTarget->addItem(tr("暂无可用叠加资产"), QString());
        return;
    }

    if (_nRestoreIndex >= 0) {
        mpctrlComboOverlayTarget->setCurrentIndex(_nRestoreIndex);
    }
}

void AnalysisWorkspaceDockWidget::selectToolPage(const QString& _strToolId)
{
    for (int _nToolIdx = 0;
        _nToolIdx < mpctrlComboToolSelector->count();
        ++_nToolIdx) {
        if (mpctrlComboToolSelector->itemData(_nToolIdx).toString() != _strToolId) {
            continue;
        }

        QSignalBlocker _blocker(mpctrlComboToolSelector);
        mpctrlComboToolSelector->setCurrentIndex(_nToolIdx);
        mpctrlToolStack->setCurrentIndex(_nToolIdx);
        return;
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
