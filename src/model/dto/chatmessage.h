// src/model/dto/chatmessage.h
#ifndef CHATMESSAGE_H_B2C3D4E5F6A1
#define CHATMESSAGE_H_B2C3D4E5F6A1

#include <QString>
#include <QJsonArray>

// ════════════════════════════════════════════════════════
//  ChatMessage
//  单条对话消息数据传输对象（DTO）
//  在 ConversationContext、AIManager、AIDockWidget 之间流转
//
//  role 取值约定：
//    "user"      — 用户输入
//    "assistant" — 模型回复（可能含 tool_calls）
//    "tool"      — 工具执行结果回传
// ════════════════════════════════════════════════════════
struct ChatMessage
{
    QString    strRole;        // 消息角色，见上方取值约定
    QString    strContent;     // 消息正文；tool_call 意图时此字段为空字符串
    QJsonArray jsonToolCalls;  // 模型请求调用的工具列表；非 tool_call 消息时为空数组
};

#endif // CHATMESSAGE_H_B2C3D4E5F6A1
