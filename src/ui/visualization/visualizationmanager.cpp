#include "visualizationmanager.h"

#include "ui/visualization/visualizationdockwidget.h"
#include "ui/visualization/charts/barchartwidget.h"
#include "ui/visualization/charts/linechartwidget.h"

VisualizationManager::VisualizationManager(QObject* _pParent)
    : QObject(_pParent)
    , mpDockWidget(nullptr)
    , mpctrlBarChartWidget(nullptr)
    , mpctrlLineChartWidget(nullptr)
{
}

void VisualizationManager::attachDock(VisualizationDockWidget* _pDockWidget)
{
    mpDockWidget = _pDockWidget;
    if (mpDockWidget == nullptr) {
        return;
    }

    if (mpctrlBarChartWidget == nullptr) {
        mpctrlBarChartWidget = new BarChartWidget();
        connect(mpctrlBarChartWidget, &BarChartWidget::pointSelected,
            this, &VisualizationManager::onPointSelected);
    }

    if (mpctrlLineChartWidget == nullptr) {
        mpctrlLineChartWidget = new LineChartWidget();
        connect(mpctrlLineChartWidget, &LineChartWidget::pointSelected,
            this, &VisualizationManager::onPointSelected);
    }

    mpDockWidget->setChartWidget(VisualizationChartType::BarChart, mpctrlBarChartWidget);
    mpDockWidget->setChartWidget(VisualizationChartType::LineChart, mpctrlLineChartWidget);
    clearView(tr("请先加载数据并执行分析。"));
}

void VisualizationManager::updateFromResult(const AnalysisResult& _result)
{
    if (mpDockWidget == nullptr) {
        return;
    }

    if (!_result.bSuccess || !_result.bHasVisualization) {
        clearView(_result.strDesc);
        return;
    }

    mResultCurrent = _result;
    if (mpctrlBarChartWidget != nullptr
        && _result.dataVisualization.chartTypeSuggested == VisualizationChartType::BarChart) {
        mpctrlBarChartWidget->setVisualizationData(_result.dataVisualization);
    }

    if (mpctrlLineChartWidget != nullptr
        && _result.dataVisualization.chartTypeSuggested == VisualizationChartType::LineChart) {
        mpctrlLineChartWidget->setVisualizationData(_result.dataVisualization);
    }

    mpDockWidget->showChart(
        _result.dataVisualization.chartTypeSuggested,
        _result.dataVisualization.strTitle);
    mpDockWidget->setDetailText(buildSummaryText(_result));
    mpDockWidget->setVisible(true);
}

void VisualizationManager::clearView(const QString& _strMessage)
{
    mResultCurrent = AnalysisResult();

    if (mpctrlBarChartWidget != nullptr) {
        mpctrlBarChartWidget->clearVisualization();
    }
    if (mpctrlLineChartWidget != nullptr) {
        mpctrlLineChartWidget->clearVisualization();
    }

    if (mpDockWidget != nullptr) {
        const QString _strPlaceholder = _strMessage.isEmpty()
            ? tr("请先加载数据并执行分析。")
            : _strMessage;
        const QString _strDetailText = _strMessage.isEmpty()
            ? tr("可视化结果详情将在这里显示。")
            : _strMessage;
        mpDockWidget->showPlaceholder(_strPlaceholder);
        mpDockWidget->setDetailText(_strDetailText);
    }
}

void VisualizationManager::onPointSelected(const VisualizationPoint& _point)
{
    if (mpDockWidget == nullptr) {
        return;
    }

    mpDockWidget->setDetailText(buildPointDetailText(_point));
}

QString VisualizationManager::buildSummaryText(const AnalysisResult& _result) const
{
    QString _strSummary = _result.strDesc;
    if (_result.bHasVisualization) {
        _strSummary += tr("\n\n图表类型：%1")
            .arg(_result.dataVisualization.chartTypeSuggested == VisualizationChartType::BarChart
                ? tr("柱状图")
                : tr("折线图"));
        _strSummary += tr("\n图表标题：%1").arg(_result.dataVisualization.strTitle);
    }
    return _strSummary;
}

QString VisualizationManager::buildPointDetailText(const VisualizationPoint& _point) const
{
    QString _strDetail = tr("数据点：%1\n数值：%2")
        .arg(_point.strLabel)
        .arg(_point.dValue, 0, 'f', 6);

    if (_point.nSourceIndex >= 0) {
        _strDetail += tr("\n源序号：%1").arg(_point.nSourceIndex);
    }
    if (_point.nRowIndex >= 0) {
        _strDetail += tr("\n行索引：%1").arg(_point.nRowIndex);
    }
    if (_point.nColIndex >= 0) {
        _strDetail += tr("\n列索引：%1").arg(_point.nColIndex);
    }
    if (!mResultCurrent.strType.isEmpty()) {
        _strDetail += tr("\n分析类型：%1").arg(mResultCurrent.strType);
    }
    if (!_point.strToolTip.isEmpty()) {
        _strDetail += tr("\n\n说明：\n%1").arg(_point.strToolTip);
    }
    return _strDetail;
}
