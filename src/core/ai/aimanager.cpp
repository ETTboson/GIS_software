#include "aimanager.h"

#include "core/ai/conversationcontext.h"
#include "core/interfaces/iaitoolhost.h"
#include "core/ai/ollamaclient.h"
#include "core/ai/toolcalldispatcher.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace
{

bool isFinalExecutionTool(const QString& _strToolName)
{
    static const QSet<QString> S_SET_FINAL_TOOLS = {
        "run_basic_statistics",
        "run_frequency_statistics",
        "run_neighborhood_analysis"
    };
    return S_SET_FINAL_TOOLS.contains(_strToolName);
}

bool isInternalAnalysisStateTool(const QString& _strToolName)
{
    static const QSet<QString> S_SET_INTERNAL_TOOLS = {
        "set_analysis_params",
        "cancel_analysis"
    };
    return S_SET_INTERNAL_TOOLS.contains(_strToolName);
}

bool looksLikeRawToolCallText(const QString& _strText)
{
    const QString _strTrimmed = _strText.trimmed();
    if (_strTrimmed.isEmpty()) {
        return false;
    }
    if (_strTrimmed.contains("<tool_call>")) {
        return true;
    }
    if (_strTrimmed.startsWith('{')) {
        return true;
    }

    const QStringList _vLines = _strTrimmed.split('\n', Qt::SkipEmptyParts);
    if (_vLines.isEmpty()) {
        return false;
    }

    for (const QString& _strRawLine : _vLines) {
        const QString _strLine = _strRawLine.trimmed();
        if (!_strLine.startsWith('{') || !_strLine.endsWith('}')) {
            return false;
        }
        if (!_strLine.contains("\"name\"") || !_strLine.contains("\"arguments\"")) {
            return false;
        }
    }

    return true;
}

QString extractFirstToolCallJson(const QString& _strRawText)
{
    QString _strText = _strRawText;
    _strText.replace("<tool_call>", "\n");
    _strText.replace("</tool_call>", "\n");

    const QStringList _vLines = _strText.split('\n', Qt::SkipEmptyParts);
    for (const QString& _strRawLine : _vLines) {
        const QString _strLine = _strRawLine.trimmed();
        if (_strLine.startsWith('{')
            && _strLine.endsWith('}')
            && _strLine.contains("\"name\"")
            && _strLine.contains("\"arguments\"")) {
            return _strLine;
        }
    }

    const QString _strTrimmed = _strText.trimmed();
    if (_strTrimmed.startsWith('{')
        && _strTrimmed.endsWith('}')
        && _strTrimmed.contains("\"name\"")
        && _strTrimmed.contains("\"arguments\"")) {
        return _strTrimmed;
    }

    return QString();
}

} // namespace

const QString AIManager::S_STR_MODEL = "qwen2.5:7b";

AIManager::AIManager(QObject* _pParent)
    : QObject(_pParent)
    , mpOllamaClient(new OllamaClient(this))
    , mpConversationContext(new ConversationContext(QString(), this))
    , mpToolCallDispatcher(new ToolCallDispatcher(this))
    , mbStreaming(false)
    , mbToolCallTextMode(false)
    , mRequestMode(RequestMode::General)
{
    connect(mpOllamaClient, &OllamaClient::chunkReceived,
        this, &AIManager::onChunkReceived);
    connect(mpOllamaClient, &OllamaClient::replyFinished,
        this, &AIManager::onReplyFinished);
    connect(mpOllamaClient, &OllamaClient::errorOccurred,
        this, &AIManager::onNetworkError);

    connect(mpToolCallDispatcher, &ToolCallDispatcher::toolExecuted,
        this, &AIManager::onToolExecuted);
    connect(mpToolCallDispatcher, &ToolCallDispatcher::toolFailed,
        this, &AIManager::onToolFailed);

    mpToolCallDispatcher->setConversationContext(mpConversationContext);
}

void AIManager::setToolHost(IAIToolHost* _pToolHost)
{
    mpToolCallDispatcher->setToolHost(_pToolHost);
    mAnalysisWorkflowCoordinator.setToolHost(_pToolHost);
}

void AIManager::sendMessage(const QString& _strUserMsg)
{
    if (_strUserMsg.trimmed().isEmpty()) {
        return;
    }
    if (mbStreaming) {
        return;
    }

    resetStreamState();
    mpConversationContext->appendUserMessage(_strUserMsg);
    mstrLatestUserMessage = _strUserMsg;

    const bool _bAnalysisModeActive =
        mAnalysisWorkflowCoordinator.isAnalysisModeActive();
    const bool _bShouldEnterAnalysisMode =
        mAnalysisWorkflowCoordinator.shouldEnterAnalysisMode(_strUserMsg);

    if (_bAnalysisModeActive || _bShouldEnterAnalysisMode) {
        const AnalysisWorkflowCoordinator::AnalysisTurnKind _turnKind =
            mAnalysisWorkflowCoordinator.classifyTurn(_strUserMsg);

        if (_turnKind == AnalysisWorkflowCoordinator::AnalysisTurnKind::ExitAnalysisMode) {
            resetAnalysisMode();
            mRequestMode = RequestMode::General;
            mbStreaming = true;
            postRequest();
            return;
        }

        mAnalysisWorkflowCoordinator.beginOrContinue(_strUserMsg);
        mbStreaming = true;

        if (_turnKind == AnalysisWorkflowCoordinator::AnalysisTurnKind::Cancel) {
            const AnalysisWorkflowCoordinator::TransitionResult _transition =
                mAnalysisWorkflowCoordinator.handleDirectUserAction(_strUserMsg);
            if (_transition.kind
                == AnalysisWorkflowCoordinator::TransitionKind::FinishWithText) {
                resetAnalysisMode();
                finishWithAssistantText(_transition.strFinalText);
                return;
            }
        }

        AnalysisWorkflowCoordinator::TransitionResult _localTransition;
        if (mAnalysisWorkflowCoordinator.tryApplyLocalStateUpdate(
            _strUserMsg, _localTransition)) {
            if (_localTransition.kind
                == AnalysisWorkflowCoordinator::TransitionKind::ExecuteTool) {
                dispatchToolCall(_localTransition.strToolName,
                    _localTransition.jsonToolArgs, true);
                return;
            }
            if (_localTransition.kind
                == AnalysisWorkflowCoordinator::TransitionKind::FinishWithText) {
                resetAnalysisMode();
                finishWithAssistantText(_localTransition.strFinalText);
                return;
            }
            if (_localTransition.kind
                == AnalysisWorkflowCoordinator::TransitionKind::Error) {
                mbStreaming = false;
                resetAnalysisMode();
                resetStreamState();
                emit errorOccurred(_localTransition.strError);
                return;
            }

            finishWithAssistantText(
                mAnalysisWorkflowCoordinator.buildDeterministicReply());
            return;
        }

        if (_turnKind == AnalysisWorkflowCoordinator::AnalysisTurnKind::StateUpdate) {
            startAnalysisStateUpdateRound();
            return;
        }

        finishWithAssistantText(mAnalysisWorkflowCoordinator.buildDeterministicReply());
        return;
    }

    mRequestMode = RequestMode::General;
    mbStreaming = true;
    postRequest();
}

void AIManager::clearHistory()
{
    mpOllamaClient->abort();
    resetStreamState();
    mpConversationContext->clear();
    resetAnalysisMode();
    mstrLatestUserMessage.clear();
    mbStreaming = false;
}

void AIManager::abortCurrentReply()
{
    if (!mbStreaming) {
        return;
    }

    mbStreaming = false;
    mpOllamaClient->abort();
    resetAnalysisMode();
    resetStreamState();
    emit replyFinished();
}

void AIManager::invalidateSystemContext()
{
    mpConversationContext->invalidateSystemContext();
    mAnalysisPromptBuilder.invalidate();
}

bool AIManager::saveMemory(const QString& _strContent,
    const QString& _strSection, bool _bOverwrite)
{
    return mpConversationContext->saveMemory(_strContent, _strSection, _bOverwrite);
}

void AIManager::postRequest()
{
    QJsonObject _body;
    _body["model"] = S_STR_MODEL;
    _body["stream"] = true;
    _body["options"] = QJsonObject{
        { "temperature", (mRequestMode == RequestMode::AnalysisStateUpdate) ? 0.1 : 0.3 }
    };

    if (mRequestMode == RequestMode::AnalysisStateUpdate) {
        _body["messages"] = mAnalysisWorkflowCoordinator.buildStateUpdateMessages(
            mAnalysisPromptBuilder.buildAnalysisModeBasePrompt(),
            mpConversationContext->getMessages(),
            mstrLatestUserMessage);
        _body["tools"] = mAnalysisWorkflowCoordinator.buildStateUpdateToolsDefinition();
    } else {
        _body["messages"] = mpConversationContext->getMessagesForRequest();
        _body["tools"] = mpToolCallDispatcher->buildToolsDefinition();
    }

    mpOllamaClient->postRequest(_body);
}

void AIManager::resetStreamState()
{
    mstrAssistantBuf.clear();
    mmapAccumulators.clear();
    mjsonPendingToolCalls = QJsonArray();
    mbToolCallTextMode = false;
}

void AIManager::accumulateToolCalls(const QJsonArray& _toolCalls)
{
    for (const QJsonValue& _jsonVal : _toolCalls) {
        const QJsonObject _tc = _jsonVal.toObject();
        const QJsonObject _fn = _tc["function"].toObject();

        int _nIdx = _fn["index"].toInt(-1);
        if (_nIdx < 0) {
            _nIdx = _tc["index"].toInt(-1);
        }
        if (_nIdx < 0) {
            _nIdx = 0;
        }

        ToolCallAccumulator& _acc = mmapAccumulators[_nIdx];
        const QString _strName = _fn["name"].toString();
        if (_acc.strName.isEmpty() && !_strName.isEmpty()) {
            _acc.strName = _strName;
        }

        const QJsonValue _argsVal = _fn["arguments"];
        if (_argsVal.isObject()) {
            _acc.strArgsBuf = QString::fromUtf8(
                QJsonDocument(_argsVal.toObject()).toJson(QJsonDocument::Compact));
        } else {
            _acc.strArgsBuf += _argsVal.toString();
        }
    }
}

bool AIManager::flushAccumulators(QJsonArray& _outToolCalls, QString& _strError)
{
    _outToolCalls = QJsonArray();
    for (auto _it = mmapAccumulators.constBegin();
        _it != mmapAccumulators.constEnd(); ++_it) {
        const ToolCallAccumulator& _acc = _it.value();

        QJsonParseError _parseErr;
        const QJsonDocument _doc = QJsonDocument::fromJson(
            _acc.strArgsBuf.toUtf8(), &_parseErr);
        if (_parseErr.error != QJsonParseError::NoError || !_doc.isObject()) {
            _strError = tr("tool_call[%1] arguments 解析失败（工具名: %2）: %3\n原始内容: %4")
                .arg(_it.key())
                .arg(_acc.strName)
                .arg(_parseErr.errorString())
                .arg(_acc.strArgsBuf);
            return false;
        }

        QJsonObject _fnObj;
        _fnObj["name"] = _acc.strName;
        _fnObj["arguments"] = _doc.object();

        QJsonObject _tcObj;
        _tcObj["function"] = _fnObj;
        _outToolCalls.append(_tcObj);
    }

    return true;
}

void AIManager::onChunkReceived(const QJsonObject& _chunk)
{
    if (!mbStreaming) {
        return;
    }

    const QJsonObject _message = _chunk["message"].toObject();
    const QJsonArray _toolCalls = _message["tool_calls"].toArray();
    if (!_toolCalls.isEmpty()) {
        mbToolCallTextMode = true;
        accumulateToolCalls(_toolCalls);
        return;
    }

    const QString _strContent = _message["content"].toString();
    if (_strContent.isEmpty()) {
        return;
    }

    if (_strContent.contains("<tool_call>")
        || looksLikeRawToolCallText(mstrAssistantBuf + _strContent)) {
        mbToolCallTextMode = true;
    }

    mstrAssistantBuf += _strContent;
    if (!mbToolCallTextMode && mRequestMode != RequestMode::AnalysisStateUpdate) {
        emit chunkReceived(_strContent);
    }
}

void AIManager::onReplyFinished(const QJsonObject& _finalChunk)
{
    if (!mbStreaming) {
        resetStreamState();
        return;
    }

    const QJsonObject _message = _finalChunk["message"].toObject();
    const QJsonArray _toolCalls = _message["tool_calls"].toArray();
    const QString _strFinalContent = _message["content"].toString();

    if (!_strFinalContent.isEmpty()) {
        if (_strFinalContent.contains("<tool_call>")
            || looksLikeRawToolCallText(mstrAssistantBuf + _strFinalContent)) {
            mbToolCallTextMode = true;
        }
        mstrAssistantBuf += _strFinalContent;
        if (!mbToolCallTextMode
            && _toolCalls.isEmpty()
            && mRequestMode != RequestMode::AnalysisStateUpdate) {
            emit chunkReceived(_strFinalContent);
        }
    }

    if (!_toolCalls.isEmpty()) {
        mbToolCallTextMode = true;
        accumulateToolCalls(_toolCalls);
    }

    if (!mmapAccumulators.isEmpty()) {
        dispatchFromAccumulators();
        return;
    }

    if (mbToolCallTextMode) {
        dispatchFromTextBuffer();
        return;
    }

    if (mRequestMode == RequestMode::AnalysisStateUpdate) {
        finishWithAssistantText(mAnalysisWorkflowCoordinator.buildDeterministicReply());
        return;
    }

    const QString _strFinalText = mstrAssistantBuf;
    mpConversationContext->appendAssistantMessage(_strFinalText);
    mbStreaming = false;
    resetStreamState();
    emit replyFinished();
}

void AIManager::dispatchFromAccumulators()
{
    QString _strFlushError;
    QJsonArray _outToolCalls;
    if (!flushAccumulators(_outToolCalls, _strFlushError)) {
        mbStreaming = false;
        resetStreamState();
        emit errorOccurred(_strFlushError);
        return;
    }

    mjsonPendingToolCalls = _outToolCalls;
    const QJsonObject _fnObj = mjsonPendingToolCalls[0].toObject()["function"].toObject();
    const QString _strName = _fnObj["name"].toString();
    const QJsonObject _jsonArgs = _fnObj["arguments"].toObject();

    mmapAccumulators.clear();
    mstrAssistantBuf.clear();

    if (mRequestMode == RequestMode::AnalysisStateUpdate
        && isInternalAnalysisStateTool(_strName)) {
        handleAnalysisStateUpdateTool(_strName, _jsonArgs);
        return;
    }

    dispatchToolCall(_strName, _jsonArgs, true);
}

void AIManager::dispatchFromTextBuffer()
{
    const QString _strJson = extractFirstToolCallJson(mstrAssistantBuf);
    if (_strJson.isEmpty()) {
        mbStreaming = false;
        resetStreamState();
        emit errorOccurred(tr("tool_call 文本解析失败：未找到有效的工具调用 JSON"));
        return;
    }

    QJsonParseError _parseErr;
    const QJsonDocument _doc = QJsonDocument::fromJson(_strJson.toUtf8(), &_parseErr);
    if (_parseErr.error != QJsonParseError::NoError || !_doc.isObject()) {
        mbStreaming = false;
        resetStreamState();
        emit errorOccurred(tr("tool_call 文本解析失败: %1\n原始内容: %2")
            .arg(_parseErr.errorString(), _strJson));
        return;
    }

    const QJsonObject _jsonObj = _doc.object();
    const QString _strName = _jsonObj["name"].toString();
    const QJsonObject _jsonArgs = _jsonObj["arguments"].toObject();
    if (_strName.isEmpty()) {
        mbStreaming = false;
        resetStreamState();
        emit errorOccurred(tr("tool_call 文本解析失败：name 字段为空"));
        return;
    }

    if (mRequestMode == RequestMode::AnalysisStateUpdate
        && isInternalAnalysisStateTool(_strName)) {
        mstrAssistantBuf.clear();
        handleAnalysisStateUpdateTool(_strName, _jsonArgs);
        return;
    }

    dispatchToolCall(_strName, _jsonArgs, true);
}

void AIManager::dispatchToolCall(const QString& _strToolName,
    const QJsonObject& _jsonArgs,
    bool _bRecordIntent)
{
    if (_bRecordIntent) {
        QJsonObject _fnObj;
        _fnObj["name"] = _strToolName;
        _fnObj["arguments"] = _jsonArgs;

        QJsonObject _tcObj;
        _tcObj["function"] = _fnObj;

        mjsonPendingToolCalls = QJsonArray();
        mjsonPendingToolCalls.append(_tcObj);
        mpConversationContext->appendToolCallIntent(mjsonPendingToolCalls);
    }

    mstrAssistantBuf.clear();
    emit toolCallStarted(_strToolName);
    mpToolCallDispatcher->dispatch(_strToolName, _jsonArgs);
}

void AIManager::startAnalysisStateUpdateRound()
{
    resetStreamState();
    mRequestMode = RequestMode::AnalysisStateUpdate;
    postRequest();
}

void AIManager::handleAnalysisStateUpdateTool(const QString& _strToolName,
    const QJsonObject& _jsonArgs)
{
    const AnalysisWorkflowCoordinator::TransitionResult _transition =
        mAnalysisWorkflowCoordinator.applyStateUpdateTool(_strToolName, _jsonArgs);

    if (_transition.kind == AnalysisWorkflowCoordinator::TransitionKind::Error) {
        mbStreaming = false;
        resetAnalysisMode();
        resetStreamState();
        emit errorOccurred(_transition.strError);
        return;
    }

    if (_transition.kind == AnalysisWorkflowCoordinator::TransitionKind::FinishWithText) {
        resetAnalysisMode();
        finishWithAssistantText(_transition.strFinalText);
        return;
    }

    if (_transition.kind == AnalysisWorkflowCoordinator::TransitionKind::ExecuteTool) {
        mRequestMode = RequestMode::General;
        dispatchToolCall(_transition.strToolName, _transition.jsonToolArgs, true);
        return;
    }

    finishWithAssistantText(mAnalysisWorkflowCoordinator.buildDeterministicReply());
}

void AIManager::finishWithAssistantText(const QString& _strText)
{
    const QString _strFinalText = _strText.trimmed();
    if (!_strFinalText.isEmpty()) {
        emit chunkReceived(_strFinalText);
        mpConversationContext->appendAssistantMessage(_strFinalText);
    }

    mbStreaming = false;
    mRequestMode = RequestMode::General;
    resetStreamState();
    emit replyFinished();
}

void AIManager::resetAnalysisMode()
{
    mAnalysisWorkflowCoordinator.reset();
    mRequestMode = RequestMode::General;
}

void AIManager::onNetworkError(const QString& _strError)
{
    mbStreaming = false;
    if (mRequestMode == RequestMode::AnalysisStateUpdate) {
        resetAnalysisMode();
    } else {
        mRequestMode = RequestMode::General;
    }
    resetStreamState();
    emit errorOccurred(_strError);
}

void AIManager::onToolExecuted(const QString& _strToolName,
    const QString& _strResult)
{
    emit toolCallFinished(_strToolName, _strResult);
    mpConversationContext->appendToolResult(_strToolName, _strResult);

    if (isFinalExecutionTool(_strToolName)) {
        mpConversationContext->appendAssistantMessage(_strResult);
        mbStreaming = false;
        resetAnalysisMode();
        resetStreamState();
        emit replyFinished();
        return;
    }

    resetStreamState();
    postRequest();
}

void AIManager::onToolFailed(const QString& _strToolName,
    const QString& _strError)
{
    emit toolCallFinished(_strToolName, tr("执行失败：%1").arg(_strError));

    if (isFinalExecutionTool(_strToolName)) {
        const QString _strFinalText = tr("执行失败：%1").arg(_strError);
        mpConversationContext->appendAssistantMessage(_strFinalText);
        mbStreaming = false;
        resetAnalysisMode();
        resetStreamState();
        emit replyFinished();
        return;
    }

    mpConversationContext->appendToolResult(
        _strToolName, tr("工具执行失败：%1").arg(_strError));
    resetStreamState();
    postRequest();
}
