// src/ui/style/imagebutton.h
#ifndef IMAGEBUTTON_H_D4E5F6A1B2C3
#define IMAGEBUTTON_H_D4E5F6A1B2C3

#include <QWidget>
#include <QString>
#include <QPixmap>

// ════════════════════════════════════════════════════════
//  ImageButton
//  职责：支持三态图片的自绘按钮控件
//
//  继承 QWidget，支持在 .ui 文件中以 native="true" 方式使用
//
//  图片命名约定：
//    resources/icons/<iconName>_normal.png
//    resources/icons/<iconName>_hover.png
//    resources/icons/<iconName>_pressed.png
//
//  两种使用方式：
//    // 方式A：代码创建，构造时直接传 iconName
//    ImageButton *btn = new ImageButton("nav_pan", parent);
//
//    // 方式B：.ui 创建（native="true"），构造后再绑定
//    ui->btnNavPan->setIconName("nav_pan");
//
//  clicked 信号由 mouseReleaseEvent 内部发出，
//  行为与 QPushButton::clicked 一致
// ════════════════════════════════════════════════════════
class ImageButton : public QWidget
{
    Q_OBJECT
        Q_PROPERTY(bool autoScale READ autoScale WRITE setAutoScale)
        Q_PROPERTY(int  radius    READ radius    WRITE setRadius)

public:
    /*
     * @brief 方式B 构造函数（.ui 路径）
     *        构造后需手动调用 setIconName() 绑定图标
     * @param_1 _pParent: 父对象指针，用于 Qt 对象树内存管理
     */
    explicit ImageButton(QWidget* _pParent = nullptr);

    /*
     * @brief 方式A 构造函数（代码路径）
     *        构造时直接绑定图标名，内部调用 setIconName()
     * @param_1 _strIconName: 图标名称，不含路径与后缀，如 "nav_pan"
     * @param_2 _pParent:     父对象指针，用于 Qt 对象树内存管理
     */
    explicit ImageButton(const QString& _strIconName,
        QWidget* _pParent = nullptr);

    ~ImageButton() override = default;

    // ── 核心接口 ─────────────────────────────────────

    /*
     * @brief 设置图标名并加载三态图片，可在构造后任意时刻调用
     *        相同名称重复调用时不重新加载
     * @param_1 _strIconName: 图标名称，不含路径与后缀，如 "nav_pan"
     */
    void    setIconName(const QString& _strIconName);

    /*
     * @brief 返回当前绑定的图标名称
     */
    QString iconName() const;

    // ── 外观属性 ──────────────────────────────────────

    /*
     * @brief 返回图片是否自动缩放到控件尺寸
     */
    bool autoScale() const;

    /*
     * @brief 设置图片是否自动缩放到控件尺寸
     *        false 时图片居中显示，不缩放
     * @param_1 _bScale: true 为自动缩放，false 为居中原尺寸显示
     */
    void setAutoScale(bool _bScale);

    /*
     * @brief 返回当前圆角半径（像素）
     */
    int  radius() const;

    /*
     * @brief 设置控件绘制时的圆角半径
     *        设为 0 时等价于直角矩形
     * @param_1 _nRadius: 圆角半径，单位像素
     */
    void setRadius(int _nRadius);

    /*
     * @brief 设置控件边框的宽度与颜色
     *        宽度为 0 时不绘制边框
     * @param_1 _nWidth:    边框宽度，单位像素
     * @param_2 _strColor:  边框颜色字符串，如 "#ff0000" 或 "red"
     */
    void setBorder(int _nWidth, const QString& _strColor);

    // ── 尺寸建议 ──────────────────────────────────────

    /*
     * @brief 返回控件的建议尺寸，供布局参考
     */
    QSize sizeHint()        const override;

    /*
     * @brief 返回控件的最小建议尺寸
     */
    QSize minimumSizeHint() const override;

signals:
    /*
     * @brief 鼠标在控件范围内完成左键按下并释放时触发
     *        等价于 QPushButton::clicked 的行为
     */
    void clicked();

protected:
    /*
     * @brief 绘制三态图片与可选边框
     * @param_1 _pEvent: Qt 绘制事件对象
     */
    void paintEvent(QPaintEvent* _pEvent) override;

    /*
     * @brief 鼠标进入控件区域时触发，切换到 hover 状态
     * @param_1 _pEvent: Qt 进入事件对象
     */
    void enterEvent(QEvent* _pEvent) override;

    /*
     * @brief 鼠标离开控件区域时触发，恢复 normal 状态
     * @param_1 _pEvent: Qt 离开事件对象
     */
    void leaveEvent(QEvent* _pEvent) override;

    /*
     * @brief 鼠标左键按下时触发，切换到 pressed 状态
     * @param_1 _pEvent: Qt 鼠标事件对象
     */
    void mousePressEvent(QMouseEvent* _pEvent) override;

    /*
     * @brief 鼠标左键释放时触发
     *        若释放位置在控件范围内则 emit clicked()
     * @param_1 _pEvent: Qt 鼠标事件对象
     */
    void mouseReleaseEvent(QMouseEvent* _pEvent) override;

private:
    /*
     * @brief 根据当前 mstrIconName 拼接路径并加载三态像素图
     *        hover/pressed 缺失时自动降级：pressed → hover → normal
     */
    void loadPixmaps();

    // ── 资源根目录 ────────────────────────────────────
    static const QString S_STR_ICON_DIR;    // 图标资源根目录，默认 "resources/icons/"

    // ── 图标名与像素图缓存 ────────────────────────────
    QString  mstrIconName;   // 当前绑定的图标名称，不含路径与后缀
    QPixmap  mpmNormal;      // normal 状态像素图（鼠标离开时显示）
    QPixmap  mpmHover;       // hover 状态像素图（鼠标悬停时显示）
    QPixmap  mpmPressed;     // pressed 状态像素图（鼠标按下时显示）

    // ── 交互状态 ──────────────────────────────────────
    bool     mbHovered;      // 鼠标当前是否悬停在控件上
    bool     mbPressed;      // 鼠标左键当前是否处于按下状态

    // ── 外观参数 ──────────────────────────────────────
    bool     mbAutoScale;       // 是否将图片缩放到控件尺寸，默认 true
    int      mnRadius;          // 圆角半径（像素），0 表示直角，默认 0
    int      mnBorderWidth;     // 边框宽度（像素），0 表示不绘制，默认 0
    QString  mstrBorderColor;   // 边框颜色字符串，如 "#333333"，默认空
};

#endif // IMAGEBUTTON_H_D4E5F6A1B2C3