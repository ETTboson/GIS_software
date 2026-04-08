// src/core/toolcalldispatcher.h
#ifndef TOOLCALLDISPATCHER_H_F6A1B2C3D4E5
#define TOOLCALLDISPATCHER_H_F6A1B2C3D4E5

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include "analysisresult.h"

class AnalysisService;
class ConversationContext;

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
     * @brief 注入 AnalysisService
     * @param_1 _pAnalysisService: 空间分析服务指针
     */
    void setAnalysisService(AnalysisService* _pAnalysisService);

    /*
     * @brief 注入 ConversationContext
     * @param_1 _pConversationContext: 对话上下文管理器
     */
    void setConversationContext(ConversationContext* _pConversationContext);

    /*
     * @brief 根据工具名分发到对应实现
     * @param_1 _strName: 工具名称
     * @param_2 _args: 工具参数 JSON
     */
    void dispatch(const QString& _strName, const QJsonObject& _args);

    /*
     * @brief 返回全部工具定义列表
     */
    QJsonArray buildToolsDefinition() const;

signals:
    void toolExecuted(const QString& _strToolName, const QString& _strResult);
    void toolFailed(const QString& _strToolName, const QString& _strError);

private slots:
    void onAnalysisFinished(const AnalysisResult& _result);
    void onAnalysisFailed(const AnalysisResult& _result);

private:
    /*
     * @brief 执行 save_memory 工具
     */
    void executeSaveMemory(const QJsonObject& _args);

    /*
     * @brief 执行 search_memory 工具
     */
    void executeSearchMemory(const QJsonObject& _args);

    AnalysisService*    mpAnalysisService;
    ConversationContext* mpConversationContext;
    QString             mstrPendingToolName;
};

#endif // TOOLCALLDISPATCHER_H_F6A1B2C3D4E5
