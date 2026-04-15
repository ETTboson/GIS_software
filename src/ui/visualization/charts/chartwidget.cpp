#include "chartwidget.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

ChartWidget::ChartWidget(QWidget* _pParent)
    : QWidget(_pParent)
    , mnHoveredPointIdx(-1)
    , mnSelectedPointIdx(-1)
{
    setMouseTracking(true);
    setMinimumSize(360, 260);
}

void ChartWidget::setVisualizationData(const VisualizationData& _dataVisualization)
{
    mdataVisualization = _dataVisualization;
    mnHoveredPointIdx = -1;
    mnSelectedPointIdx = -1;
    update();
}

void ChartWidget::clearVisualization()
{
    mdataVisualization = VisualizationData();
    mnHoveredPointIdx = -1;
    mnSelectedPointIdx = -1;
    QToolTip::hideText();
    update();
}

bool ChartWidget::hasVisualization() const
{
    const VisualizationSeries* _pSeries = primarySeries();
    return _pSeries != nullptr && !_pSeries->vPoints.isEmpty();
}

void ChartWidget::paintEvent(QPaintEvent* _pEvent)
{
    Q_UNUSED(_pEvent)

    QPainter _painter(this);
    _painter.setRenderHint(QPainter::Antialiasing, true);
    _painter.fillRect(rect(), QColor("#FFFFFF"));
    _painter.setPen(QPen(QColor("#D0D7DE"), 1.0));
    _painter.drawRect(rect().adjusted(0, 0, -1, -1));

    if (!hasVisualization()) {
        _painter.setPen(QColor("#5C6773"));
        _painter.drawText(rect().adjusted(16, 16, -16, -16),
            Qt::AlignCenter | Qt::TextWordWrap,
            tr("暂无图表数据"));
        return;
    }

    drawChart(_painter, buildPlotArea());
}

void ChartWidget::mouseMoveEvent(QMouseEvent* _pEvent)
{
    if (!hasVisualization()) {
        QWidget::mouseMoveEvent(_pEvent);
        return;
    }

    const int _nPointIdx = hitTestPoint(_pEvent->pos());
    if (_nPointIdx != mnHoveredPointIdx) {
        mnHoveredPointIdx = _nPointIdx;
        update();
    }

    if (_nPointIdx >= 0) {
        QToolTip::showText(_pEvent->globalPos(), toolTipForPoint(_nPointIdx), this);
    } else {
        QToolTip::hideText();
    }

    QWidget::mouseMoveEvent(_pEvent);
}

void ChartWidget::mousePressEvent(QMouseEvent* _pEvent)
{
    if (!hasVisualization()) {
        QWidget::mousePressEvent(_pEvent);
        return;
    }

    const int _nPointIdx = hitTestPoint(_pEvent->pos());
    if (_nPointIdx >= 0) {
        mnSelectedPointIdx = _nPointIdx;
        const VisualizationSeries* _pSeries = primarySeries();
        if (_pSeries != nullptr && _nPointIdx < _pSeries->vPoints.size()) {
            emit pointSelected(_pSeries->vPoints[_nPointIdx]);
        }
        update();
    }

    QWidget::mousePressEvent(_pEvent);
}

void ChartWidget::leaveEvent(QEvent* _pEvent)
{
    mnHoveredPointIdx = -1;
    QToolTip::hideText();
    update();
    QWidget::leaveEvent(_pEvent);
}

const VisualizationData& ChartWidget::visualizationData() const
{
    return mdataVisualization;
}

const VisualizationSeries* ChartWidget::primarySeries() const
{
    if (mdataVisualization.vSeries.isEmpty()) {
        return nullptr;
    }
    return &mdataVisualization.vSeries.first();
}

QRectF ChartWidget::buildPlotArea() const
{
    const QRect _rcWidget = rect();
    const qreal _dLeftMargin = 76.0;
    const qreal _dTopMargin = 28.0;
    const qreal _dRightMargin = 24.0;
    const qreal _dBottomMargin = 84.0;
    return QRectF(_rcWidget.left() + _dLeftMargin,
        _rcWidget.top() + _dTopMargin,
        qMax(120.0, _rcWidget.width() - _dLeftMargin - _dRightMargin),
        qMax(100.0, _rcWidget.height() - _dTopMargin - _dBottomMargin));
}

int ChartWidget::hoveredPointIndex() const
{
    return mnHoveredPointIdx;
}

int ChartWidget::selectedPointIndex() const
{
    return mnSelectedPointIdx;
}

QString ChartWidget::toolTipForPoint(int _nPointIdx) const
{
    const VisualizationSeries* _pSeries = primarySeries();
    if (_pSeries == nullptr
        || _nPointIdx < 0
        || _nPointIdx >= _pSeries->vPoints.size()) {
        return QString();
    }

    const VisualizationPoint& _point = _pSeries->vPoints[_nPointIdx];
    if (!_point.strToolTip.isEmpty()) {
        return _point.strToolTip;
    }

    return tr("%1\n数值：%2")
        .arg(_point.strLabel)
        .arg(_point.dValue, 0, 'f', 6);
}
