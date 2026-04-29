#ifndef ANTBUTTON_H_91A2B3C4D5E6
#define ANTBUTTON_H_91A2B3C4D5E6

#include <QColor>
#include <QPoint>
#include <QPushButton>

class QEvent;
class QMouseEvent;
class QPaintEvent;
class QVariantAnimation;

// ============================================================================
//  AntButton
//  职责：Qt5 兼容的 Ant Design 风格按钮。
//        支持 Primary / Default / Quiet / Danger 四种角色，并提供轻量点击涟漪反馈。
// ============================================================================
class AntButton : public QPushButton
{
public:
    enum ButtonRole
    {
        Primary,
        Default,
        Quiet,
        Danger
    };

    /*
     * @brief 构造 Ant 风格按钮
     * @param_1 _strText: 按钮文本
     * @param_2 _pParent: 父控件指针
     */
    explicit AntButton(const QString& _strText = QString(),
        QWidget* _pParent = nullptr);

    /*
     * @brief 设置按钮视觉角色
     * @param_1 _eRole: 按钮角色
     */
    void setButtonRole(ButtonRole _eRole);

    /*
     * @brief 返回按钮视觉角色
     */
    ButtonRole buttonRole() const;

    /*
     * @brief 返回推荐尺寸
     */
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* _pEvent) override;
    void mousePressEvent(QMouseEvent* _pEvent) override;
    void mouseReleaseEvent(QMouseEvent* _pEvent) override;
    void enterEvent(QEvent* _pEvent) override;
    void leaveEvent(QEvent* _pEvent) override;

private:
    QColor currentBackgroundColor() const;
    QColor currentBorderColor() const;
    QColor currentTextColor() const;
    QColor rippleColor() const;
    void startRipple(const QPoint& _pointCenter);

    ButtonRole meRole;                    // 当前按钮视觉角色
    bool       mbHovered;                 // 鼠标是否悬停
    bool       mbPressed;                 // 鼠标是否按下
    bool       mbRippleActive;            // 涟漪动画是否处于激活状态
    qreal      mdRippleProgress;          // 涟漪动画进度
    QPoint     mpointRippleCenter;        // 涟漪中心点
    QVariantAnimation* mpctrlRippleAnimation; // 涟漪动画对象
    int        mnRadius;                  // 圆角半径
    int        mnHorizontalPadding;       // 水平内边距
};

#endif // ANTBUTTON_H_91A2B3C4D5E6
