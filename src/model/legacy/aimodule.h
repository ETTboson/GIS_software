// src/model/legacy/aimodule.h
#ifndef AIMODULE_H_E5F6A1B2C3D4
#define AIMODULE_H_E5F6A1B2C3D4

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// ════════════════════════════════════════════════════════
//  AIModule
//  职责：与 Ollama /api/chat 通信，管理多轮上下文
//        支持 function calling（tools）
//
//  工具调用流程：
//    1. 每次请求携带 tools 定义列表
//    2. 模型返回 tool_calls → 解析后 emit toolCallRequested
//    3. 外部执行工具 → 调用 submitToolResult() 把结果回传
//    4. AIModule 自动继续请求，拿到最终自然语言回复
//
//  新增工具：在 buildToolsDefinition() 里加一条 JSON 对象即可
// ════════════════════════════════════════════════════════
class AIModule : public QObject
{
    Q_OBJECT

public:
    explicit AIModule(QObject *_pParent = nullptr);
    ~AIModule();

    // 发送用户消息
    void sendMessage(const QString &_strUserMsg);

    // 工具执行完毕后，把结果回传给 AI 继续对话
    // _strToolName : 与 toolCallRequested 里的 name 一致
    // _strResult   : 工具执行结果的文字描述
    void submitToolResult(const QString &_strToolName,
                          const QString &_strResult);

    // 清空对话历史
    void clearHistory();

signals:
    // 普通流式文本 chunk
    void chunkReceived(const QString &_strChunk);

    // 本轮普通对话完成
    void replyFinished();

    // 模型请求调用工具
    // _strName : 工具名（如 "run_buffer_analysis"）
    // _args    : 工具参数 JSON 对象（如 {"radius_meters": 500}）
    void toolCallRequested(const QString &_strName,
                           const QJsonObject &_args);

    // 错误
    void errorOccurred(const QString &_strError);

private slots:
    void onReadyRead();
    void onReplyFinished();
    void onNetworkError(QNetworkReply::NetworkError _eCode);

private:
    // 构造工具定义列表（新增工具在此函数里添加）
    QJsonArray buildToolsDefinition() const;

    // 发起 HTTP 请求（sendMessage 和 submitToolResult 都走这里）
    void postRequest();

    // 将本轮 assistant 内容追加进历史
    void flushAssistantBuffer();

    // ── 网络 ──────────────────────────────────────────
    QNetworkAccessManager  *mpNetManager;
    QNetworkReply          *mpCurrentReply;

    // ── 对话历史 ──────────────────────────────────────
    QJsonArray              mvMessages;

    // ── 本轮 assistant 回复缓冲 ───────────────────────
    QString                 mstrAssistantBuf;

    // ── tool_call 解析状态 ────────────────────────────
    // 流式响应里 tool_calls 可能分多个 chunk 到达，需要拼接
    QString                 mstrToolCallBuf;
    bool                    mbHasToolCall;      // 本轮是否含 tool_call

    // ── 配置 ──────────────────────────────────────────
    static const QString    S_STR_API_URL;
    static const QString    S_STR_MODEL;
};

#endif // AIMODULE_H_E5F6A1B2C3D4
