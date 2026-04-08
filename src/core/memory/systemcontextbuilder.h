// src/core/memory/systemcontextbuilder.h
#ifndef SYSTEMCONTEXTBUILDER_H_C1D2E3F4A5B6
#define SYSTEMCONTEXTBUILDER_H_C1D2E3F4A5B6

#include <QString>
#include <QJsonObject>

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

private:
    /*
     * @brief 组装完整的 Claude Code CLI 风格 system prompt 文本
     */
    QString buildPromptText() const;

    /*
     * @brief 构建角色与输出风格说明
     */
    QString buildRoleBlock() const;

    /*
     * @brief 构建工具调用规范说明
     */
    QString buildToolPolicyBlock() const;

    /*
     * @brief 构建记忆系统说明
     */
    QString buildMemoryMechanicsBlock() const;

    /*
     * @brief 构建当前环境上下文说明
     */
    QString buildEnvContextBlock() const;

    bool    mbBuilt;
    QString mstrCachedPrompt;
};

#endif // SYSTEMCONTEXTBUILDER_H_C1D2E3F4A5B6
