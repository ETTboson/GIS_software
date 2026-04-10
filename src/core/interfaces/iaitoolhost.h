#ifndef IAITOOLHOST_H_E1F2A3B4C5D6
#define IAITOOLHOST_H_E1F2A3B4C5D6

#include <QString>
#include <QJsonObject>

// ════════════════════════════════════════════════════════
//  IAIToolHost
//  职责：定义 AI 与宿主程序之间的最小桥接接口。
//        AI 侧只通过该接口读取分析上下文、触发分析执行，
//        不直接依赖 MainWindow 或具体 service 实现。
//  位于 core/interfaces/ 层。
// ════════════════════════════════════════════════════════
class IAIToolHost
{
public:
    /*
     * @brief 虚析构函数
     */
    virtual ~IAIToolHost() = default;

    /*
     * @brief 返回当前分析上下文快照
     */
    virtual QJsonObject getAnalysisContext() const = 0;

    /*
     * @brief 执行 AI 发起的分析工具调用
     * @param_1 _strToolName: 工具名称
     * @param_2 _jsonArgs: 工具参数
     * @param_3 _strResult: 成功时的人类可读结果
     * @param_4 _strError: 失败时的错误文本
     */
    virtual bool executeAnalysisTool(const QString& _strToolName,
        const QJsonObject& _jsonArgs,
        QString& _strResult,
        QString& _strError) = 0;
};

#endif // IAITOOLHOST_H_E1F2A3B4C5D6
