#include "linechartwidget.h"

#include <QFontMetrics>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

LineChartWidget::LineChartWidget(QWidget* _pParent)
    : ChartWidget(_pParent)
{
}

void LineChartWidget::drawChart(QPainter& _painter, const QRectF& _rcPlotArea)
{
    const VisualizationSeries* _pSeries = primarySeries();
    if (_pSeries == nullptr || _pSeries->vPoints.isEmpty()) {
        return;
    }

    const QPair<double, double> _pairDomain = valueDomain();
    drawAxes(_painter, _rcPlotArea, _pairDomain.first, _pairDomain.second);

    const QVector<QPointF> _vPointPositions = buildPointPositions(_rcPlotArea);
    const QColor _cBaseColor = seriesColor();

    QPainterPath _pathLine;
    _pathLine.moveTo(_vPointPositions.first());
    for (int _nPointIdx = 1; _nPointIdx < _vPointPositions.size(); ++_nPointIdx) {
        _pathLine.lineTo(_vPointPositions[_nPointIdx]);
    }

    _painter.setPen(QPen(_cBaseColor, 2.5));
    _painter.drawPath(_pathLine);

    for (int _nPointIdx = 0; _nPointIdx < _vPointPositions.size(); ++_nPointIdx) {
        QColor _cFill = _cBaseColor;
        int _nRadius = 5;
        if (_nPointIdx == selectedPointIndex()) {
            _cFill = _cFill.darker(125);
            _nRadius = 8;
        } else if (_nPointIdx == hoveredPointIndex()) {
            _cFill = _cFill.lighter(120);
            _nRadius = 7;
        }

        _painter.setPen(QPen(QColor("#FFFFFF"), 1.5));
        _painter.setBrush(_cFill);
        _painter.drawEllipse(_vPointPositions[_nPointIdx], _nRadius, _nRadius);

        const QRectF _rcText(
            _vPointPositions[_nPointIdx].x() - 28.0,
            _vPointPositions[_nPointIdx].y() - 30.0,
            56.0,
            18.0);
        _painter.setPen(QColor("#1F2933"));
        _painter.drawText(_rcText, Qt::AlignCenter,
            formatValueText(_pSeries->vPoints[_nPointIdx].dValue));
    }
}

int LineChartWidget::hitTestPoint(const QPoint& _ptPos) const
{
    const QVector<QPointF> _vPointPositions = buildPointPositions(buildPlotArea());
    for (int _nPointIdx = 0; _nPointIdx < _vPointPositions.size(); ++_nPointIdx) {
        if (QLineF(_vPointPositions[_nPointIdx], _ptPos).length() <= 10.0) {
            return _nPointIdx;
        }
    }
    return -1;
}

QVector<QPointF> LineChartWidget::buildPointPositions(const QRectF& _rcPlotArea) const
{
    QVector<QPointF> _vPointPositions;
    const VisualizationSeries* _pSeries = primarySeries();
    if (_pSeries == nullptr || _pSeries->vPoints.isEmpty()) {
        return _vPointPositions;
    }

    const QPair<double, double> _pairDomain = valueDomain();
    const int _nPointCount = _pSeries->vPoints.size();
    const double _dStepX = (_nPointCount > 1)
        ? (_rcPlotArea.width() / (_nPointCount - 1))
        : 0.0;

    for (int _nPointIdx = 0; _nPointIdx < _nPointCount; ++_nPointIdx) {
        const double _dPosX = (_nPointCount == 1)
            ? _rcPlotArea.center().x()
            : (_rcPlotArea.left() + _dStepX * _nPointIdx);
        const double _dPosY = valueToY(
            _pSeries->vPoints[_nPointIdx].dValue,
            _rcPlotArea,
            _pairDomain.first,
            _pairDomain.second);
        _vPointPositions.append(QPointF(_dPosX, _dPosY));
    }

    return _vPointPositions;
}

QPair<double, double> LineChartWidget::valueDomain() const
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

    if (qFuzzyCompare(_dMinValue + 1.0, _dMaxValue + 1.0)) {
        _dMinValue -= 1.0;
        _dMaxValue += 1.0;
    }

    const double _dPadding = (_dMaxValue - _dMinValue) * 0.1;
    return qMakePair(_dMinValue - _dPadding, _dMaxValue + _dPadding);
}

double LineChartWidget::valueToY(double _dValue, const QRectF& _rcPlotArea,
    double _dDomainMin, double _dDomainMax) const
{
    const double _dRange = qMax(0.000001, _dDomainMax - _dDomainMin);
    return _rcPlotArea.top()
        + (_dDomainMax - _dValue) / _dRange * _rcPlotArea.height();
}

void LineChartWidget::drawAxes(QPainter& _painter, const QRectF& _rcPlotArea,
    double _dDomainMin, double _dDomainMax) const
{
    const VisualizationSeries* _pSeries = primarySeries();
    if (_pSeries == nullptr || _pSeries->vPoints.isEmpty()) {
        return;
    }

    _painter.setPen(QPen(QColor("#4B5563"), 1.0));
    _painter.drawLine(QPointF(_rcPlotArea.left(), _rcPlotArea.top()),
        QPointF(_rcPlotArea.left(), _rcPlotArea.bottom()));
    _painter.drawLine(QPointF(_rcPlotArea.left(), _rcPlotArea.bottom()),
        QPointF(_rcPlotArea.right(), _rcPlotArea.bottom()));

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

    const QVector<QPointF> _vPointPositions = buildPointPositions(_rcPlotArea);
    const QFontMetrics _fontMetrics(font());
    for (int _nPointIdx = 0; _nPointIdx < _vPointPositions.size(); ++_nPointIdx) {
        const QRectF _rcLabel(
            _vPointPositions[_nPointIdx].x() - 32.0,
            _rcPlotArea.bottom() + 8.0,
            64.0,
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

QColor LineChartWidget::seriesColor() const
{
    const VisualizationSeries* _pSeries = primarySeries();
    const QColor _cSeriesColor((_pSeries != nullptr) ? _pSeries->strColorHex : QString());
    return _cSeriesColor.isValid() ? _cSeriesColor : QColor("#E76F51");
}

QString LineChartWidget::formatValueText(double _dValue) const
{
    return (qAbs(_dValue - qRound64(_dValue)) < 0.000001)
        ? QString::number(qRound64(_dValue))
        : QString::number(_dValue, 'f', 2);
}
