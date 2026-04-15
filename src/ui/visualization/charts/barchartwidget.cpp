#include "barchartwidget.h"

#include <QFontMetrics>
#include <QPainter>
#include <QtMath>

BarChartWidget::BarChartWidget(QWidget* _pParent)
    : ChartWidget(_pParent)
{
}

void BarChartWidget::drawChart(QPainter& _painter, const QRectF& _rcPlotArea)
{
    const VisualizationSeries* _pSeries = primarySeries();
    if (_pSeries == nullptr || _pSeries->vPoints.isEmpty()) {
        return;
    }

    const QPair<double, double> _pairDomain = valueDomain();
    drawAxes(_painter, _rcPlotArea, _pairDomain.first, _pairDomain.second);

    const QVector<QRectF> _vrBars = buildBarRects(_rcPlotArea);
    const QColor _cBaseColor = seriesColor();

    _painter.setPen(QPen(QColor("#1F2933"), 1.0));
    for (int _nPointIdx = 0; _nPointIdx < _vrBars.size(); ++_nPointIdx) {
        QColor _cFill = _cBaseColor;
        if (_nPointIdx == selectedPointIndex()) {
            _cFill = _cFill.darker(125);
        } else if (_nPointIdx == hoveredPointIndex()) {
            _cFill = _cFill.lighter(120);
        }

        _painter.fillRect(_vrBars[_nPointIdx], _cFill);
        _painter.drawRect(_vrBars[_nPointIdx]);

        const QRectF _rcTextRect(
            _vrBars[_nPointIdx].left() - 12.0,
            _vrBars[_nPointIdx].top() - 24.0,
            _vrBars[_nPointIdx].width() + 24.0,
            18.0);
        _painter.setPen(QColor("#1F2933"));
        _painter.drawText(_rcTextRect, Qt::AlignCenter,
            formatValueText(_pSeries->vPoints[_nPointIdx].dValue));
    }
}

int BarChartWidget::hitTestPoint(const QPoint& _ptPos) const
{
    const QVector<QRectF> _vrBars = buildBarRects(buildPlotArea());
    for (int _nPointIdx = 0; _nPointIdx < _vrBars.size(); ++_nPointIdx) {
        if (_vrBars[_nPointIdx].contains(_ptPos)) {
            return _nPointIdx;
        }
    }
    return -1;
}

QVector<QRectF> BarChartWidget::buildBarRects(const QRectF& _rcPlotArea) const
{
    QVector<QRectF> _vrBars;
    const VisualizationSeries* _pSeries = primarySeries();
    if (_pSeries == nullptr || _pSeries->vPoints.isEmpty()) {
        return _vrBars;
    }

    const QPair<double, double> _pairDomain = valueDomain();
    const double _dBaselineY = valueToY(0.0, _rcPlotArea, _pairDomain.first, _pairDomain.second);
    const double _dSlotWidth = _rcPlotArea.width() / _pSeries->vPoints.size();
    const double _dBarWidth = qMax(12.0, _dSlotWidth * 0.56);

    for (int _nPointIdx = 0; _nPointIdx < _pSeries->vPoints.size(); ++_nPointIdx) {
        const double _dCenterX = _rcPlotArea.left() + _dSlotWidth * (_nPointIdx + 0.5);
        const double _dValueY = valueToY(
            _pSeries->vPoints[_nPointIdx].dValue,
            _rcPlotArea,
            _pairDomain.first,
            _pairDomain.second);
        const double _dTop = qMin(_dBaselineY, _dValueY);
        const double _dBottom = qMax(_dBaselineY, _dValueY);
        _vrBars.append(QRectF(
            _dCenterX - _dBarWidth / 2.0,
            _dTop,
            _dBarWidth,
            qMax(1.0, _dBottom - _dTop)));
    }

    return _vrBars;
}

QPair<double, double> BarChartWidget::valueDomain() const
{
    const VisualizationSeries* _pSeries = primarySeries();
    if (_pSeries == nullptr || _pSeries->vPoints.isEmpty()) {
        return qMakePair(0.0, 1.0);
    }

    double _dMinValue = _pSeries->vPoints.first().dValue;
    double _dMaxValue = _pSeries->vPoints.first().dValue;
    for (const VisualizationPoint& _point : _pSeries->vPoints) {
        _dMinValue = qMin(_dMinValue, _point.dValue);
        _dMaxValue = qMax(_dMaxValue, _point.dValue);
    }

    double _dDomainMin = qMin(0.0, _dMinValue);
    double _dDomainMax = qMax(0.0, _dMaxValue);
    if (qFuzzyCompare(_dDomainMin + 1.0, _dDomainMax + 1.0)) {
        _dDomainMin -= 1.0;
        _dDomainMax += 1.0;
    }

    const double _dPadding = (_dDomainMax - _dDomainMin) * 0.08;
    return qMakePair(_dDomainMin - _dPadding, _dDomainMax + _dPadding);
}

double BarChartWidget::valueToY(double _dValue, const QRectF& _rcPlotArea,
    double _dDomainMin, double _dDomainMax) const
{
    const double _dRange = qMax(0.000001, _dDomainMax - _dDomainMin);
    return _rcPlotArea.top()
        + (_dDomainMax - _dValue) / _dRange * _rcPlotArea.height();
}

void BarChartWidget::drawAxes(QPainter& _painter, const QRectF& _rcPlotArea,
    double _dDomainMin, double _dDomainMax) const
{
    const VisualizationSeries* _pSeries = primarySeries();
    if (_pSeries == nullptr || _pSeries->vPoints.isEmpty()) {
        return;
    }

    const double _dAxisY = valueToY(0.0, _rcPlotArea, _dDomainMin, _dDomainMax);
    const double _dClampedAxisY = qBound(_rcPlotArea.top(), _dAxisY, _rcPlotArea.bottom());
    _painter.setPen(QPen(QColor("#4B5563"), 1.0));
    _painter.drawLine(QPointF(_rcPlotArea.left(), _rcPlotArea.top()),
        QPointF(_rcPlotArea.left(), _rcPlotArea.bottom()));
    _painter.drawLine(QPointF(_rcPlotArea.left(), _dClampedAxisY),
        QPointF(_rcPlotArea.right(), _dClampedAxisY));

    for (int _nTickIdx = 0; _nTickIdx <= 4; ++_nTickIdx) {
        const double _dValue = _dDomainMin + (_dDomainMax - _dDomainMin) * _nTickIdx / 4.0;
        const double _dTickY = valueToY(_dValue, _rcPlotArea, _dDomainMin, _dDomainMax);
        _painter.setPen(QPen(QColor("#E5E7EB"), 1.0));
        _painter.drawLine(QPointF(_rcPlotArea.left(), _dTickY),
            QPointF(_rcPlotArea.right(), _dTickY));

        _painter.setPen(QColor("#4B5563"));
        const QRectF _rcLabel(
            _rcPlotArea.left() - 68.0,
            _dTickY - 10.0,
            60.0,
            20.0);
        _painter.drawText(_rcLabel, Qt::AlignRight | Qt::AlignVCenter,
            formatValueText(_dValue));
    }

    const QVector<QRectF> _vrBars = buildBarRects(_rcPlotArea);
    const QFontMetrics _fontMetrics(font());
    for (int _nPointIdx = 0; _nPointIdx < _vrBars.size(); ++_nPointIdx) {
        const QRectF _rcLabel(
            _vrBars[_nPointIdx].left() - 20.0,
            _rcPlotArea.bottom() + 8.0,
            _vrBars[_nPointIdx].width() + 40.0,
            36.0);
        const QString _strLabel = _fontMetrics.elidedText(
            _pSeries->vPoints[_nPointIdx].strLabel,
            Qt::ElideRight,
            static_cast<int>(_rcLabel.width()));
        _painter.drawText(_rcLabel, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
            _strLabel);
    }

    _painter.drawText(QRectF(_rcPlotArea.left(), _rcPlotArea.bottom() + 48.0,
            _rcPlotArea.width(), 20.0),
        Qt::AlignCenter,
        visualizationData().strXAxisTitle);

    _painter.save();
    _painter.translate(_rcPlotArea.left() - 54.0, _rcPlotArea.center().y());
    _painter.rotate(-90.0);
    _painter.drawText(QRectF(-_rcPlotArea.height() / 2.0, -12.0,
            _rcPlotArea.height(), 24.0),
        Qt::AlignCenter,
        visualizationData().strYAxisTitle);
    _painter.restore();
}

QColor BarChartWidget::seriesColor() const
{
    const VisualizationSeries* _pSeries = primarySeries();
    const QColor _cSeriesColor((_pSeries != nullptr) ? _pSeries->strColorHex : QString());
    return _cSeriesColor.isValid() ? _cSeriesColor : QColor("#3E7CB1");
}

QString BarChartWidget::formatValueText(double _dValue) const
{
    return (qAbs(_dValue - qRound64(_dValue)) < 0.000001)
        ? QString::number(qRound64(_dValue))
        : QString::number(_dValue, 'f', 2);
}
