// src/core/aimanager.h
#ifndef AIMANAGER_H_A2B3C4D5E6F7
#define AIMANAGER_H_A2B3C4D5E6F7

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>

class OllamaClient;
class ConversationContext;
class ToolCallDispatcher;
class AnalysisService;

class AIManager : public QObject
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数，内部创建 AI 子模块并完成信号连接
     * @param_1 _pParent: 父对象指针
     */
    explicit AIManager(QObject* _pParent = nullptr);

    ~AIManager() override = default;

    /*
     * @brief 注入 AnalysisService，透传给 ToolCallDispatcher
     * @param_1 _pAnalysisService: 空间分析服务指针
     */
    void setAnalysisService(AnalysisService* _pAnalysisService);

    /*
     * @brief 发送用户消息，启动一轮完整对话
     * @param_1 _strUserMsg: 用户输入文本
     */
    void sendMessage(const QString& _strUserMsg);

    /*
     * @brief 清空对话历史
     */
    void clearHistory();

    /*
     * @brief 中断当前流式回复
     */
    void abortCurrentReply();

    /*
     * @brief 使 system prompt 缓存失效
     */
    void invalidateSystemContext();

    /*
     * @brief 将内容保存到记忆文件
     * @param_1 _strContent: 需要写入的 Markdown 内容
     * @param_2 _strSection: 目标节标题
     * @param_3 _bOverwrite: 是否覆盖原节内容
     */
    bool saveMemory(const QString& _strContent,
        const QString& _strSection = QString(),
        bool _bOverwrite = false);

signals:
    void chunkReceived(const QString& _strChunk);
    void replyFinished();
    void toolCallStarted(const QString& _strToolName);
    void toolCallFinished(const QString& _strToolName,
        const QString& _strResult);
    void errorOccurred(const QString& _strError);

private slots:
    void onChunkReceived(const QJsonObject& _chunk);
    void onReplyFinished(const QJsonObject& _finalChunk);
    void onNetworkError(const QString& _strError);
    void onToolExecuted(const QString& _strToolName,
        const QString& _strResult);
    void onToolFailed(const QString& _strToolName,
        const QString& _strError);

private:
    struct ToolCallAccumulator
    {
        QString strName;
        QString strArgsBuf;
    };

    /*
     * @brief 组装请求体并通过 OllamaClient 发起请求
     */
    void postRequest();

    /*
     * @brief 重置本轮流式状态
     */
    void resetStreamState();

    /*
     * @brief 将 tool_calls 数组累积到中间态容器
     * @param_1 _toolCalls: 流式帧中的 tool_calls 数组
     */
    void accumulateToolCalls(const QJsonArray& _toolCalls);

    /*
     * @brief 将累积的 tool_calls 转换为标准数组
     * @param_1 _outToolCalls: 输出的标准数组
     * @param_2 _strError: 失败时的错误描述
     */
    bool flushAccumulators(QJsonArray& _outToolCalls, QString& _strError);

    /*
     * @brief 从标准累积器分发工具调用
     */
    void dispatchFromAccumulators();

    /*
     * @brief 从文本缓冲解析 <tool_call> 并分发
     */
    void dispatchFromTextBuffer();

    OllamaClient*         mpOllamaClient;
    ConversationContext*  mpConversationContext;
    ToolCallDispatcher*   mpToolCallDispatcher;
    QString               mstrAssistantBuf;
    bool                  mbStreaming;
    bool                  mbToolCallTextMode;
    QMap<int, ToolCallAccumulator> mmapAccumulators;
    QJsonArray            mjsonPendingToolCalls;

    static const QString S_STR_MODEL;
};

#endif // AIMANAGER_H_A2B3C4D5E6F7
