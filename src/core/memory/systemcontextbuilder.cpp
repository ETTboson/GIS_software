// src/core/memory/systemcontextbuilder.cpp
#include "systemcontextbuilder.h"

#include <QDateTime>
#include <QDir>
#include <QStringList>

QJsonObject SystemContextBuilder::buildOnce()
{
    if (!mbBuilt) {
        mstrCachedPrompt = buildPromptText();
        mbBuilt = true;
    }

    QJsonObject _jsonObj;
    _jsonObj["role"] = "system";
    _jsonObj["content"] = mstrCachedPrompt;
    return _jsonObj;
}

void SystemContextBuilder::invalidate()
{
    mbBuilt = false;
    mstrCachedPrompt.clear();
}

QString SystemContextBuilder::buildPromptText() const
{
    QStringList _vBlocks;
    _vBlocks << buildRoleBlock();
    _vBlocks << buildToolPolicyBlock();
    _vBlocks << buildMemoryMechanicsBlock();
    _vBlocks << buildEnvContextBlock();
    return _vBlocks.join("\n\n");
}

QString SystemContextBuilder::buildRoleBlock() const
{
    return QString::fromUtf8(
        "你是一个运行在 GIS 空间分析桌面程序中的 AI 助手，"
        "交互风格参考 Claude Code CLI。\n\n"
        "你的回答要求：\n"
        "- 简洁、直接、专业\n"
        "- 优先给出可执行结论，不写空泛寒暄\n"
        "- 当信息不足以调用工具时，先指出缺失参数并提问\n"
        "- 当工具已经足够执行时，直接调用工具，不要重复征求许可");
}

QString SystemContextBuilder::buildToolPolicyBlock() const
{
    return QString::fromUtf8(
        "## 工具调用规范\n\n"
        "可用工具分为两类：空间分析工具与记忆工具。\n"
        "- 空间分析工具需要参数完整后再调用\n"
        "- 参数不完整时，明确指出缺失项\n"
        "- 工具执行失败时，解释失败原因并给出下一步建议\n"
        "- 若记忆文件中可能存在历史偏好或参数，可使用 search_memory 检索\n"
        "- 当用户明确要求“记住”某些规则、偏好或项目事实时，可使用 save_memory");
}

QString SystemContextBuilder::buildMemoryMechanicsBlock() const
{
    return QString::fromUtf8(
        "## 记忆系统说明\n\n"
        "记忆分为三层：\n"
        "1. 热数据：当前 system prompt，始终注入\n"
        "2. 温数据：项目目录下的 CLAUDE.md 与 memory/*.md，会在用户消息前自动注入\n"
        "3. 冷数据：通过 search_memory 检索历史记忆\n\n"
        "记忆文件使用 Markdown 格式。适合保存：项目约束、用户偏好、常用分析参数、重要决策。");
}

QString SystemContextBuilder::buildEnvContextBlock() const
{
    return QString(
        "## 当前环境\n\n"
        "时间：%1\n"
        "工作目录：%2\n"
        "可用空间分析工具：run_buffer_analysis、run_overlay_analysis、run_spatial_query、run_raster_calc\n"
        "注意：环境信息为当前会话快照。")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"))
        .arg(QDir::currentPath());
}
