#include "imagebutton.h"

#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include <QMouseEvent>
#include <QDebug>

// ════════════════════════════════════════════════════════
//  静态常量：图标资源根目录
//  若使用 Qt 资源系统（.qrc），改为 ":/icons/"
// ════════════════════════════════════════════════════════
const QString ImageButton::S_STR_ICON_DIR = "resources/icons/";

// ════════════════════════════════════════════════════════
//  构造
// ════════════════════════════════════════════════════════

// 方式B：.ui 路径，无参构造，图片留空等待 setIconName()
ImageButton::ImageButton(QWidget *_pParent)
    : QWidget(_pParent)
    , mstrIconName(QString())
    , mbHovered(false)
    , mbPressed(false)
    , mbAutoScale(true)
    , mnRadius(0)
    , mnBorderWidth(0)
    , mstrBorderColor(QString())
{
    setAttribute(Qt::WA_Hover);
    setCursor(Qt::PointingHandCursor);
}

// 方式A：代码路径，构造时直接绑定 iconName
ImageButton::ImageButton(const QString &_strIconName, QWidget *_pParent)
    : QWidget(_pParent)
    , mstrIconName(QString())   // 先置空，由 setIconName() 填充
    , mbHovered(false)
    , mbPressed(false)
    , mbAutoScale(true)
    , mnRadius(0)
    , mnBorderWidth(0)
    , mstrBorderColor(QString())
{
    setAttribute(Qt::WA_Hover);
    setCursor(Qt::PointingHandCursor);
    setIconName(_strIconName);  // 统一走 setIconName，不重复实现
}

// ════════════════════════════════════════════════════════
//  setIconName — 核心绑定接口
//  清空旧缓存 → 重新加载 → 触发重绘
//  可在构造后任意时刻调用（包括 .ui 路径的延迟绑定）
// ════════════════════════════════════════════════════════
void ImageButton::setIconName(const QString &_strIconName)
{
    if (_strIconName == mstrIconName) {
        return;   // 相同名称不重复加载
    }
    mstrIconName = _strIconName;

    // 清空旧缓存
    mpmNormal  = QPixmap();
    mpmHover   = QPixmap();
    mpmPressed = QPixmap();

    loadPixmaps();
    update();
}

QString ImageButton::iconName() const
{
    return mstrIconName;
}

// ════════════════════════════════════════════════════════
//  loadPixmaps — 拼路径并加载三态图，缺失时降级处理
// ════════════════════════════════════════════════════════
void ImageButton::loadPixmaps()
{
    if (mstrIconName.isEmpty()) {
        // iconName 未设置时保持全空，paintEvent 显示空白
        return;
    }

    const QString _strNormalPath  = S_STR_ICON_DIR + mstrIconName + "_normal.png";
    const QString _strHoverPath   = S_STR_ICON_DIR + mstrIconName + "_hover.png";
    const QString _strPressedPath = S_STR_ICON_DIR + mstrIconName + "_pressed.png";

    // normal：必须存在，缺失时打印警告，保持空白（与参考案例一致）
    if (QFile::exists(_strNormalPath)) {
        mpmNormal.load(_strNormalPath);
    } else {
        qWarning() << "[ImageButton] 找不到 normal 图片:" << _strNormalPath;
        // mpmNormal 保持 null，paintEvent 显示空白
    }

    // hover：缺失时降级使用 normal
    if (QFile::exists(_strHoverPath)) {
        mpmHover.load(_strHoverPath);
    } else {
        mpmHover = mpmNormal;
    }

    // pressed：缺失时降级使用 hover
    if (QFile::exists(_strPressedPath)) {
        mpmPressed.load(_strPressedPath);
    } else {
        mpmPressed = mpmHover;
    }
}

// ════════════════════════════════════════════════════════
//  外观属性
// ════════════════════════════════════════════════════════
bool ImageButton::autoScale() const  { return mbAutoScale; }
void ImageButton::setAutoScale(bool _bScale) { mbAutoScale = _bScale; update(); }

int  ImageButton::radius()   const   { return mnRadius; }
void ImageButton::setRadius(int _nRadius) { mnRadius = _nRadius; update(); }

void ImageButton::setBorder(int _nWidth, const QString &_strColor)
{
    mnBorderWidth   = _nWidth;
    mstrBorderColor = _strColor;
    update();
}

QSize ImageButton::sizeHint()        const { return QSize(96, 96); }
QSize ImageButton::minimumSizeHint() const { return QSize(16, 16); }

// ════════════════════════════════════════════════════════
//  paintEvent — 三态绘制
//  iconName 未设置或图片缺失时显示空白（不绘制任何内容）
// ════════════════════════════════════════════════════════
void ImageButton::paintEvent(QPaintEvent *_pEvent)
{
    Q_UNUSED(_pEvent)

    QPainter _painter(this);
    _painter.setRenderHint(QPainter::Antialiasing);
    _painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 圆角裁剪路径（radius == 0 时等价于无裁剪）
    if (mnRadius > 0) {
        QPainterPath _clipPath;
        _clipPath.addRoundedRect(rect(), mnRadius, mnRadius);
        _painter.setClipPath(_clipPath);
    }

    // ── 选择当前帧 ────────────────────────────────────
    const QPixmap *_ppmCurrent = nullptr;

    if (!isEnabled()) {
        // disabled：使用 normal 图半透明
        _painter.setOpacity(0.4);
        _ppmCurrent = &mpmNormal;
    } else if (mbPressed && !mpmPressed.isNull()) {
        _ppmCurrent = &mpmPressed;
    } else if (mbHovered && !mpmHover.isNull()) {
        _ppmCurrent = &mpmHover;
    } else if (!mpmNormal.isNull()) {
        _ppmCurrent = &mpmNormal;
    }
    // else：_ppmCurrent 为 nullptr ，paintEvent 显示空白

    // ── 绘制图片 ──────────────────────────────────────
    if (_ppmCurrent != nullptr && !_ppmCurrent->isNull()) {
        if (mbAutoScale) {
            _painter.drawPixmap(rect(), *_ppmCurrent);
        } else {
            // 不缩放时居中显示
            int _nX = (width()  - _ppmCurrent->width())  / 2;
            int _nY = (height() - _ppmCurrent->height()) / 2;
            _painter.drawPixmap(_nX, _nY, *_ppmCurrent);
        }
    }

    _painter.setOpacity(1.0);

    // ── 可选边框 ──────────────────────────────────────
    if (mnBorderWidth > 0) {
        QPen _pen;
        _pen.setWidth(mnBorderWidth);
        _pen.setColor(QColor::isValidColor(mstrBorderColor)
                      ? QColor(mstrBorderColor)
                      : Qt::black);
        _painter.setPen(_pen);
        _painter.setBrush(Qt::NoBrush);

        QRectF _borderRect = QRectF(rect()).adjusted(
            mnBorderWidth * 0.5,  mnBorderWidth * 0.5,
           -mnBorderWidth * 0.5, -mnBorderWidth * 0.5);
        _painter.drawRoundedRect(_borderRect, mnRadius, mnRadius);
    }
}

// ════════════════════════════════════════════════════════
//  事件处理
// ════════════════════════════════════════════════════════
void ImageButton::enterEvent(QEvent *_pEvent)
{
    Q_UNUSED(_pEvent)
    mbHovered = true;
    update();
}

void ImageButton::leaveEvent(QEvent *_pEvent)
{
    Q_UNUSED(_pEvent)
    mbHovered = false;
    mbPressed = false;
    update();
}

void ImageButton::mousePressEvent(QMouseEvent *_pEvent)
{
    if (_pEvent->button() == Qt::LeftButton) {
        mbPressed = true;
        update();
    }
    QWidget::mousePressEvent(_pEvent);
}

void ImageButton::mouseReleaseEvent(QMouseEvent *_pEvent)
{
    if (_pEvent->button() == Qt::LeftButton && mbPressed) {
        mbPressed = false;
        update();
        // 只有在控件范围内释放才发出 clicked
        if (rect().contains(_pEvent->pos())) {
            emit clicked();
        }
    }
    QWidget::mouseReleaseEvent(_pEvent);
}
