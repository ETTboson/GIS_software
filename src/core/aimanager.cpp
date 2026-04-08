// src/core/aimanager.cpp
#include "aimanager.h"
#include "ollamaclient.h"
#include "conversationcontext.h"
#include "toolcalldispatcher.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

const QString AIManager::S_STR_MODEL = "qwen2.5:7b";

AIManager::AIManager(QObject* _pParent)
    : QObject(_pParent)
    , mpOllamaClient(new OllamaClient(this))
    , mpConversationContext(new ConversationContext(QString(), this))
    , mpToolCallDispatcher(new ToolCallDispatcher(this))
    , mbStreaming(false)
    , mbToolCallTextMode(false)
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

void AIManager::setAnalysisService(AnalysisService* _pAnalysisService)
{
    mpToolCallDispatcher->setAnalysisService(_pAnalysisService);
}

void AIManager::sendMessage(const QString& _strUserMsg)
{
    if (_strUserMsg.trimmed().isEmpty()) {
        return;
    }
    if (mbStreaming) {
        qWarning() << "[AIManager] 正在流式接收中，忽略新消息";
        return;
    }

    resetStreamState();
    mpConversationContext->appendUserMessage(_strUserMsg);
    mbStreaming = true;
    postRequest();
}

void AIManager::clearHistory()
{
    mpOllamaClient->abort();
    resetStreamState();
    mpConversationContext->clear();
    mbStreaming = false;
}

void AIManager::abortCurrentReply()
{
    if (!mbStreaming) {
        return;
    }

    mpOllamaClient->abort();
    resetStreamState();
    mbStreaming = false;
    emit replyFinished();
}

void AIManager::invalidateSystemContext()
{
    mpConversationContext->invalidateSystemContext();
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
    _body["messages"] = mpConversationContext->getMessagesForRequest();
    _body["tools"] = mpToolCallDispatcher->buildToolsDefinition();
    _body["stream"] = true;
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
    for (const QJsonValue& _tv : _toolCalls) {
        const QJsonObject _tc = _tv.toObject();
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
    for (auto _it = mmapAccumulators.constBegin(); _it != mmapAccumulators.constEnd(); ++_it) {
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

    if (_strContent.contains("<tool_call>")) {
        mbToolCallTextMode = true;
    }

    mstrAssistantBuf += _strContent;
    if (!mbToolCallTextMode) {
        emit chunkReceived(_strContent);
    }
}

void AIManager::onReplyFinished(const QJsonObject& _finalChunk)
{
    const QJsonObject _message = _finalChunk["message"].toObject();
    const QJsonArray _toolCalls = _message["tool_calls"].toArray();
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

    mpConversationContext->appendAssistantMessage(mstrAssistantBuf);
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
    mpConversationContext->appendToolCallIntent(mjsonPendingToolCalls);

    const QJsonObject _fnObj = mjsonPendingToolCalls[0].toObject()["function"].toObject();
    const QString _strName = _fnObj["name"].toString();
    const QJsonObject _args = _fnObj["arguments"].toObject();

    mmapAccumulators.clear();
    mstrAssistantBuf.clear();
    emit toolCallStarted(_strName);
    mpToolCallDispatcher->dispatch(_strName, _args);
}

void AIManager::dispatchFromTextBuffer()
{
    QString _strJson;
    const int _nStartRaw = mstrAssistantBuf.indexOf("<tool_call>");
    const int _nEnd = mstrAssistantBuf.indexOf("</tool_call>");

    if (_nStartRaw >= 0 && _nEnd > _nStartRaw) {
        const int _nStart = _nStartRaw + QString("<tool_call>").length();
        _strJson = mstrAssistantBuf.mid(_nStart, _nEnd - _nStart).trimmed();
    } else if (_nStartRaw >= 0) {
        const int _nStart = _nStartRaw + QString("<tool_call>").length();
        _strJson = mstrAssistantBuf.mid(_nStart).trimmed();
    } else {
        _strJson = mstrAssistantBuf.trimmed();
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

    const QJsonObject _tcObj = _doc.object();
    const QString _strName = _tcObj["name"].toString();
    const QJsonObject _args = _tcObj["arguments"].toObject();
    if (_strName.isEmpty()) {
        mbStreaming = false;
        resetStreamState();
        emit errorOccurred(tr("tool_call 文本解析失败：name 字段为空\n原始内容: %1")
            .arg(_strJson));
        return;
    }

    QJsonObject _fnObj;
    _fnObj["name"] = _strName;
    _fnObj["arguments"] = _args;

    QJsonObject _tcWrap;
    _tcWrap["function"] = _fnObj;
    mjsonPendingToolCalls = QJsonArray();
    mjsonPendingToolCalls.append(_tcWrap);

    mpConversationContext->appendToolCallIntent(mjsonPendingToolCalls);
    mstrAssistantBuf.clear();
    emit toolCallStarted(_strName);
    mpToolCallDispatcher->dispatch(_strName, _args);
}

void AIManager::onNetworkError(const QString& _strError)
{
    mbStreaming = false;
    resetStreamState();
    emit errorOccurred(_strError);
}

void AIManager::onToolExecuted(const QString& _strToolName,
    const QString& _strResult)
{
    emit toolCallFinished(_strToolName, _strResult);
    mpConversationContext->appendToolResult(_strToolName, _strResult);
    resetStreamState();
    postRequest();
}

void AIManager::onToolFailed(const QString& _strToolName,
    const QString& _strError)
{
    emit toolCallFinished(_strToolName, tr("执行失败：%1").arg(_strError));
    mpConversationContext->appendToolResult(
        _strToolName, tr("工具执行失败：%1").arg(_strError));
    resetStreamState();
    postRequest();
}
