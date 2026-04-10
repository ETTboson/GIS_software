// src/core/conversationcontext.cpp
#include "conversationcontext.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QJsonObject>

ConversationContext::ConversationContext(const QString& _strWorkDir,
    QObject* _pParent)
    : QObject(_pParent)
    , mMemoryFileManager(_strWorkDir)
    , mvMessages(QJsonArray())
    , mnMaxMessages(40)
    , mnKeepRecentCount(20)
    , mbWarmInjectionEnabled(true)
{
}

void ConversationContext::appendUserMessage(const QString& _strContent)
{
    QString _strFinalContent = _strContent;
    if (mbWarmInjectionEnabled) {
        const QString _strWarmData = loadWarmDataForInjection();
        if (!_strWarmData.isEmpty()) {
            _strFinalContent = _strWarmData + "\n\n" + _strContent;
        }
    }

    QJsonObject _msg;
    _msg["role"] = "user";
    _msg["content"] = _strFinalContent;
    mvMessages.append(_msg);
    compactIfNeeded();
}

void ConversationContext::appendAssistantMessage(const QString& _strContent)
{
    if (_strContent.isEmpty()) {
        return;
    }

    QJsonObject _msg;
    _msg["role"] = "assistant";
    _msg["content"] = _strContent;
    mvMessages.append(_msg);
}

void ConversationContext::appendToolCallIntent(const QJsonArray& _jsonToolCalls)
{
    QJsonObject _msg;
    _msg["role"] = "assistant";
    _msg["content"] = QString();
    _msg["tool_calls"] = _jsonToolCalls;
    mvMessages.append(_msg);
}

void ConversationContext::appendToolResult(const QString& _strToolName,
    const QString& _strResult)
{
    QJsonObject _msg;
    _msg["role"] = "tool";
    _msg["name"] = _strToolName;
    _msg["content"] = _strResult;
    mvMessages.append(_msg);
}

void ConversationContext::clear()
{
    mvMessages = QJsonArray();
    mstrLastWarmDataHash.clear();
}

QJsonArray ConversationContext::getMessages() const
{
    return mvMessages;
}

QJsonArray ConversationContext::getMessagesForRequest()
{
    QJsonArray _jsonResult;
    _jsonResult.append(mSystemContextBuilder.buildOnce());
    for (const QJsonValue& _jsonVal : mvMessages) {
        _jsonResult.append(_jsonVal);
    }
    return _jsonResult;
}

int ConversationContext::messageCount() const
{
    return mvMessages.size();
}

bool ConversationContext::saveMemory(const QString& _strContent,
    const QString& _strSection, bool _bOverwrite)
{
    const bool _bOk = mMemoryFileManager.saveMemory(
        _strContent, _strSection, _bOverwrite);
    if (_bOk) {
        mstrLastWarmDataHash.clear();
    }
    return _bOk;
}

QList<MemorySearchResult> ConversationContext::searchMemory(
    const QString& _strKeyword) const
{
    return mMemoryFileManager.searchCold(_strKeyword);
}

void ConversationContext::invalidateSystemContext()
{
    mSystemContextBuilder.invalidate();
}

bool ConversationContext::hasMemoryFiles() const
{
    return mMemoryFileManager.hasMemoryFiles();
}

void ConversationContext::compactHistory()
{
    if (mvMessages.size() <= mnKeepRecentCount) {
        return;
    }

    const int _nKeepHead = qMin(2, mvMessages.size());
    const int _nDropCount = mvMessages.size() - _nKeepHead - mnKeepRecentCount;
    if (_nDropCount <= 0) {
        return;
    }

    QJsonArray _jsonDropped;
    for (int _nMsgIdx = _nKeepHead; _nMsgIdx < _nKeepHead + _nDropCount; ++_nMsgIdx) {
        _jsonDropped.append(mvMessages[_nMsgIdx]);
    }

    QJsonArray _jsonNewMessages;
    for (int _nMsgIdx = 0; _nMsgIdx < _nKeepHead; ++_nMsgIdx) {
        _jsonNewMessages.append(mvMessages[_nMsgIdx]);
    }

    QJsonObject _jsonSummary;
    _jsonSummary["role"] = "system";
    _jsonSummary["content"] = buildCompactSummary(_jsonDropped);
    _jsonNewMessages.append(_jsonSummary);

    for (int _nMsgIdx = mvMessages.size() - mnKeepRecentCount;
        _nMsgIdx < mvMessages.size(); ++_nMsgIdx) {
        _jsonNewMessages.append(mvMessages[_nMsgIdx]);
    }

    mvMessages = _jsonNewMessages;
}

void ConversationContext::setMaxMessages(int _nMaxMessages)
{
    mnMaxMessages = _nMaxMessages;
}

void ConversationContext::setKeepRecentCount(int _nKeepRecentCount)
{
    mnKeepRecentCount = _nKeepRecentCount;
}

void ConversationContext::setWarmDataInjectionEnabled(bool _bEnabled)
{
    mbWarmInjectionEnabled = _bEnabled;
}

void ConversationContext::compactIfNeeded()
{
    if (mvMessages.size() > mnMaxMessages) {
        compactHistory();
    }
}

QString ConversationContext::buildCompactSummary(
    const QJsonArray& _jsonDroppedMessages) const
{
    int _nUserCount = 0;
    int _nAssistantCount = 0;
    int _nToolCount = 0;

    for (const QJsonValue& _jsonVal : _jsonDroppedMessages) {
        const QString _strRole = _jsonVal.toObject()["role"].toString();
        if (_strRole == "user") {
            ++_nUserCount;
        } else if (_strRole == "assistant") {
            ++_nAssistantCount;
        } else if (_strRole == "tool") {
            ++_nToolCount;
        }
    }

    return QString(
        "[对话历史已压缩]\n"
        "共截断 %1 条较早消息：用户 %2 条，助手 %3 条，工具结果 %4 条。\n"
        "如需查找更早的信息，请使用 search_memory 工具检索记忆文件。")
        .arg(_jsonDroppedMessages.size())
        .arg(_nUserCount)
        .arg(_nAssistantCount)
        .arg(_nToolCount);
}

QString ConversationContext::loadWarmDataForInjection()
{
    const QString _strWarmData = mMemoryFileManager.loadWarm();
    if (_strWarmData.isEmpty()) {
        return QString();
    }

    const QString _strHash = QString::fromLatin1(
        QCryptographicHash::hash(_strWarmData.toUtf8(),
            QCryptographicHash::Md5).toHex());
    if (_strHash == mstrLastWarmDataHash) {
        return QString();
    }

    mstrLastWarmDataHash = _strHash;
    return _strWarmData;
}
