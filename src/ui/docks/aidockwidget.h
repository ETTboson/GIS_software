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
class AIManager;

// ════════════════════════════════════════════════════════
//  AIDockWidget
//  职责：提供终端式 AI 交互面板。
//        负责输入输出、流式文本渲染、工具生命周期显示，以及
//        对内部 tool_call 残片做清洗，保持参考程序的纯文本终端体验。
//  位于 ui/docks/ 层，只与 AIManager 通过信号槽通信。
// ════════════════════════════════════════════════════════
class AIDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数，初始化终端式 AI 面板
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
    enum class PromptKind
    {
        Normal,
        Choice,
        Form
    };

    void initUI();
    void initConnections();
    void printBanner();
    void appendLine(const QString& _strText,
        const QColor& _color,
        bool _bBold = false,
        const QString& _strPrefix = QString());
    void beginAiBlock();
    void appendChunk(const QString& _strChunk);
    void renderCurrentAiBlock();
    void replaceCurrentAiBlock(const QString& _strPlainText,
        PromptKind _kind);
    QString normalizeRawAssistantText(const QString& _strRawText) const;
    PromptKind detectPromptKind(const QString& _strPlainText) const;
    void endAiBlock();
    void scrollToBottom();
    void historyUp();
    void historyDown();
    void pushHistory(const QString& _strCmd);
    void setStatus(const QString& _strText, const QColor& _color);

    AIManager*   mpAIManager; // AI 对话管理器
    QLabel*      mpctrlStatusBar; // 顶部状态栏
    QTextEdit*   mpctrlOutput; // 输出区
    QLineEdit*   mpctrlInput; // 输入框
    QPushButton* mpctrlBtnSend; // 发送按钮
    QPushButton* mpctrlBtnClear; // 清屏按钮
    QPushButton* mpctrlBtnStop; // 中断按钮
    bool         mbStreaming; // 是否正在流式回复
    bool         mbAiBlockOpen; // 是否已有未结束 AI 块
    QString      mstrRawAssistantBuffer; // 原始助手缓冲
    int          mnAiContentStart; // AI 块起始位置
    int          mnAiContentEnd; // AI 块结束位置
    QStringList  mvHistory; // 历史输入
    int          mnHistoryIdx; // 当前历史索引
    QString      mstrInputBackup; // 历史导航时的输入备份

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
