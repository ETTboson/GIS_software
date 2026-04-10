#ifndef OLLAMACLIENT_H_D4E5F6A1B2C3
#define OLLAMACLIENT_H_D4E5F6A1B2C3

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

// ════════════════════════════════════════════════════════
//  OllamaClient
//  职责：封装与 Ollama `/api/chat` 的流式 HTTP 通信。
//        负责 NDJSON 分块解析、半包缓冲、中断收尾与网络错误上报，
//        为上层 AIManager 提供稳定的流式消息输入。
//  位于 core/ai/ 层。
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
     * @param_1 _body: 完整请求体
     */
    void postRequest(const QJsonObject& _body);

    /*
     * @brief 中止当前请求，并主动发送结束帧
     */
    void abort();

signals:
    void chunkReceived(const QJsonObject& _chunk);
    void replyFinished(const QJsonObject& _finalChunk);
    void errorOccurred(const QString& _strError);

private slots:
    void onReadyRead();
    void onReplyFinished();
    void onNetworkError(QNetworkReply::NetworkError _eCode);

private:
    /*
     * @brief 处理已读取但尚未解析完成的缓冲
     * @param_1 _bFlushRemaining: 是否在收尾时解析尾包
     */
    void processPendingBuffer(bool _bFlushRemaining);

    /*
     * @brief 解析单行 NDJSON 字节数据为 QJsonObject
     * @param_1 _baLine: 单行原始字节
     */
    QJsonObject parseLine(const QByteArray& _baLine) const;

    QNetworkAccessManager* mpNetManager; // Qt 网络管理器
    QNetworkReply*         mpCurrentReply; // 当前网络请求
    QByteArray             mbaPendingBuffer; // 尚未完成解析的缓冲
    bool                   mbFinalChunkDelivered; // 是否已投递 done:true
    bool                   mbHadNetworkError; // 是否已发生网络错误

    static const QString S_STR_API_URL;
};

#endif // OLLAMACLIENT_H_D4E5F6A1B2C3
