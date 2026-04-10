#include "aidockwidget.h"

#include "core/ai/aimanager.h"

#include <QDateTime>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSet>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextEdit>
#include <QVBoxLayout>

namespace
{

bool isInternalPlanningTool(const QString& _strToolName)
{
    static const QSet<QString> S_SET_INTERNAL_TOOLS = {
        "set_analysis_params",
        "cancel_analysis",
        "get_analysis_context",
        "search_memory"
    };
    return S_SET_INTERNAL_TOOLS.contains(_strToolName);
}

bool isRawToolCallJsonLine(const QString& _strLine)
{
    const QString _strTrimmed = _strLine.trimmed();
    return _strTrimmed.startsWith('{')
        && _strTrimmed.endsWith('}')
        && _strTrimmed.contains("\"name\"")
        && _strTrimmed.contains("\"arguments\"");
}

} // namespace

const QColor AIDockWidget::S_C_BG("#1E1E1E");
const QColor AIDockWidget::S_C_USER("#4EC9B0");
const QColor AIDockWidget::S_C_AI("#D4D4D4");
const QColor AIDockWidget::S_C_TOOL("#DCDCAA");
const QColor AIDockWidget::S_C_ERROR("#F44747");
const QColor AIDockWidget::S_C_SYSTEM("#9CDCFE");
const QColor AIDockWidget::S_C_DIM("#555555");
const QColor AIDockWidget::S_C_ACCENT("#C586C0");

AIDockWidget::AIDockWidget(QWidget* _pParent)
    : QDockWidget(tr("AI 终端"), _pParent)
    , mpAIManager(nullptr)
    , mpctrlStatusBar(nullptr)
    , mpctrlOutput(nullptr)
    , mpctrlInput(nullptr)
    , mpctrlBtnSend(nullptr)
    , mpctrlBtnClear(nullptr)
    , mpctrlBtnStop(nullptr)
    , mbStreaming(false)
    , mbAiBlockOpen(false)
    , mnAiContentStart(-1)
    , mnAiContentEnd(-1)
    , mnHistoryIdx(-1)
{
    setObjectName("dockAI");
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setMinimumWidth(420);

    initUI();
    initConnections();
    printBanner();
}

void AIDockWidget::setAIManager(AIManager* _pAIManager)
{
    if (mpAIManager != nullptr) {
        disconnect(mpAIManager, nullptr, this, nullptr);
    }

    mpAIManager = _pAIManager;
    if (mpAIManager == nullptr) {
        return;
    }

    connect(mpAIManager, &AIManager::chunkReceived,
        this, &AIDockWidget::onChunkReceived);
    connect(mpAIManager, &AIManager::replyFinished,
        this, &AIDockWidget::onReplyFinished);
    connect(mpAIManager, &AIManager::errorOccurred,
        this, &AIDockWidget::onErrorOccurred);
    connect(mpAIManager, &AIManager::toolCallStarted,
        this, &AIDockWidget::onToolCallStarted);
    connect(mpAIManager, &AIManager::toolCallFinished,
        this, &AIDockWidget::onToolCallFinished);
}

void AIDockWidget::focusInput()
{
    setVisible(true);
    raise();
    mpctrlInput->setFocus();
}

bool AIDockWidget::eventFilter(QObject* _pObj, QEvent* _pEvent)
{
    if (_pObj == mpctrlInput && _pEvent->type() == QEvent::KeyPress) {
        QKeyEvent* _pKeyEvent = static_cast<QKeyEvent*>(_pEvent);
        if (_pKeyEvent->key() == Qt::Key_Up) {
            historyUp();
            return true;
        }
        if (_pKeyEvent->key() == Qt::Key_Down) {
            historyDown();
            return true;
        }
        if (_pKeyEvent->key() == Qt::Key_L
            && _pKeyEvent->modifiers() == Qt::ControlModifier) {
            onClearClicked();
            return true;
        }
        if (_pKeyEvent->key() == Qt::Key_C
            && _pKeyEvent->modifiers() == Qt::ControlModifier) {
            onStopClicked();
            return true;
        }
    }

    return QDockWidget::eventFilter(_pObj, _pEvent);
}

void AIDockWidget::initUI()
{
    QWidget* _pctrlContainer = new QWidget(this);
    _pctrlContainer->setStyleSheet(QString("background:%1;").arg(S_C_BG.name()));

    QVBoxLayout* _pRootLayout = new QVBoxLayout(_pctrlContainer);
    _pRootLayout->setContentsMargins(0, 0, 0, 0);
    _pRootLayout->setSpacing(0);

    mpctrlStatusBar = new QLabel(_pctrlContainer);
    mpctrlStatusBar->setFixedHeight(28);

    mpctrlOutput = new QTextEdit(_pctrlContainer);
    mpctrlOutput->setReadOnly(true);
    mpctrlOutput->setFrameShape(QFrame::NoFrame);
    mpctrlOutput->setStyleSheet(
        "QTextEdit{background:#1E1E1E;color:#D4D4D4;selection-background-color:#264F78;padding:8px 14px;}"
        "QScrollBar:vertical{background:#252526;width:8px;border:none;}"
        "QScrollBar::handle:vertical{background:#424242;border-radius:4px;min-height:20px;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}");

    QFont _monoFont("Consolas", 10);
    _monoFont.setStyleHint(QFont::Monospace);
    _monoFont.setFixedPitch(true);
    mpctrlOutput->setFont(_monoFont);
    mpctrlOutput->document()->setDefaultFont(_monoFont);
    mpctrlOutput->setLineWrapMode(QTextEdit::WidgetWidth);

    QWidget* _pctrlInputBar = new QWidget(_pctrlContainer);
    _pctrlInputBar->setFixedHeight(44);
    _pctrlInputBar->setStyleSheet("background:#252526;border-top:1px solid #333;");
    QHBoxLayout* _pInputLayout = new QHBoxLayout(_pctrlInputBar);
    _pInputLayout->setContentsMargins(10, 6, 10, 6);
    _pInputLayout->setSpacing(6);

    QLabel* _pctrlPrompt = new QLabel(">", _pctrlInputBar);
    _pctrlPrompt->setFixedWidth(20);
    _pctrlPrompt->setStyleSheet(
        "color:#4EC9B0;font-size:20px;font-family:Consolas,Courier,monospace;font-weight:bold;");

    mpctrlInput = new QLineEdit(_pctrlInputBar);
    mpctrlInput->setPlaceholderText(tr("输入消息，Enter 发送 | Ctrl+C 中断 | Ctrl+L 清屏"));
    mpctrlInput->setFrame(false);
    mpctrlInput->setStyleSheet(
        "QLineEdit{background:transparent;color:#D4D4D4;font-size:20px;"
        "font-family:Consolas,Courier,monospace;selection-background-color:#264F78;}");
    mpctrlInput->installEventFilter(this);

    mpctrlBtnStop = new QPushButton("[]", _pctrlInputBar);
    mpctrlBtnStop->setFixedSize(34, 30);
    mpctrlBtnStop->setToolTip(tr("中断当前回复"));
    mpctrlBtnStop->setStyleSheet(
        "QPushButton{background:#333;color:#858585;border:none;border-radius:4px;font-size:16px;}"
        "QPushButton:hover{background:#444;color:#D4D4D4;}");

    mpctrlBtnClear = new QPushButton(tr("清屏"), _pctrlInputBar);
    mpctrlBtnClear->setFixedSize(52, 30);
    mpctrlBtnClear->setStyleSheet(
        "QPushButton{background:#333;color:#858585;border:none;border-radius:4px;font-size:14px;}"
        "QPushButton:hover{background:#444;color:#D4D4D4;}");

    mpctrlBtnSend = new QPushButton(tr("发送"), _pctrlInputBar);
    mpctrlBtnSend->setFixedSize(56, 30);
    mpctrlBtnSend->setStyleSheet(
        "QPushButton{background:#0E639C;color:white;border:none;border-radius:4px;font-size:14px;}"
        "QPushButton:hover{background:#1177BB;}"
        "QPushButton:disabled{background:#333;color:#555;}");

    _pInputLayout->addWidget(_pctrlPrompt);
    _pInputLayout->addWidget(mpctrlInput, 1);
    _pInputLayout->addWidget(mpctrlBtnStop);
    _pInputLayout->addWidget(mpctrlBtnClear);
    _pInputLayout->addWidget(mpctrlBtnSend);

    _pRootLayout->addWidget(mpctrlStatusBar);
    _pRootLayout->addWidget(mpctrlOutput, 1);
    _pRootLayout->addWidget(_pctrlInputBar);

    setWidget(_pctrlContainer);
    setStatus(tr("● Ollama  qwen2.5:7b  ·  就绪"), S_C_USER);
    mpctrlBtnStop->setEnabled(false);
}

void AIDockWidget::initConnections()
{
    connect(mpctrlBtnSend, &QPushButton::clicked,
        this, &AIDockWidget::onSendClicked);
    connect(mpctrlInput, &QLineEdit::returnPressed,
        this, &AIDockWidget::onSendClicked);
    connect(mpctrlBtnClear, &QPushButton::clicked,
        this, &AIDockWidget::onClearClicked);
    connect(mpctrlBtnStop, &QPushButton::clicked,
        this, &AIDockWidget::onStopClicked);
}

void AIDockWidget::printBanner()
{
    appendLine(QString(58, '-'), S_C_DIM);
    appendLine("  GIS AI TERMINAL", S_C_ACCENT, true);
    appendLine(QString("  模型: qwen2.5:7b    后端: Ollama    %1")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm")),
        S_C_SYSTEM);
    appendLine("  支持多轮上下文、工具调用、ET.md 记忆注入。", S_C_DIM);
    appendLine(QString(58, '-'), S_C_DIM);
    QTextCursor _cursor = mpctrlOutput->textCursor();
    _cursor.movePosition(QTextCursor::End);
    _cursor.insertBlock();
    mpctrlOutput->setTextCursor(_cursor);
}

void AIDockWidget::appendLine(const QString& _strText,
    const QColor& _color,
    bool _bBold,
    const QString& _strPrefix)
{
    QTextCursor _cursor = mpctrlOutput->textCursor();
    _cursor.movePosition(QTextCursor::End);

    QTextBlockFormat _blkFmt;
    _blkFmt.setBackground(S_C_BG);
    _blkFmt.setTopMargin(1);
    _blkFmt.setBottomMargin(1);
    _cursor.insertBlock(_blkFmt);

    QTextCharFormat _charFmt;
    _charFmt.setForeground(_color);
    _charFmt.setFontWeight(_bBold ? QFont::Bold : QFont::Normal);
    QFont _monoFont("Consolas", 10);
    _monoFont.setFixedPitch(true);
    _charFmt.setFont(_monoFont);

    if (!_strPrefix.isEmpty()) {
        QTextCharFormat _prefixFmt = _charFmt;
        _prefixFmt.setForeground(S_C_DIM);
        _cursor.insertText(_strPrefix, _prefixFmt);
    }

    _cursor.insertText(_strText, _charFmt);
    mpctrlOutput->setTextCursor(_cursor);
    scrollToBottom();
}

void AIDockWidget::beginAiBlock()
{
    if (mbAiBlockOpen) {
        endAiBlock();
    }

    QTextCursor _cursor = mpctrlOutput->textCursor();
    _cursor.movePosition(QTextCursor::End);

    QTextBlockFormat _blkFmt;
    _blkFmt.setBackground(S_C_BG);
    _blkFmt.setTopMargin(4);
    _cursor.insertBlock(_blkFmt);

    QFont _monoFont("Consolas", 10);
    _monoFont.setFixedPitch(true);

    QTextCharFormat _bodyFmt;
    _bodyFmt.setForeground(S_C_AI);
    _bodyFmt.setFont(_monoFont);
    _cursor.setCharFormat(_bodyFmt);

    mstrRawAssistantBuffer.clear();
    mnAiContentStart = _cursor.position();
    mnAiContentEnd = mnAiContentStart;
    mpctrlOutput->setTextCursor(_cursor);
    mbAiBlockOpen = true;
    scrollToBottom();
}

void AIDockWidget::appendChunk(const QString& _strChunk)
{
    Q_UNUSED(_strChunk)
    if (!mbAiBlockOpen) {
        beginAiBlock();
    }
    renderCurrentAiBlock();
}

void AIDockWidget::renderCurrentAiBlock()
{
    if (!mbAiBlockOpen || mnAiContentStart < 0) {
        return;
    }

    const QString _strPlainText = normalizeRawAssistantText(mstrRawAssistantBuffer);
    replaceCurrentAiBlock(_strPlainText, detectPromptKind(_strPlainText));
}

void AIDockWidget::replaceCurrentAiBlock(const QString& _strPlainText,
    PromptKind _kind)
{
    QTextCursor _cursor(mpctrlOutput->document());
    _cursor.setPosition(mnAiContentStart);
    _cursor.setPosition(mnAiContentEnd, QTextCursor::KeepAnchor);
    _cursor.removeSelectedText();

    QFont _monoFont("Consolas", 10);
    _monoFont.setFixedPitch(true);

    auto _makeFormat = [&](const QColor& _color, bool _bBold) {
        QTextCharFormat _fmt;
        _fmt.setForeground(_color);
        _fmt.setFont(_monoFont);
        _fmt.setFontWeight(_bBold ? QFont::Bold : QFont::Normal);
        return _fmt;
    };

    const QStringList _vLines = _strPlainText.split('\n');
    for (int _nLineIdx = 0; _nLineIdx < _vLines.size(); ++_nLineIdx) {
        if (_nLineIdx > 0) {
            _cursor.insertText("\n", _makeFormat(S_C_AI, false));
        }

        QTextCharFormat _fmt = _makeFormat(S_C_AI, false);
        if (_kind != PromptKind::Normal && _nLineIdx == 0
            && !_vLines[_nLineIdx].trimmed().isEmpty()) {
            _fmt = _makeFormat(S_C_SYSTEM, true);
        }

        _cursor.insertText(_vLines[_nLineIdx], _fmt);
    }

    mnAiContentEnd = _cursor.position();
    mpctrlOutput->setTextCursor(_cursor);
    scrollToBottom();
}

QString AIDockWidget::normalizeRawAssistantText(const QString& _strRawText) const
{
    QString _strText = _strRawText;
    _strText.replace("\r\n", "\n");
    _strText.replace('\r', '\n');

    QStringList _vNormalizedLines;
    const QStringList _vRawLines = _strText.split('\n');
    for (QString _strLine : _vRawLines) {
        if (_strLine.contains("<tool_call>") || _strLine.contains("</tool_call>")) {
            continue;
        }
        if (isRawToolCallJsonLine(_strLine)) {
            continue;
        }
        _strLine.remove(QRegularExpression(R"(^\s*```.*$)"));
        _strLine.remove('`');
        _strLine.replace("**", "");
        _strLine.replace("__", "");
        _strLine.replace(QRegularExpression(R"(^\s*#{1,6}\s*)"), "");
        _strLine.replace(QRegularExpression(R"(^\s*[-*]\s+)"), "");
        _strLine.replace(QRegularExpression(R"(^\s*>\s*)"), "");
        _strLine.remove(QRegularExpression(R"(\s+$)"));
        _vNormalizedLines << _strLine;
    }

    QStringList _vCompactedLines;
    bool _bLastWasBlank = false;
    for (const QString& _strLine : _vNormalizedLines) {
        const bool _bBlank = _strLine.trimmed().isEmpty();
        if (_bBlank) {
            if (!_bLastWasBlank) {
                _vCompactedLines << QString();
            }
            _bLastWasBlank = true;
            continue;
        }

        _vCompactedLines << _strLine;
        _bLastWasBlank = false;
    }

    while (!_vCompactedLines.isEmpty()
        && _vCompactedLines.first().trimmed().isEmpty()) {
        _vCompactedLines.removeFirst();
    }
    while (!_vCompactedLines.isEmpty()
        && _vCompactedLines.last().trimmed().isEmpty()) {
        _vCompactedLines.removeLast();
    }

    return _vCompactedLines.join('\n').trimmed();
}

AIDockWidget::PromptKind AIDockWidget::detectPromptKind(
    const QString& _strPlainText) const
{
    const QString _strSimplified = _strPlainText.trimmed();
    if (_strSimplified.startsWith(QString::fromUtf8("请选择一个选项"))) {
        return PromptKind::Choice;
    }
    if (_strSimplified.startsWith(QString::fromUtf8("还需要这些信息"))) {
        return PromptKind::Form;
    }
    return PromptKind::Normal;
}

void AIDockWidget::endAiBlock()
{
    if (!mbAiBlockOpen) {
        return;
    }

    QTextCursor _cursor = mpctrlOutput->textCursor();
    _cursor.movePosition(QTextCursor::End);

    QTextBlockFormat _blkFmt;
    _blkFmt.setBackground(S_C_BG);
    _blkFmt.setTopMargin(4);
    _blkFmt.setBottomMargin(2);
    _cursor.insertBlock(_blkFmt);

    QTextCharFormat _fmt;
    _fmt.setForeground(S_C_DIM);
    QFont _monoFont("Consolas", 10);
    _monoFont.setFixedPitch(true);
    _fmt.setFont(_monoFont);
    _cursor.insertText(QString(58, '-'), _fmt);

    mpctrlOutput->setTextCursor(_cursor);
    mbAiBlockOpen = false;
    mstrRawAssistantBuffer.clear();
    mnAiContentStart = -1;
    mnAiContentEnd = -1;
    scrollToBottom();
}

void AIDockWidget::scrollToBottom()
{
    mpctrlOutput->verticalScrollBar()->setValue(
        mpctrlOutput->verticalScrollBar()->maximum());
}

void AIDockWidget::historyUp()
{
    if (mvHistory.isEmpty()) {
        return;
    }

    if (mnHistoryIdx == -1) {
        mstrInputBackup = mpctrlInput->text();
        mnHistoryIdx = mvHistory.size() - 1;
    } else if (mnHistoryIdx > 0) {
        --mnHistoryIdx;
    }

    mpctrlInput->setText(mvHistory[mnHistoryIdx]);
    mpctrlInput->end(false);
}

void AIDockWidget::historyDown()
{
    if (mnHistoryIdx == -1) {
        return;
    }

    if (mnHistoryIdx < mvHistory.size() - 1) {
        ++mnHistoryIdx;
        mpctrlInput->setText(mvHistory[mnHistoryIdx]);
    } else {
        mnHistoryIdx = -1;
        mpctrlInput->setText(mstrInputBackup);
    }
    mpctrlInput->end(false);
}

void AIDockWidget::pushHistory(const QString& _strCmd)
{
    if (!_strCmd.isEmpty() && (mvHistory.isEmpty() || mvHistory.last() != _strCmd)) {
        mvHistory.append(_strCmd);
    }

    while (mvHistory.size() > 200) {
        mvHistory.removeFirst();
    }
}

void AIDockWidget::setStatus(const QString& _strText, const QColor& _color)
{
    mpctrlStatusBar->setText(_strText);
    mpctrlStatusBar->setStyleSheet(QString(
        "QLabel{background:#252526;color:%1;font-size:14px;"
        "font-family:Consolas,Courier,monospace;padding-left:14px;border-bottom:1px solid #333;}")
        .arg(_color.name()));
}

void AIDockWidget::onSendClicked()
{
    const QString _strText = mpctrlInput->text().trimmed();
    if (_strText.isEmpty() || mbStreaming) {
        return;
    }

    pushHistory(_strText);
    mpctrlInput->clear();
    mnHistoryIdx = -1;

    appendLine(_strText, S_C_USER, false, "> ");
    beginAiBlock();
    mbStreaming = true;
    mpctrlBtnSend->setEnabled(false);
    mpctrlBtnStop->setEnabled(true);
    setStatus(tr("● Ollama  ·  生成中..."), S_C_TOOL);

    if (mpAIManager != nullptr) {
        mpAIManager->sendMessage(_strText);
    } else {
        onErrorOccurred(tr("AIManager 未初始化"));
    }
}

void AIDockWidget::onChunkReceived(const QString& _strChunk)
{
    mstrRawAssistantBuffer += _strChunk;
    appendChunk(_strChunk);
}

void AIDockWidget::onReplyFinished()
{
    renderCurrentAiBlock();
    endAiBlock();
    mbStreaming = false;
    mpctrlBtnSend->setEnabled(true);
    mpctrlBtnStop->setEnabled(false);
    setStatus(tr("● Ollama  qwen2.5:7b  ·  就绪"), S_C_USER);
    mpctrlInput->setFocus();
}

void AIDockWidget::onErrorOccurred(const QString& _strError)
{
    endAiBlock();
    appendLine(tr("错误: %1").arg(_strError), S_C_ERROR, false, "x ");
    mbStreaming = false;
    mpctrlBtnSend->setEnabled(true);
    mpctrlBtnStop->setEnabled(false);
    setStatus(tr("● Ollama  ·  连接失败"), S_C_ERROR);
}

void AIDockWidget::onToolCallStarted(const QString& _strToolName)
{
    if (mbAiBlockOpen) {
        const bool _bHasVisibleText =
            !normalizeRawAssistantText(mstrRawAssistantBuffer).trimmed().isEmpty();
        if (_bHasVisibleText) {
            endAiBlock();
        } else {
            mbAiBlockOpen = false;
            mstrRawAssistantBuffer.clear();
            mnAiContentStart = -1;
            mnAiContentEnd = -1;
        }
    }

    setStatus(tr("● Ollama  ·  执行中..."), S_C_TOOL);
    if (isInternalPlanningTool(_strToolName)) {
        return;
    }

    appendLine(tr("正在执行工具: %1").arg(_strToolName), S_C_TOOL, false, ">> ");
}

void AIDockWidget::onToolCallFinished(const QString& _strToolName,
    const QString& _strResult)
{
    if (isInternalPlanningTool(_strToolName)) {
        return;
    }

    const QString _strPlainText = normalizeRawAssistantText(_strResult);
    if (!_strPlainText.isEmpty()) {
        appendLine(_strPlainText, S_C_SYSTEM, false, "<< ");
    }
}

void AIDockWidget::onClearClicked()
{
    mpctrlOutput->clear();
    mstrRawAssistantBuffer.clear();
    mnAiContentStart = -1;
    mnAiContentEnd = -1;
    printBanner();
    mbStreaming = false;
    mbAiBlockOpen = false;
    mpctrlBtnSend->setEnabled(true);
    mpctrlBtnStop->setEnabled(false);
    setStatus(tr("● Ollama  qwen2.5:7b  ·  就绪"), S_C_USER);

    if (mpAIManager != nullptr) {
        mpAIManager->clearHistory();
    }
}

void AIDockWidget::onStopClicked()
{
    if (mpAIManager != nullptr) {
        mpAIManager->abortCurrentReply();
    }
    mbStreaming = false;
    mpctrlBtnSend->setEnabled(true);
    mpctrlBtnStop->setEnabled(false);
    setStatus(tr("● Ollama  ·  已中断"), S_C_DIM);
}
