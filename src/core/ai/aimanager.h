#ifndef AIMANAGER_H_A2B3C4D5E6F7
#define AIMANAGER_H_A2B3C4D5E6F7

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>

#include "core/workflow/analysisworkflowcoordinator.h"
#include "core/ai/memory/systemcontextbuilder.h"

class OllamaClient;
class ConversationContext;
class ToolCallDispatcher;
class IAIToolHost;

// ════════════════════════════════════════════════════════
//  AIManager
//  职责：AI 模块总协调器。
//        统一管理对话上下文、模型请求、tool_call 分发、分析状态更新链，
//        是 UI 与底层 AI 子模块之间的唯一编排入口。
//  位于 core/ai/ 层。
// ════════════════════════════════════════════════════════
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
     * @brief 注入宿主桥接接口
     * @param_1 _pToolHost: 宿主接口指针
     */
    void setToolHost(IAIToolHost* _pToolHost);

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
    enum class RequestMode
    {
        General,
        AnalysisStateUpdate
    };

    struct ToolCallAccumulator
    {
        QString strName; // 工具名称
        QString strArgsBuf; // 参数缓冲
    };

    void postRequest();
    void resetStreamState();
    void accumulateToolCalls(const QJsonArray& _toolCalls);
    bool flushAccumulators(QJsonArray& _outToolCalls, QString& _strError);
    void dispatchFromAccumulators();
    void dispatchFromTextBuffer();
    void dispatchToolCall(const QString& _strToolName,
        const QJsonObject& _jsonArgs,
        bool _bRecordIntent);
    void startAnalysisStateUpdateRound();
    void handleAnalysisStateUpdateTool(const QString& _strToolName,
        const QJsonObject& _jsonArgs);
    void finishWithAssistantText(const QString& _strText);
    void resetAnalysisMode();

    OllamaClient*               mpOllamaClient; // 网络通信层
    ConversationContext*        mpConversationContext; // 对话上下文
    ToolCallDispatcher*         mpToolCallDispatcher; // 工具分发层
    AnalysisWorkflowCoordinator mAnalysisWorkflowCoordinator; // 分析工作流状态机
    SystemContextBuilder        mAnalysisPromptBuilder; // 轻量 prompt 构建器
    QString                     mstrAssistantBuf; // 本轮助手文本缓冲
    bool                        mbStreaming; // 是否处于流式接收状态
    bool                        mbToolCallTextMode; // 是否进入文本 tool_call 模式
    QMap<int, ToolCallAccumulator> mmapAccumulators; // tool_call 累积器
    QJsonArray                  mjsonPendingToolCalls; // 待处理工具调用
    RequestMode                 mRequestMode; // 当前请求模式
    QString                     mstrLatestUserMessage; // 最近一次用户输入

    static const QString S_STR_MODEL;
};

#endif // AIMANAGER_H_A2B3C4D5E6F7
