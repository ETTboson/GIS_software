// src/core/ollamaclient.h
#ifndef OLLAMACLIENT_H_D4E5F6A1B2C3
#define OLLAMACLIENT_H_D4E5F6A1B2C3

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

// ════════════════════════════════════════════════════════
//  OllamaClient
//  职责：与 Ollama /api/chat 端点的纯网络通信层
//
//  设计约束：
//    - 不感知 tool_calls、对话历史、工具定义等业务概念
//    - 只接收调用方组装好的完整请求体，原样 POST 发出
//    - 流式响应逐行解析为 QJsonObject 后通过信号向上传递
//    - 每次只允许一个并发请求，新请求到来时中止旧请求
// ════════════════════════════════════════════════════════
class OllamaClient : public QObject
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数，初始化网络管理器
     * @param_1 _pParent: 父对象指针，用于 Qt 对象树内存管理
     */
    explicit OllamaClient(QObject* _pParent = nullptr);

    /*
     * @brief 析构函数，中止并释放当前未完成的网络请求
     */
    ~OllamaClient() override;

    /*
     * @brief 向 Ollama 发起一次流式 POST 请求
     *        若当前已有进行中的请求，先中止再发新请求
     * @param_1 _body: 由调用方组装好的完整请求体 JSON 对象
     *                 应包含 model / messages / tools / stream 等字段
     */
    void postRequest(const QJsonObject& _body);

    /*
     * @brief 中止当前正在进行的网络请求
     *        中止后不会触发 replyFinished，但可能触发 errorOccurred
     */
    void abort();

signals:
    /*
     * @brief 每收到一行流式 NDJSON 数据时触发
     *        调用方需自行判断其中是否含 tool_calls 或普通文本
     * @param_1 _chunk: 解析后的单条消息 JSON 对象（对应 /api/chat 的一行响应）
     */
    void chunkReceived(const QJsonObject& _chunk);

    /*
     * @brief 流式响应完全结束（done:true）时触发
     *        携带最后一条 JSON 对象，可能含 tool_calls 等最终定性信息
     * @param_1 _finalChunk: done:true 对应的那条 JSON 对象
     */
    void replyFinished(const QJsonObject& _finalChunk);

    /*
     * @brief 网络层发生错误时触发
     * @param_1 _strError: 错误描述字符串
     */
    void errorOccurred(const QString& _strError);

private slots:
    /*
     * @brief QNetworkReply 有数据可读时触发，逐行解析 NDJSON
     */
    void onReadyRead();

    /*
     * @brief QNetworkReply 请求完成时触发，处理尾部残留数据
     */
    void onReplyFinished();

    /*
     * @brief QNetworkReply 网络错误时触发
     * @param_1 _eCode: Qt 网络错误枚举值
     */
    void onNetworkError(QNetworkReply::NetworkError _eCode);

private:
    /*
     * @brief 解析单行 NDJSON 字节数据为 QJsonObject
     *        解析失败时返回空 QJsonObject 并打印警告
     * @param_1 _baLine: 从网络流中读取的单行原始字节
     */
    QJsonObject parseLine(const QByteArray& _baLine) const;

    // ── 网络 ──────────────────────────────────────────
    QNetworkAccessManager* mpNetManager;    // Qt 网络管理器，负责发起 HTTP 请求
    QNetworkReply* mpCurrentReply;  // 当前正在进行的网络响应，空闲时为 nullptr

    // ── 配置 ──────────────────────────────────────────
    static const QString   S_STR_API_URL;  // Ollama API 端点地址
};

#endif // OLLAMACLIENT_H_D4E5F6A1B2C3