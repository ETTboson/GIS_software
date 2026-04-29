#include "antbutton.h"

#include <QAbstractAnimation>
#include <QEasingCurve>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>

#include <cmath>

namespace
{

const QColor S_COLOR_PRIMARY("#1677FF");
const QColor S_COLOR_PRIMARY_HOVER("#4096FF");
const QColor S_COLOR_PRIMARY_PRESSED("#0958D9");
const QColor S_COLOR_DANGER("#FF4D4F");
const QColor S_COLOR_DANGER_HOVER("#FF7875");
const QColor S_COLOR_DANGER_PRESSED("#D9363E");
const QColor S_COLOR_TEXT("#1F2937");
const QColor S_COLOR_MUTED("#64748B");
const QColor S_COLOR_BORDER("#D9D9D9");
const QColor S_COLOR_BORDER_HOVER("#1677FF");
const QColor S_COLOR_BG("#FFFFFF");
const QColor S_COLOR_BG_HOVER("#F0F7FF");
const QColor S_COLOR_BG_PRESSED("#E6F4FF");
const QColor S_COLOR_DISABLED_BG("#F5F7FB");
const QColor S_COLOR_DISABLED_TEXT("#A3AAB8");
const QColor S_COLOR_DISABLED_BORDER("#E5E7EB");

QColor mixColor(const QColor& _colorBase, int _nAlpha)
{
    QColor _colorMixed = _colorBase;
    _colorMixed.setAlpha(_nAlpha);
    return _colorMixed;
}

} // namespace

AntButton::AntButton(const QString& _strText, QWidget* _pParent)
    : QPushButton(_strText, _pParent)
    , meRole(Default)
    , mbHovered(false)
    , mbPressed(false)
    , mbRippleActive(false)
    , mdRippleProgress(0.0)
    , mpointRippleCenter(QPoint())
    , mpctrlRippleAnimation(nullptr)
    , mnRadius(6)
    , mnHorizontalPadding(16)
{
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
    setMinimumHeight(34);
    setFocusPolicy(Qt::StrongFocus);
}

void AntButton::setButtonRole(ButtonRole _eRole)
{
    if (meRole == _eRole) {
        return;
    }
    meRole = _eRole;
    update();
}

AntButton::ButtonRole AntButton::buttonRole() const
{
    return meRole;
}

QSize AntButton::sizeHint() const
{
    const QFontMetrics _metricsFont(font());
    const int _nTextWidth = _metricsFont.horizontalAdvance(text());
    return QSize(_nTextWidth + mnHorizontalPadding * 2, 36);
}

void AntButton::paintEvent(QPaintEvent* _pEvent)
{
    Q_UNUSED(_pEvent)

    QPainter _painter(this);
    _painter.setRenderHint(QPainter::Antialiasing);
    _painter.setRenderHint(QPainter::TextAntialiasing);

    const QRectF _rectButton = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath _pathButton;
    _pathButton.addRoundedRect(_rectButton, mnRadius, mnRadius);

    _painter.fillPath(_pathButton, currentBackgroundColor());
    _painter.setPen(QPen(currentBorderColor(), meRole == Quiet ? 0 : 1));
    _painter.drawPath(_pathButton);

    if (hasFocus() && isEnabled()) {
        QPen _penFocus(mixColor(S_COLOR_PRIMARY, 80), 2);
        _painter.setPen(_penFocus);
        _painter.setBrush(Qt::NoBrush);
        _painter.drawRoundedRect(_rectButton.adjusted(2, 2, -2, -2),
            mnRadius - 1,
            mnRadius - 1);
    }

    if (mbRippleActive && isEnabled()) {
        _painter.save();
        _painter.setClipPath(_pathButton);

        const qreal _dWidth = static_cast<qreal>(width());
        const qreal _dHeight = static_cast<qreal>(height());
        const qreal _dMaxRadius = std::sqrt(_dWidth * _dWidth + _dHeight * _dHeight);
        const qreal _dRadius = _dMaxRadius * mdRippleProgress;
        QColor _colorRipple = rippleColor();
        _colorRipple.setAlphaF(0.20 * (1.0 - mdRippleProgress));

        _painter.setPen(Qt::NoPen);
        _painter.setBrush(_colorRipple);
        _painter.drawEllipse(QPointF(mpointRippleCenter), _dRadius, _dRadius);
        _painter.restore();
    }

    _painter.setPen(currentTextColor());
    _painter.setFont(font());
    _painter.drawText(rect().adjusted(8, 0, -8, 0), Qt::AlignCenter, text());
}

void AntButton::mousePressEvent(QMouseEvent* _pEvent)
{
    if (_pEvent->button() == Qt::LeftButton && isEnabled()) {
        mbPressed = true;
        startRipple(_pEvent->pos());
        update();
    }
    QPushButton::mousePressEvent(_pEvent);
}

void AntButton::mouseReleaseEvent(QMouseEvent* _pEvent)
{
    if (_pEvent->button() == Qt::LeftButton) {
        mbPressed = false;
        update();
    }
    QPushButton::mouseReleaseEvent(_pEvent);
}

void AntButton::enterEvent(QEvent* _pEvent)
{
    mbHovered = true;
    update();
    QPushButton::enterEvent(_pEvent);
}

void AntButton::leaveEvent(QEvent* _pEvent)
{
    mbHovered = false;
    mbPressed = false;
    update();
    QPushButton::leaveEvent(_pEvent);
}

QColor AntButton::currentBackgroundColor() const
{
    if (!isEnabled()) {
        return S_COLOR_DISABLED_BG;
    }

    if (meRole == Primary) {
        if (mbPressed) {
            return S_COLOR_PRIMARY_PRESSED;
        }
        if (mbHovered) {
            return S_COLOR_PRIMARY_HOVER;
        }
        return S_COLOR_PRIMARY;
    }

    if (meRole == Danger) {
        if (mbPressed) {
            return S_COLOR_DANGER_PRESSED;
        }
        if (mbHovered) {
            return S_COLOR_DANGER_HOVER;
        }
        return S_COLOR_DANGER;
    }

    if (meRole == Quiet) {
        if (mbPressed) {
            return S_COLOR_BG_PRESSED;
        }
        if (mbHovered) {
            return S_COLOR_BG_HOVER;
        }
        return QColor(255, 255, 255, 0);
    }

    if (mbPressed) {
        return S_COLOR_BG_PRESSED;
    }
    if (mbHovered) {
        return S_COLOR_BG_HOVER;
    }
    return S_COLOR_BG;
}

QColor AntButton::currentBorderColor() const
{
    if (!isEnabled()) {
        return S_COLOR_DISABLED_BORDER;
    }

    if (meRole == Primary) {
        return mbPressed ? S_COLOR_PRIMARY_PRESSED : S_COLOR_PRIMARY;
    }

    if (meRole == Danger) {
        return mbPressed ? S_COLOR_DANGER_PRESSED : S_COLOR_DANGER;
    }

    if (meRole == Quiet) {
        return QColor(255, 255, 255, 0);
    }

    if (mbPressed) {
        return S_COLOR_PRIMARY_PRESSED;
    }
    if (mbHovered) {
        return S_COLOR_BORDER_HOVER;
    }
    return S_COLOR_BORDER;
}

QColor AntButton::currentTextColor() const
{
    if (!isEnabled()) {
        return S_COLOR_DISABLED_TEXT;
    }

    if (meRole == Primary || meRole == Danger) {
        return QColor("#FFFFFF");
    }

    if (meRole == Quiet) {
        if (mbPressed) {
            return S_COLOR_PRIMARY_PRESSED;
        }
        if (mbHovered) {
            return S_COLOR_PRIMARY;
        }
        return S_COLOR_MUTED;
    }

    if (mbPressed) {
        return S_COLOR_PRIMARY_PRESSED;
    }
    if (mbHovered) {
        return S_COLOR_PRIMARY;
    }
    return S_COLOR_TEXT;
}

QColor AntButton::rippleColor() const
{
    if (meRole == Primary || meRole == Danger) {
        return QColor("#FFFFFF");
    }
    return S_COLOR_PRIMARY;
}

void AntButton::startRipple(const QPoint& _pointCenter)
{
    if (mpctrlRippleAnimation != nullptr) {
        mpctrlRippleAnimation->stop();
        mpctrlRippleAnimation->deleteLater();
        mpctrlRippleAnimation = nullptr;
    }

    mpointRippleCenter = _pointCenter;
    mdRippleProgress = 0.0;
    mbRippleActive = true;

    QVariantAnimation* _pctrlAnimation = new QVariantAnimation(this);
    mpctrlRippleAnimation = _pctrlAnimation;
    _pctrlAnimation->setStartValue(0.0);
    _pctrlAnimation->setEndValue(1.0);
    _pctrlAnimation->setDuration(420);
    _pctrlAnimation->setEasingCurve(QEasingCurve::OutCubic);

    connect(_pctrlAnimation, &QVariantAnimation::valueChanged,
        this, [this](const QVariant& _valueCurrent) {
            mdRippleProgress = _valueCurrent.toReal();
            update();
        });

    connect(_pctrlAnimation, &QVariantAnimation::finished,
        this, [this, _pctrlAnimation]() {
            if (mpctrlRippleAnimation == _pctrlAnimation) {
                mpctrlRippleAnimation = nullptr;
                mbRippleActive = false;
                mdRippleProgress = 1.0;
                update();
            }
        });

    _pctrlAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}
