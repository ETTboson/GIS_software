// src/ui/docks/aidockwidget.h
#ifndef AIDOCKWIDGET_H_F6A1B2C3D4E5
#define AIDOCKWIDGET_H_F6A1B2C3D4E5

#include <QDockWidget>
#include <QColor>
#include <QStringList>

class QTextEdit;
class QLineEdit;
class QLabel;
class QPushButton;
class QEvent;
class QKeyEvent;
class AIManager;

class AIDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数，初始化 Claude Code CLI 风格终端界面
     * @param_1 _pParent: 父对象指针
     */
    explicit AIDockWidget(QWidget* _pParent = nullptr);

    ~AIDockWidget() override = default;

    /*
     * @brief 注入 AIManager 并连接信号
     * @param_1 _pAIManager: AI 对话管理器指针
     */
    void setAIManager(AIManager* _pAIManager);

    /*
     * @brief 聚焦输入框并确保面板可见
     */
    void focusInput();

protected:
    /*
     * @brief 监听输入框快捷键
     * @param_1 _pObj: 事件目标对象
     * @param_2 _pEvent: 事件对象
     */
    bool eventFilter(QObject* _pObj, QEvent* _pEvent) override;

private slots:
    void onSendClicked();
    void onChunkReceived(const QString& _strChunk);
    void onReplyFinished();
    void onErrorOccurred(const QString& _strError);
    void onToolCallStarted(const QString& _strToolName);
    void onToolCallFinished(const QString& _strToolName,
        const QString& _strResult);
    void onClearClicked();
    void onStopClicked();

private:
    /*
     * @brief 初始化终端风格界面
     */
    void initUI();

    /*
     * @brief 连接内部控件信号
     */
    void initConnections();

    /*
     * @brief 打印终端 banner
     */
    void printBanner();

    /*
     * @brief 追加一行带样式的输出
     * @param_1 _strText: 行文本
     * @param_2 _color: 文本颜色
     * @param_3 _bBold: 是否粗体
     * @param_4 _strPrefix: 行前缀
     */
    void appendLine(const QString& _strText,
        const QColor& _color,
        bool _bBold = false,
        const QString& _strPrefix = QString());

    /*
     * @brief 开启新的 AI 输出块
     */
    void beginAiBlock();

    /*
     * @brief 向当前 AI 输出块追加 chunk
     * @param_1 _strChunk: 流式文本片段
     */
    void appendChunk(const QString& _strChunk);

    /*
     * @brief 结束 AI 输出块
     */
    void endAiBlock();

    /*
     * @brief 滚动到输出区底部
     */
    void scrollToBottom();

    /*
     * @brief 历史输入向上浏览
     */
    void historyUp();

    /*
     * @brief 历史输入向下浏览
     */
    void historyDown();

    /*
     * @brief 记录一条输入历史
     * @param_1 _strCmd: 用户输入文本
     */
    void pushHistory(const QString& _strCmd);

    /*
     * @brief 更新顶部状态栏文本与颜色
     * @param_1 _strText: 状态文本
     * @param_2 _color: 状态颜色
     */
    void setStatus(const QString& _strText, const QColor& _color);

    AIManager*    mpAIManager;
    QLabel*       mpctrlStatusBar;
    QTextEdit*    mpctrlOutput;
    QLineEdit*    mpctrlInput;
    QPushButton*  mpctrlBtnSend;
    QPushButton*  mpctrlBtnClear;
    QPushButton*  mpctrlBtnStop;
    bool          mbStreaming;
    bool          mbAiBlockOpen;
    QStringList   mvHistory;
    int           mnHistoryIdx;
    QString       mstrInputBackup;

    static const QColor S_C_BG;
    static const QColor S_C_USER;
    static const QColor S_C_AI;
    static const QColor S_C_TOOL;
    static const QColor S_C_ERROR;
    static const QColor S_C_SYSTEM;
    static const QColor S_C_DIM;
    static const QColor S_C_ACCENT;
};

#endif // AIDOCKWIDGET_H_F6A1B2C3D4E5
