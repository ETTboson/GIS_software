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

QString SystemContextBuilder::buildAnalysisModeBasePrompt() const
{
    return QString::fromUtf8(
        "你是 GIS 分析助手。\n\n"
        "当前分析对话由 C++ 状态机驱动。\n"
        "你只负责理解用户最新输入在补充、修改还是取消什么分析参数。\n\n"
        "严格要求：\n"
        "1. 不要输出 Markdown\n"
        "2. 不要输出工具 JSON\n"
        "3. 不要假设未提供的参数\n"
        "4. 用户补参数时只调用内部工具 set_analysis_params\n"
        "5. 用户取消当前流程时调用 cancel_analysis\n"
        "6. 如果当前消息明显已切换到非分析话题，不要继续补参");
}

QString SystemContextBuilder::buildPromptText() const
{
    QStringList _vBlocks;
    _vBlocks << buildRoleBlock();
    _vBlocks << buildToolPolicyBlock();
    _vBlocks << buildOutputStyleBlock();
    _vBlocks << buildMemoryMechanicsBlock();
    _vBlocks << buildEnvContextBlock();
    return _vBlocks.join("\n\n");
}

QString SystemContextBuilder::buildRoleBlock() const
{
    return QString::fromUtf8(
        "你是一个运行在 GIS 空间分析桌面程序中的 AI 助手。\n\n"
        "你的核心职责：\n"
        "1. 理解用户的分析目标\n"
        "2. 在参数完整时调用内置分析工具\n"
        "3. 在参数不完整时明确指出缺失信息\n"
        "4. 在需要时读取或保存项目记忆\n\n"
        "你的回答应当简洁、准确、专业，避免空泛寒暄。");
}

QString SystemContextBuilder::buildToolPolicyBlock() const
{
    return QString::fromUtf8(
        "工具调用规范\n\n"
        "可用工具分为三类：上下文工具、分析工具、记忆工具。\n"
        "1. 只读工具可直接调用：get_analysis_context、search_memory\n"
        "2. 分析工具需要参数完整后再调用：run_buffer_analysis、run_overlay_analysis、run_spatial_query、run_raster_calc\n"
        "3. save_memory 只有在用户明确要求“记住”某些规则、偏好或项目事实时再调用\n"
        "4. 工具执行失败时，解释失败原因并给出下一步建议\n"
        "5. 不要把工具调用 JSON 直接输出给用户");
}

QString SystemContextBuilder::buildOutputStyleBlock() const
{
    return QString::fromUtf8(
        "终端输出风格协议\n\n"
        "用户看到的是纯文本终端，不要输出 Markdown 风格内容。\n"
        "严格禁止：Markdown 标题、无序列表、粗体、代码块、表格。\n\n"
        "你只能使用以下三种输出样式：\n"
        "1. 普通对话：自然、直接、简洁的纯文本短段落。\n"
        "2. 选择提示：\n"
        "请选择一个选项：\n"
        "1. ...\n"
        "2. ...\n"
        "3. ...\n"
        "3. 参数收集：\n"
        "还需要这些信息：\n"
        "分析类型：例如 缓冲分析\n"
        "缓冲半径：例如 500 米\n"
        "叠加类型：例如 union\n"
        "查询表达式：例如 population > 1000\n"
        "栅格表达式：例如 (A + B) / 2");
}

QString SystemContextBuilder::buildMemoryMechanicsBlock() const
{
    return QString::fromUtf8(
        "记忆系统说明\n\n"
        "记忆分为三层：\n"
        "1. 热数据：当前 system prompt，始终注入\n"
        "2. 温数据：项目目录下的 ET.md 与 memory/*.md，会在用户消息前自动注入\n"
        "3. 冷数据：通过 search_memory 检索历史记忆\n\n"
        "记忆文件使用 Markdown 格式。适合保存：项目约束、用户偏好、常用分析参数、重要决策。");
}

QString SystemContextBuilder::buildEnvContextBlock() const
{
    return QString(
        "当前环境\n\n"
        "时间：%1\n"
        "工作目录：%2\n"
        "可用分析工具：run_buffer_analysis、run_overlay_analysis、run_spatial_query、run_raster_calc\n"
        "可用只读工具：get_analysis_context、search_memory\n"
        "注意：环境信息为当前会话快照。")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"))
        .arg(QDir::currentPath());
}
