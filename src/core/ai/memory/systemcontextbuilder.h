#ifndef SYSTEMCONTEXTBUILDER_H_C1D2E3F4A5B6
#define SYSTEMCONTEXTBUILDER_H_C1D2E3F4A5B6

#include <QString>
#include <QJsonObject>

// ════════════════════════════════════════════════════════
//  SystemContextBuilder
//  职责：统一构造 AI system prompt。
//        负责拼装角色说明、工具策略、输出风格、记忆机制与环境块，
//        同时提供分析状态更新链使用的轻量 prompt。
//  位于 core/ai/memory/ 层。
// ════════════════════════════════════════════════════════
class SystemContextBuilder
{
public:
    SystemContextBuilder()
        : mbBuilt(false)
    {
    }

    /*
     * @brief 构建或返回缓存的 system prompt
     */
    QJsonObject buildOnce();

    /*
     * @brief 使缓存失效，下次 buildOnce() 时重建
     */
    void invalidate();

    /*
     * @brief 返回分析状态更新模式的轻量 prompt
     */
    QString buildAnalysisModeBasePrompt() const;

private:
    QString buildPromptText() const;
    QString buildRoleBlock() const;
    QString buildToolPolicyBlock() const;
    QString buildOutputStyleBlock() const;
    QString buildMemoryMechanicsBlock() const;
    QString buildEnvContextBlock() const;

    bool    mbBuilt; // 是否已构建缓存
    QString mstrCachedPrompt; // 已缓存的 prompt 文本
};

#endif // SYSTEMCONTEXTBUILDER_H_C1D2E3F4A5B6
