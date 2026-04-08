#include "aimodule.h"

#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QUrl>
#include <QDebug>

const QString AIModule::S_STR_API_URL = "http://localhost:11434/api/chat";
const QString AIModule::S_STR_MODEL   = "qwen2.5:7b";

// ════════════════════════════════════════════════════════
//  构造 / 析构
// ════════════════════════════════════════════════════════
AIModule::AIModule(QObject *_pParent)
    : QObject(_pParent)
    , mpNetManager(new QNetworkAccessManager(this))
    , mpCurrentReply(nullptr)
    , mbHasToolCall(false)
{
}

AIModule::~AIModule()
{
    if (mpCurrentReply != nullptr) {
        mpCurrentReply->abort();
        mpCurrentReply->deleteLater();
        mpCurrentReply = nullptr;
    }
}

// ════════════════════════════════════════════════════════
//  buildToolsDefinition
//  !! 新增工具只需在此函数末尾追加一个 QJsonObject !!
//
//  每个工具格式：
//  {
//    "type": "function",
//    "function": {
//      "name": "工具名",
//      "description": "何时调用此工具的描述（中文即可）",
//      "parameters": {
//        "type": "object",
//        "properties": { 参数定义... },
//        "required": [ 必填参数名列表 ]
//      }
//    }
//  }
// ════════════════════════════════════════════════════════
QJsonArray AIModule::buildToolsDefinition() const
{
    QJsonArray _tools;

    // ── 1. 缓冲区分析 ────────────────────────────────
    {
        QJsonObject _params;
        _params["type"] = "object";

        QJsonObject _radiusProp;
        _radiusProp["type"]        = "number";
        _radiusProp["description"] = "缓冲区半径，单位：米，例如 500";

        QJsonObject _properties;
        _properties["radius_meters"] = _radiusProp;
        _params["properties"] = _properties;
        _params["required"]   = QJsonArray{ "radius_meters" };

        QJsonObject _fn;
        _fn["name"]        = "run_buffer_analysis";
        _fn["description"] = "对当前已加载的矢量图层执行缓冲区分析。"
                             "调用前必须确认用户已提供缓冲半径（米）。"
                             "若用户未提供，请先向用户询问。";
        _fn["parameters"]  = _params;

        QJsonObject _tool;
        _tool["type"]     = "function";
        _tool["function"] = _fn;
        _tools.append(_tool);
    }

    // ── 2. 叠加分析 ──────────────────────────────────
    {
        QJsonObject _params;
        _params["type"] = "object";

        QJsonObject _typeProp;
        _typeProp["type"]        = "string";
        _typeProp["description"] = "叠加类型：intersection（交集）、union（并集）、difference（差集）";
        _typeProp["enum"]        = QJsonArray{ "intersection", "union", "difference" };

        QJsonObject _properties;
        _properties["overlay_type"] = _typeProp;
        _params["properties"] = _properties;
        _params["required"]   = QJsonArray{ "overlay_type" };

        QJsonObject _fn;
        _fn["name"]        = "run_overlay_analysis";
        _fn["description"] = "对已加载的两个图层执行叠加分析。"
                             "调用前需确认叠加类型（交集/并集/差集）。";
        _fn["parameters"]  = _params;

        QJsonObject _tool;
        _tool["type"]     = "function";
        _tool["function"] = _fn;
        _tools.append(_tool);
    }

    // ── 3. 空间查询 ──────────────────────────────────
    {
        QJsonObject _params;
        _params["type"] = "object";

        QJsonObject _exprProp;
        _exprProp["type"]        = "string";
        _exprProp["description"] = "查询表达式，例如 \"area > 1000\" 或 \"name = '北京'\"";

        QJsonObject _properties;
        _properties["query_expression"] = _exprProp;
        _params["properties"] = _properties;
        _params["required"]   = QJsonArray{ "query_expression" };

        QJsonObject _fn;
        _fn["name"]        = "run_spatial_query";
        _fn["description"] = "根据属性条件对当前图层执行空间查询。"
                             "调用前需确认查询表达式。";
        _fn["parameters"]  = _params;

        QJsonObject _tool;
        _tool["type"]     = "function";
        _tool["function"] = _fn;
        _tools.append(_tool);
    }

    // ── 4. 栅格计算 ──────────────────────────────────
    {
        QJsonObject _params;
        _params["type"] = "object";

        QJsonObject _exprProp;
        _exprProp["type"]        = "string";
        _exprProp["description"] = "栅格计算表达式，例如 \"(Band1 - Band2) / (Band1 + Band2)\"";

        QJsonObject _properties;
        _properties["expression"] = _exprProp;
        _params["properties"] = _properties;
        _params["required"]   = QJsonArray{ "expression" };

        QJsonObject _fn;
        _fn["name"]        = "run_raster_calc";
        _fn["description"] = "执行栅格波段计算。调用前需确认计算表达式。";
        _fn["parameters"]  = _params;

        QJsonObject _tool;
        _tool["type"]     = "function";
        _tool["function"] = _fn;
        _tools.append(_tool);
    }

    // ── 在此继续追加新工具 ────────────────────────────

    return _tools;
}

// ════════════════════════════════════════════════════════
//  sendMessage — 追加用户消息并发起请求
// ════════════════════════════════════════════════════════
void AIModule::sendMessage(const QString &_strUserMsg)
{
    if (_strUserMsg.trimmed().isEmpty()) { return; }

    if (mpCurrentReply != nullptr) {
        mpCurrentReply->abort();
        mpCurrentReply->deleteLater();
        mpCurrentReply = nullptr;
    }

    QJsonObject _userMsg;
    _userMsg["role"]    = "user";
    _userMsg["content"] = _strUserMsg;
    mvMessages.append(_userMsg);

    mstrAssistantBuf.clear();
    mstrToolCallBuf.clear();
    mbHasToolCall = false;

    postRequest();
}

// ════════════════════════════════════════════════════════
//  submitToolResult — 工具执行完毕，把结果回传给 AI
// ════════════════════════════════════════════════════════
void AIModule::submitToolResult(const QString &_strToolName,
                                const QString &_strResult)
{
    // 把工具结果作为 tool 角色消息追加进历史
    QJsonObject _toolMsg;
    _toolMsg["role"]    = "tool";
    _toolMsg["name"]    = _strToolName;
    _toolMsg["content"] = _strResult;
    mvMessages.append(_toolMsg);

    mstrAssistantBuf.clear();
    mstrToolCallBuf.clear();
    mbHasToolCall = false;

    // 继续请求，让 AI 根据工具结果生成最终回复
    postRequest();
}

// ════════════════════════════════════════════════════════
//  clearHistory
// ════════════════════════════════════════════════════════
void AIModule::clearHistory()
{
    mvMessages      = QJsonArray();
    mstrAssistantBuf.clear();
    mstrToolCallBuf.clear();
    mbHasToolCall   = false;
}

// ════════════════════════════════════════════════════════
//  postRequest — 构造请求体并发送
// ════════════════════════════════════════════════════════
void AIModule::postRequest()
{
    QJsonObject _body;
    _body["model"]    = S_STR_MODEL;
    _body["messages"] = mvMessages;
    _body["tools"]    = buildToolsDefinition();
    _body["stream"]   = true;

    QUrl            _url(S_STR_API_URL);
    QNetworkRequest _netRequest(_url);
    _netRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                          QString("application/json"));

    QByteArray _baBody = QJsonDocument(_body).toJson(QJsonDocument::Compact);
    mpCurrentReply = mpNetManager->post(_netRequest, _baBody);

    connect(mpCurrentReply, &QNetworkReply::readyRead,
            this, &AIModule::onReadyRead);
    connect(mpCurrentReply, &QNetworkReply::finished,
            this, &AIModule::onReplyFinished);
    connect(mpCurrentReply,
            QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &AIModule::onNetworkError);
}

// ════════════════════════════════════════════════════════
//  onReadyRead — 逐行解析流式 NDJSON
// ════════════════════════════════════════════════════════
void AIModule::onReadyRead()
{
    if (mpCurrentReply == nullptr) { return; }

    while (mpCurrentReply->canReadLine()) {
        QByteArray _baLine = mpCurrentReply->readLine().trimmed();
        if (_baLine.isEmpty()) { continue; }

        QJsonParseError _parseErr;
        QJsonDocument   _doc = QJsonDocument::fromJson(_baLine, &_parseErr);
        if (_parseErr.error != QJsonParseError::NoError) {
            qWarning() << "[AIModule] JSON 解析失败:" << _parseErr.errorString();
            continue;
        }

        QJsonObject _obj     = _doc.object();
        QJsonObject _message = _obj["message"].toObject();
        bool        _bDone   = _obj["done"].toBool();

        // ── 检查是否含 tool_calls ─────────────────────
        QJsonArray _toolCalls = _message["tool_calls"].toArray();
        if (!_toolCalls.isEmpty()) {
            mbHasToolCall = true;

            for (const QJsonValue &_tv : _toolCalls) {
                QJsonObject _tc       = _tv.toObject();
                QJsonObject _fn       = _tc["function"].toObject();
                QString     _strName  = _fn["name"].toString();
                QJsonObject _args     = _fn["arguments"].toObject();

                // 把 assistant 的 tool_call 意图写入历史
                QJsonObject _assistantMsg;
                _assistantMsg["role"]       = "assistant";
                _assistantMsg["content"]    = QString();
                _assistantMsg["tool_calls"] = _toolCalls;
                mvMessages.append(_assistantMsg);

                qDebug() << "[AIModule] toolCall:" << _strName << _args;
                emit toolCallRequested(_strName, _args);
            }
        }

        // ── 普通文本 chunk ────────────────────────────
        QString _strChunk = _message["content"].toString();
        if (!_strChunk.isEmpty() && !mbHasToolCall) {
            mstrAssistantBuf += _strChunk;
            emit chunkReceived(_strChunk);
        }

        if (_bDone && !mbHasToolCall) {
            flushAssistantBuffer();
        }
    }
}

// ════════════════════════════════════════════════════════
//  onReplyFinished
// ════════════════════════════════════════════════════════
void AIModule::onReplyFinished()
{
    if (mpCurrentReply == nullptr) { return; }

    // 读取剩余数据（兜底）
    QByteArray _baRemain = mpCurrentReply->readAll().trimmed();
    if (!_baRemain.isEmpty() && !mbHasToolCall) {
        QJsonDocument _doc = QJsonDocument::fromJson(_baRemain);
        if (!_doc.isNull()) {
            QString _strChunk = _doc.object()["message"]
                                .toObject()["content"].toString();
            if (!_strChunk.isEmpty()) {
                mstrAssistantBuf += _strChunk;
                emit chunkReceived(_strChunk);
            }
        }
    }

    if (!mbHasToolCall) {
        flushAssistantBuffer();
        emit replyFinished();
    }
    // tool_call 情况下不 emit replyFinished，
    // 等 submitToolResult → postRequest → 再次 onReplyFinished 时再 emit

    mpCurrentReply->deleteLater();
    mpCurrentReply = nullptr;
}

// ════════════════════════════════════════════════════════
//  onNetworkError
// ════════════════════════════════════════════════════════
void AIModule::onNetworkError(QNetworkReply::NetworkError _eCode)
{
    Q_UNUSED(_eCode)
    if (mpCurrentReply != nullptr) {
        emit errorOccurred(tr("网络错误: %1").arg(mpCurrentReply->errorString()));
    }
}

// ════════════════════════════════════════════════════════
//  flushAssistantBuffer — 把本轮回复写入历史
// ════════════════════════════════════════════════════════
void AIModule::flushAssistantBuffer()
{
    if (mstrAssistantBuf.isEmpty()) { return; }

    QJsonObject _msg;
    _msg["role"]    = "assistant";
    _msg["content"] = mstrAssistantBuf;
    mvMessages.append(_msg);
    mstrAssistantBuf.clear();
}
