#ifndef TOOLCALLDISPATCHER_H_F6A1B2C3D4E5
#define TOOLCALLDISPATCHER_H_F6A1B2C3D4E5

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>

class ConversationContext;
class IAIToolHost;

// ════════════════════════════════════════════════════════
//  ToolCallDispatcher
//  职责：统一注册 AI 可用工具，并把工具调用分发到宿主桥接或记忆层。
//        在这里集中维护工具 schema、参数校验、错误文本与结果文本，
//        保持 AI 工具接口的一致性。
//  位于 core/ai/ 层。
// ════════════════════════════════════════════════════════
class ToolCallDispatcher : public QObject
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数
     * @param_1 _pParent: 父对象指针
     */
    explicit ToolCallDispatcher(QObject* _pParent = nullptr);

    ~ToolCallDispatcher() override = default;

    /*
     * @brief 注入 ConversationContext
     * @param_1 _pConversationContext: 对话上下文管理器
     */
    void setConversationContext(ConversationContext* _pConversationContext);

    /*
     * @brief 注入宿主桥接接口
     * @param_1 _pToolHost: 宿主接口指针
     */
    void setToolHost(IAIToolHost* _pToolHost);

    /*
     * @brief 根据工具名分发到对应实现
     * @param_1 _strName: 工具名称
     * @param_2 _jsonArgs: 工具参数 JSON
     */
    void dispatch(const QString& _strName, const QJsonObject& _jsonArgs);

    /*
     * @brief 返回全部工具定义列表
     */
    QJsonArray buildToolsDefinition() const;

signals:
    void toolExecuted(const QString& _strToolName, const QString& _strResult);
    void toolFailed(const QString& _strToolName, const QString& _strError);

private:
    /*
     * @brief 执行分析上下文读取工具
     */
    void executeGetAnalysisContext();

    /*
     * @brief 执行分析工具
     * @param_1 _strToolName: 工具名称
     * @param_2 _jsonArgs: 工具参数
     */
    void executeAnalysisTool(const QString& _strToolName,
        const QJsonObject& _jsonArgs);

    /*
     * @brief 执行 save_memory 工具
     * @param_1 _jsonArgs: 工具参数
     */
    void executeSaveMemory(const QJsonObject& _jsonArgs);

    /*
     * @brief 执行 search_memory 工具
     * @param_1 _jsonArgs: 工具参数
     */
    void executeSearchMemory(const QJsonObject& _jsonArgs);

    /*
     * @brief 格式化分析上下文文本
     * @param_1 _jsonContext: 上下文 JSON
     */
    static QString formatAnalysisContext(const QJsonObject& _jsonContext);

    ConversationContext* mpConversationContext; // 对话上下文管理器
    IAIToolHost*         mpToolHost; // 宿主桥接接口
    QString              mstrPendingToolName; // 当前执行中的工具名
};

#endif // TOOLCALLDISPATCHER_H_F6A1B2C3D4E5
