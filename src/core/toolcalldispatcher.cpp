// src/core/toolcalldispatcher.cpp
#include "toolcalldispatcher.h"
#include "service/analysisservice.h"
#include "analysisresult.h"
#include "conversationcontext.h"

#include <QDebug>

ToolCallDispatcher::ToolCallDispatcher(QObject* _pParent)
    : QObject(_pParent)
    , mpAnalysisService(nullptr)
    , mpConversationContext(nullptr)
{
}

void ToolCallDispatcher::setAnalysisService(AnalysisService* _pAnalysisService)
{
    if (mpAnalysisService != nullptr) {
        disconnect(mpAnalysisService, nullptr, this, nullptr);
    }

    mpAnalysisService = _pAnalysisService;
    if (mpAnalysisService == nullptr) {
        return;
    }

    connect(mpAnalysisService, &AnalysisService::analysisFinished,
        this, &ToolCallDispatcher::onAnalysisFinished);
    connect(mpAnalysisService, &AnalysisService::analysisFailed,
        this, &ToolCallDispatcher::onAnalysisFailed);
}

void ToolCallDispatcher::setConversationContext(
    ConversationContext* _pConversationContext)
{
    mpConversationContext = _pConversationContext;
}

void ToolCallDispatcher::dispatch(const QString& _strName,
    const QJsonObject& _args)
{
    mstrPendingToolName = _strName;

    if (_strName == "run_buffer_analysis") {
        if (mpAnalysisService == nullptr) {
            emit toolFailed(_strName, tr("错误：AnalysisService 未初始化"));
            return;
        }

        const double _dRadius = _args["radius_meters"].toDouble(0.0);
        if (_dRadius <= 0.0) {
            emit toolFailed(_strName, tr("参数错误：缓冲半径必须大于 0"));
            return;
        }
        mpAnalysisService->runBufferAnalysis(_dRadius);
    } else if (_strName == "run_overlay_analysis") {
        if (mpAnalysisService == nullptr) {
            emit toolFailed(_strName, tr("错误：AnalysisService 未初始化"));
            return;
        }

        const QString _strType = _args["overlay_type"].toString().trimmed();
        if (_strType.isEmpty()) {
            emit toolFailed(_strName, tr("参数错误：叠加类型不能为空"));
            return;
        }
        mpAnalysisService->runOverlayAnalysis(_strType);
    } else if (_strName == "run_spatial_query") {
        if (mpAnalysisService == nullptr) {
            emit toolFailed(_strName, tr("错误：AnalysisService 未初始化"));
            return;
        }

        const QString _strExpr = _args["query_expression"].toString().trimmed();
        if (_strExpr.isEmpty()) {
            emit toolFailed(_strName, tr("参数错误：查询表达式不能为空"));
            return;
        }
        mpAnalysisService->runSpatialQuery(_strExpr);
    } else if (_strName == "run_raster_calc") {
        if (mpAnalysisService == nullptr) {
            emit toolFailed(_strName, tr("错误：AnalysisService 未初始化"));
            return;
        }

        const QString _strExpr = _args["expression"].toString().trimmed();
        if (_strExpr.isEmpty()) {
            emit toolFailed(_strName, tr("参数错误：计算表达式不能为空"));
            return;
        }
        mpAnalysisService->runRasterCalc(_strExpr);
    } else if (_strName == "save_memory") {
        executeSaveMemory(_args);
    } else if (_strName == "search_memory") {
        executeSearchMemory(_args);
    } else {
        emit toolFailed(_strName, tr("未知工具：%1").arg(_strName));
    }
}

QJsonArray ToolCallDispatcher::buildToolsDefinition() const
{
    QJsonArray _tools;

    {
        QJsonObject _radiusProp;
        _radiusProp["type"] = "number";
        _radiusProp["description"] = "缓冲区半径，单位米";

        QJsonObject _properties;
        _properties["radius_meters"] = _radiusProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;
        _params["required"] = QJsonArray{ "radius_meters" };

        QJsonObject _fn;
        _fn["name"] = "run_buffer_analysis";
        _fn["description"] = "对当前已加载图层执行缓冲区分析。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _tools.append(_tool);
    }

    {
        QJsonObject _typeProp;
        _typeProp["type"] = "string";
        _typeProp["description"] = "叠加类型：intersection、union、difference";
        _typeProp["enum"] = QJsonArray{ "intersection", "union", "difference" };

        QJsonObject _properties;
        _properties["overlay_type"] = _typeProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;
        _params["required"] = QJsonArray{ "overlay_type" };

        QJsonObject _fn;
        _fn["name"] = "run_overlay_analysis";
        _fn["description"] = "对两个已加载图层执行叠加分析。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _tools.append(_tool);
    }

    {
        QJsonObject _exprProp;
        _exprProp["type"] = "string";
        _exprProp["description"] = "空间查询表达式";

        QJsonObject _properties;
        _properties["query_expression"] = _exprProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;
        _params["required"] = QJsonArray{ "query_expression" };

        QJsonObject _fn;
        _fn["name"] = "run_spatial_query";
        _fn["description"] = "对当前图层执行空间查询。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _tools.append(_tool);
    }

    {
        QJsonObject _exprProp;
        _exprProp["type"] = "string";
        _exprProp["description"] = "栅格计算表达式";

        QJsonObject _properties;
        _properties["expression"] = _exprProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;
        _params["required"] = QJsonArray{ "expression" };

        QJsonObject _fn;
        _fn["name"] = "run_raster_calc";
        _fn["description"] = "执行栅格计算。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _tools.append(_tool);
    }

    {
        QJsonObject _contentProp;
        _contentProp["type"] = "string";
        _contentProp["description"] = "要写入记忆文件的 Markdown 内容";

        QJsonObject _sectionProp;
        _sectionProp["type"] = "string";
        _sectionProp["description"] = "目标节标题，可选";

        QJsonObject _overwriteProp;
        _overwriteProp["type"] = "boolean";
        _overwriteProp["description"] = "是否覆盖该节内容";

        QJsonObject _properties;
        _properties["content"] = _contentProp;
        _properties["section"] = _sectionProp;
        _properties["overwrite"] = _overwriteProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;
        _params["required"] = QJsonArray{ "content" };

        QJsonObject _fn;
        _fn["name"] = "save_memory";
        _fn["description"] = "将重要项目事实、偏好或规则保存到 CLAUDE.md。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _tools.append(_tool);
    }

    {
        QJsonObject _keywordProp;
        _keywordProp["type"] = "string";
        _keywordProp["description"] = "记忆检索关键词，支持简单通配符";

        QJsonObject _properties;
        _properties["keyword"] = _keywordProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;
        _params["required"] = QJsonArray{ "keyword" };

        QJsonObject _fn;
        _fn["name"] = "search_memory";
        _fn["description"] = "在 CLAUDE.md 和 memory/*.md 中检索历史记忆。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _tools.append(_tool);
    }

    return _tools;
}

void ToolCallDispatcher::onAnalysisFinished(const AnalysisResult& _result)
{
    emit toolExecuted(mstrPendingToolName, _result.strDesc);
    mstrPendingToolName.clear();
}

void ToolCallDispatcher::onAnalysisFailed(const AnalysisResult& _result)
{
    emit toolFailed(mstrPendingToolName, _result.strDesc);
    mstrPendingToolName.clear();
}

void ToolCallDispatcher::executeSaveMemory(const QJsonObject& _args)
{
    if (mpConversationContext == nullptr) {
        emit toolFailed(mstrPendingToolName, tr("内部错误：ConversationContext 未注入"));
        return;
    }

    const QString _strContent = _args["content"].toString().trimmed();
    const QString _strSection = _args["section"].toString().trimmed();
    const bool _bOverwrite = _args["overwrite"].toBool(false);

    if (_strContent.isEmpty()) {
        emit toolFailed(mstrPendingToolName, tr("参数错误：content 不能为空"));
        return;
    }

    if (!mpConversationContext->saveMemory(_strContent, _strSection, _bOverwrite)) {
        emit toolFailed(mstrPendingToolName, tr("写入记忆文件失败，请检查文件权限"));
        return;
    }

    const QString _strResult = _strSection.isEmpty()
        ? tr("已将内容写入 CLAUDE.md")
        : tr("已将内容写入 CLAUDE.md 的 [%1] 节").arg(_strSection);
    emit toolExecuted(mstrPendingToolName, _strResult);
    mstrPendingToolName.clear();
}

void ToolCallDispatcher::executeSearchMemory(const QJsonObject& _args)
{
    if (mpConversationContext == nullptr) {
        emit toolFailed(mstrPendingToolName, tr("内部错误：ConversationContext 未注入"));
        return;
    }

    const QString _strKeyword = _args["keyword"].toString().trimmed();
    if (_strKeyword.isEmpty()) {
        emit toolFailed(mstrPendingToolName, tr("参数错误：keyword 不能为空"));
        return;
    }

    const QList<MemorySearchResult> _vResults =
        mpConversationContext->searchMemory(_strKeyword);
    emit toolExecuted(mstrPendingToolName,
        MemoryFileManager::formatSearchResults(_vResults, _strKeyword));
    mstrPendingToolName.clear();
}
