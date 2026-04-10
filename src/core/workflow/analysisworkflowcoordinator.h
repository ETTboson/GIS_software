#ifndef ANALYSISWORKFLOWCOORDINATOR_H_A7B8C9D0E1F2
#define ANALYSISWORKFLOWCOORDINATOR_H_A7B8C9D0E1F2

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

class IAIToolHost;

// ════════════════════════════════════════════════════════
//  AnalysisWorkflowCoordinator
//  职责：维护分析工作流状态机。
//        负责识别分析类型、收集缺失参数、构建状态更新请求，
//        并在参数完整后给出执行转移。
//  位于 core/workflow/ 层，与具体 UI 解耦。
// ════════════════════════════════════════════════════════
class AnalysisWorkflowCoordinator
{
public:
    enum class AnalysisTaskType
    {
        None,
        Buffer,
        Overlay,
        SpatialQuery,
        RasterCalc
    };

    enum class AnalysisTurnKind
    {
        IntentOnly,
        StateUpdate,
        Cancel,
        ExitAnalysisMode
    };

    enum class TransitionKind
    {
        GenerateReply,
        ExecuteTool,
        FinishWithText,
        Error
    };

    struct TransitionResult
    {
        TransitionKind kind = TransitionKind::GenerateReply;
        QString        strToolName;
        QJsonObject    jsonToolArgs;
        QString        strFinalText;
        QString        strError;
    };

    AnalysisWorkflowCoordinator() = default;

    /*
     * @brief 注入宿主桥接接口
     * @param_1 _pToolHost: 宿主接口指针
     */
    void setToolHost(IAIToolHost* _pToolHost);

    /*
     * @brief 清空当前分析状态
     */
    void reset();

    /*
     * @brief 当前是否处于分析工作流中
     */
    bool isAnalysisModeActive() const;

    /*
     * @brief 判断输入是否应该进入分析工作流
     * @param_1 _strUserText: 用户输入
     */
    bool shouldEnterAnalysisMode(const QString& _strUserText) const;

    /*
     * @brief 对当前输入做轮次分类
     * @param_1 _strUserText: 用户输入
     */
    AnalysisTurnKind classifyTurn(const QString& _strUserText) const;

    /*
     * @brief 更新当前轮输入并推断任务类型
     * @param_1 _strUserText: 用户输入
     */
    void beginOrContinue(const QString& _strUserText);

    /*
     * @brief 构造确定性回复
     */
    QString buildDeterministicReply() const;

    /*
     * @brief 构造缺参回复
     */
    QString buildNeedMoreInfoReply() const;

    /*
     * @brief 构造状态更新模式下的内部工具定义
     */
    QJsonArray buildStateUpdateToolsDefinition() const;

    /*
     * @brief 构造状态更新模式下的请求消息
     * @param_1 _strBasePrompt: 轻量 system prompt
     * @param_2 _jsonRawHistory: 裸历史
     * @param_3 _strLatestUserText: 最新用户消息
     */
    QJsonArray buildStateUpdateMessages(const QString& _strBasePrompt,
        const QJsonArray& _jsonRawHistory,
        const QString& _strLatestUserText) const;

    /*
     * @brief 应用内部状态更新工具
     * @param_1 _strToolName: 内部工具名
     * @param_2 _jsonArgs: 工具参数
     */
    TransitionResult applyStateUpdateTool(const QString& _strToolName,
        const QJsonObject& _jsonArgs);

    /*
     * @brief 处理用户直接取消的动作
     * @param_1 _strUserText: 用户输入
     */
    TransitionResult handleDirectUserAction(const QString& _strUserText);

    /*
     * @brief 尝试在本地解析用户补参输入
     * @param_1 _strUserText: 用户输入
     * @param_2 _outTransition: 输出转移结果
     */
    bool tryApplyLocalStateUpdate(const QString& _strUserText,
        TransitionResult& _outTransition);

private:
    struct AnalysisWorkflowState
    {
        AnalysisTaskType type = AnalysisTaskType::None; // 当前分析任务类型
        bool bActive = false;                           // 是否处于分析工作流
        bool bHasRadius = false;                        // 是否已有缓冲半径
        double dRadiusMeters = 0.0;                     // 缓冲半径参数
        QString strOverlayType;                         // 叠加分析类型
        QString strQueryExpression;                     // 空间查询表达式
        QString strRasterExpression;                    // 栅格计算表达式
        QStringList vMissingParams;                     // 当前缺失参数列表
        QString strLastError;                           // 最近错误文本
    };

    void inferTaskTypeFromText(const QString& _strUserText);
    void recalculateMissingParams();
    TransitionResult buildExecutionTransition() const;
    TransitionResult applyStateUpdateArgs(const QJsonObject& _jsonArgs);
    QJsonArray extractRecentDialogue(const QJsonArray& _jsonRawHistory,
        const QString& _strLatestUserText) const;
    QString buildStateSnapshot(const QString& _strLatestUserText) const;
    QString buildCollectedParamLines() const;
    QString buildMissingParamLines() const;
    QString currentToolName() const;
    static QString normalizeOverlayType(const QString& _strValue);
    static QString normalizeExpression(const QString& _strValue);
    static bool isCancelText(const QString& _strText);
    static bool isCapabilityIntent(const QString& _strText);
    bool looksLikeStateUpdateInput(const QString& _strText) const;
    static AnalysisTaskType detectTaskType(const QString& _strUserText);
    static bool tryExtractRadiusMeters(const QString& _strText,
        double& _dRadiusMeters);
    static bool tryExtractQueryExpression(const QString& _strText,
        QString& _strExpression);
    static bool tryExtractRasterExpression(const QString& _strText,
        QString& _strExpression);

    AnalysisWorkflowState mState; // 当前分析工作流状态
    IAIToolHost*          mpToolHost = nullptr; // 宿主桥接接口
    QString               mstrLastUserText; // 最近一次用户输入
};

#endif // ANALYSISWORKFLOWCOORDINATOR_H_A7B8C9D0E1F2
