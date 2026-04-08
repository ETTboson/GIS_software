// src/core/conversationcontext.h
#ifndef CONVERSATIONCONTEXT_H_E5F6A1B2C3D4
#define CONVERSATIONCONTEXT_H_E5F6A1B2C3D4

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>

#include "core/memory/systemcontextbuilder.h"
#include "core/memory/memoryfilemanager.h"

class ConversationContext : public QObject
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数
     * @param_1 _strWorkDir: 工作目录，用于扫描 CLAUDE.md 与 memory/*.md
     * @param_2 _pParent: 父对象指针，用于 Qt 对象树内存管理
     */
    explicit ConversationContext(const QString& _strWorkDir = QString(),
        QObject* _pParent = nullptr);

    ~ConversationContext() override = default;

    /*
     * @brief 追加用户消息，必要时自动注入温数据记忆
     * @param_1 _strContent: 用户输入文本
     */
    void appendUserMessage(const QString& _strContent);

    /*
     * @brief 追加 assistant 普通文本回复
     * @param_1 _strContent: 模型生成的文本内容
     */
    void appendAssistantMessage(const QString& _strContent);

    /*
     * @brief 追加 assistant tool_call 意图
     * @param_1 _jsonToolCalls: 模型返回的 tool_calls 数组
     */
    void appendToolCallIntent(const QJsonArray& _jsonToolCalls);

    /*
     * @brief 追加工具执行结果
     * @param_1 _strToolName: 工具名称
     * @param_2 _strResult: 工具执行结果文本
     */
    void appendToolResult(const QString& _strToolName,
        const QString& _strResult);

    /*
     * @brief 清空对话历史
     */
    void clear();

    /*
     * @brief 获取裸对话历史列表，不包含 system prompt
     */
    QJsonArray getMessages() const;

    /*
     * @brief 获取用于模型请求的完整消息列表
     *        = system prompt + 对话历史
     */
    QJsonArray getMessagesForRequest();

    /*
     * @brief 返回当前历史中的消息条数
     */
    int messageCount() const;

    /*
     * @brief 将内容写入 CLAUDE.md 记忆文件
     * @param_1 _strContent: 需要写入的 Markdown 内容
     * @param_2 _strSection: 目标节标题，空则直接追加
     * @param_3 _bOverwrite: 是否覆盖指定节内容
     */
    bool saveMemory(const QString& _strContent,
        const QString& _strSection = QString(),
        bool _bOverwrite = false);

    /*
     * @brief 在记忆文件中搜索关键词
     * @param_1 _strKeyword: 搜索关键词，支持简单通配符
     */
    QList<MemorySearchResult> searchMemory(const QString& _strKeyword) const;

    /*
     * @brief 使 system prompt 缓存失效，下次请求时重建
     */
    void invalidateSystemContext();

    /*
     * @brief 返回当前工作目录是否存在记忆文件
     */
    bool hasMemoryFiles() const;

    /*
     * @brief 手动压缩历史消息
     */
    void compactHistory();

    /*
     * @brief 设置触发压缩的最大消息数阈值
     * @param_1 _nMaxMessages: 阈值
     */
    void setMaxMessages(int _nMaxMessages);

    /*
     * @brief 设置压缩后保留的最近消息数
     * @param_1 _nKeepRecentCount: 保留条数
     */
    void setKeepRecentCount(int _nKeepRecentCount);

    /*
     * @brief 设置是否启用温数据记忆注入
     * @param_1 _bEnabled: true 启用，false 关闭
     */
    void setWarmDataInjectionEnabled(bool _bEnabled);

private:
    /*
     * @brief 检查是否需要压缩历史，超阈值时自动执行
     */
    void compactIfNeeded();

    /*
     * @brief 构建被压缩历史的摘要说明
     * @param_1 _jsonDroppedMessages: 被丢弃的消息片段
     */
    QString buildCompactSummary(const QJsonArray& _jsonDroppedMessages) const;

    /*
     * @brief 加载温数据记忆并包装成注入文本
     */
    QString loadWarmDataForInjection();

    SystemContextBuilder mSystemContextBuilder;  // system prompt 构建器
    MemoryFileManager    mMemoryFileManager;     // Claude 风格记忆文件管理器
    QJsonArray           mvMessages;             // 裸对话历史（不含 system）
    int                  mnMaxMessages;          // 历史压缩阈值
    int                  mnKeepRecentCount;      // 压缩后保留最近消息数
    bool                 mbWarmInjectionEnabled; // 是否启用温数据注入
    QString              mstrLastWarmDataHash;   // 上次注入记忆内容的 hash
};

#endif // CONVERSATIONCONTEXT_H_E5F6A1B2C3D4
