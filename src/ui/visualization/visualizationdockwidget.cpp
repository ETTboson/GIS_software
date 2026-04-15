#include "visualizationdockwidget.h"

#include <QLabel>
#include <QLayoutItem>
#include <QStackedWidget>
#include <QTextEdit>
#include <QVBoxLayout>

VisualizationDockWidget::VisualizationDockWidget(QWidget* _pParent)
    : QDockWidget(tr("可视化结果"), _pParent)
    , mpctrlContainer(new QWidget(this))
    , mpctrlLabelTitle(new QLabel(tr("暂无可视化"), mpctrlContainer))
    , mpctrlChartStack(new QStackedWidget(mpctrlContainer))
    , mpctrlPagePlaceholder(new QWidget(mpctrlChartStack))
    , mpctrlPageBarChart(new QWidget(mpctrlChartStack))
    , mpctrlPageLineChart(new QWidget(mpctrlChartStack))
    , mpctrlLabelPlaceholder(new QLabel(tr("请先加载数据并执行分析。"), mpctrlPagePlaceholder))
    , mpctrlDetailView(new QTextEdit(mpctrlContainer))
{
    setObjectName("dockVisualization");
    setAllowedAreas(Qt::AllDockWidgetAreas);

    QVBoxLayout* _pMainLayout = new QVBoxLayout(mpctrlContainer);
    _pMainLayout->setContentsMargins(8, 8, 8, 8);
    _pMainLayout->setSpacing(8);

    mpctrlLabelTitle->setWordWrap(true);
    mpctrlLabelTitle->setStyleSheet("font-weight:600;color:#1F2933;");

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

    mpctrlDetailView->setReadOnly(true);
    mpctrlDetailView->setPlaceholderText(tr("点击图表中的数据点后，这里会显示详细说明。"));
    mpctrlDetailView->setMinimumHeight(140);

    _pMainLayout->addWidget(mpctrlLabelTitle);
    _pMainLayout->addWidget(mpctrlChartStack, 1);
    _pMainLayout->addWidget(mpctrlDetailView);

    setWidget(mpctrlContainer);
    showPlaceholder(tr("请先加载数据并执行分析。"));
}

void VisualizationDockWidget::setChartWidget(VisualizationChartType _chartType, QWidget* _pWidget)
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

void VisualizationDockWidget::showChart(VisualizationChartType _chartType, const QString& _strTitle)
{
    mpctrlLabelTitle->setText(_strTitle);
    QWidget* _pPage = pageForChartType(_chartType);
    if (_pPage != nullptr) {
        mpctrlChartStack->setCurrentWidget(_pPage);
    }
}

void VisualizationDockWidget::showPlaceholder(const QString& _strMessage)
{
    mpctrlLabelTitle->setText(tr("暂无可视化"));
    mpctrlLabelPlaceholder->setText(_strMessage);
    mpctrlChartStack->setCurrentWidget(mpctrlPagePlaceholder);
}

void VisualizationDockWidget::setDetailText(const QString& _strDetailText)
{
    mpctrlDetailView->setPlainText(_strDetailText);
}

QWidget* VisualizationDockWidget::pageForChartType(VisualizationChartType _chartType) const
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
