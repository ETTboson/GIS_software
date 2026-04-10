#include "toolcalldispatcher.h"

#include "core/ai/conversationcontext.h"
#include "core/interfaces/iaitoolhost.h"
#include "core/ai/memory/memoryfilemanager.h"

ToolCallDispatcher::ToolCallDispatcher(QObject* _pParent)
    : QObject(_pParent)
    , mpConversationContext(nullptr)
    , mpToolHost(nullptr)
{
}

void ToolCallDispatcher::setConversationContext(
    ConversationContext* _pConversationContext)
{
    mpConversationContext = _pConversationContext;
}

void ToolCallDispatcher::setToolHost(IAIToolHost* _pToolHost)
{
    mpToolHost = _pToolHost;
}

void ToolCallDispatcher::dispatch(const QString& _strName,
    const QJsonObject& _jsonArgs)
{
    mstrPendingToolName = _strName;

    if (_strName == "get_analysis_context") {
        executeGetAnalysisContext();
    } else if (_strName == "run_buffer_analysis"
        || _strName == "run_overlay_analysis"
        || _strName == "run_spatial_query"
        || _strName == "run_raster_calc") {
        executeAnalysisTool(_strName, _jsonArgs);
    } else if (_strName == "save_memory") {
        executeSaveMemory(_jsonArgs);
    } else if (_strName == "search_memory") {
        executeSearchMemory(_jsonArgs);
    } else {
        emit toolFailed(_strName, tr("未知工具：%1").arg(_strName));
    }
}

QJsonArray ToolCallDispatcher::buildToolsDefinition() const
{
    QJsonArray _jsonTools;

    {
        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = QJsonObject();

        QJsonObject _fn;
        _fn["name"] = "get_analysis_context";
        _fn["description"] = "读取当前分析上下文，包括是否已加载可分析数据、数据集尺寸与已加载图层列表。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
    }

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
        _fn["description"] = "执行缓冲分析。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
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
        _fn["description"] = "执行叠加分析。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
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
        _fn["description"] = "执行空间查询分析。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
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
        _fn["description"] = "执行栅格计算分析。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
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
        _fn["description"] = "将重要项目事实、偏好或规则保存到 ET.md。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
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
        _fn["description"] = "在 ET.md 和 memory/*.md 中检索历史记忆。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
    }

    return _jsonTools;
}

void ToolCallDispatcher::executeGetAnalysisContext()
{
    if (mpToolHost == nullptr) {
        emit toolFailed(mstrPendingToolName, tr("内部错误：IAIToolHost 未注入"));
        return;
    }

    emit toolExecuted(mstrPendingToolName,
        formatAnalysisContext(mpToolHost->getAnalysisContext()));
    mstrPendingToolName.clear();
}

void ToolCallDispatcher::executeAnalysisTool(const QString& _strToolName,
    const QJsonObject& _jsonArgs)
{
    if (mpToolHost == nullptr) {
        emit toolFailed(_strToolName, tr("内部错误：IAIToolHost 未注入"));
        return;
    }

    QString _strError;
    QString _strResult;
    if (_strToolName == "run_buffer_analysis") {
        const double _dRadius = _jsonArgs["radius_meters"].toDouble(0.0);
        if (_dRadius <= 0.0) {
            emit toolFailed(_strToolName, tr("参数错误：缓冲半径必须大于 0"));
            return;
        }
    } else if (_strToolName == "run_overlay_analysis") {
        const QString _strOverlayType = _jsonArgs["overlay_type"].toString().trimmed();
        if (_strOverlayType.isEmpty()) {
            emit toolFailed(_strToolName, tr("参数错误：叠加类型不能为空"));
            return;
        }
    } else if (_strToolName == "run_spatial_query") {
        const QString _strExpression = _jsonArgs["query_expression"].toString().trimmed();
        if (_strExpression.isEmpty()) {
            emit toolFailed(_strToolName, tr("参数错误：查询表达式不能为空"));
            return;
        }
    } else if (_strToolName == "run_raster_calc") {
        const QString _strExpression = _jsonArgs["expression"].toString().trimmed();
        if (_strExpression.isEmpty()) {
            emit toolFailed(_strToolName, tr("参数错误：计算表达式不能为空"));
            return;
        }
    }

    if (!mpToolHost->executeAnalysisTool(_strToolName, _jsonArgs, _strResult, _strError)) {
        emit toolFailed(_strToolName,
            _strError.isEmpty() ? tr("分析执行失败") : _strError);
        return;
    }

    emit toolExecuted(_strToolName, _strResult);
    mstrPendingToolName.clear();
}

void ToolCallDispatcher::executeSaveMemory(const QJsonObject& _jsonArgs)
{
    if (mpConversationContext == nullptr) {
        emit toolFailed(mstrPendingToolName, tr("内部错误：ConversationContext 未注入"));
        return;
    }

    const QString _strContent = _jsonArgs["content"].toString().trimmed();
    const QString _strSection = _jsonArgs["section"].toString().trimmed();
    const bool _bOverwrite = _jsonArgs["overwrite"].toBool(false);

    if (_strContent.isEmpty()) {
        emit toolFailed(mstrPendingToolName, tr("参数错误：content 不能为空"));
        return;
    }

    if (!mpConversationContext->saveMemory(_strContent, _strSection, _bOverwrite)) {
        emit toolFailed(mstrPendingToolName, tr("写入记忆文件失败，请检查文件权限"));
        return;
    }

    const QString _strResult = _strSection.isEmpty()
        ? tr("已将内容写入 ET.md")
        : tr("已将内容写入 ET.md 的 [%1] 节").arg(_strSection);
    emit toolExecuted(mstrPendingToolName, _strResult);
    mstrPendingToolName.clear();
}

void ToolCallDispatcher::executeSearchMemory(const QJsonObject& _jsonArgs)
{
    if (mpConversationContext == nullptr) {
        emit toolFailed(mstrPendingToolName, tr("内部错误：ConversationContext 未注入"));
        return;
    }

    const QString _strKeyword = _jsonArgs["keyword"].toString().trimmed();
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

QString ToolCallDispatcher::formatAnalysisContext(const QJsonObject& _jsonContext)
{
    QStringList _vLines;
    _vLines << QString::fromUtf8("当前分析上下文");
    _vLines << QString::fromUtf8("可分析数值数据：%1")
        .arg(_jsonContext["has_numeric_data"].toBool() ? "是" : "否");

    const QString _strReadyPath = _jsonContext["ready_data_path"].toString().trimmed();
    if (!_strReadyPath.isEmpty()) {
        _vLines << QString::fromUtf8("当前数据路径：%1").arg(_strReadyPath);
    }

    const QString _strDatasetName = _jsonContext["dataset_name"].toString().trimmed();
    if (!_strDatasetName.isEmpty()) {
        _vLines << QString::fromUtf8("数据集名称：%1").arg(_strDatasetName);
        _vLines << QString::fromUtf8("数据格式：%1")
            .arg(_jsonContext["dataset_format"].toString());
        _vLines << QString::fromUtf8("数据尺寸：%1 行 × %2 列")
            .arg(_jsonContext["dataset_rows"].toInt())
            .arg(_jsonContext["dataset_cols"].toInt());
    }

    const QJsonArray _jsonLayers = _jsonContext["layers"].toArray();
    if (_jsonLayers.isEmpty()) {
        _vLines << QString::fromUtf8("已加载图层：无");
    } else {
        _vLines << QString::fromUtf8("已加载图层：");
        for (const QJsonValue& _jsonVal : _jsonLayers) {
            const QJsonObject _jsonLayer = _jsonVal.toObject();
            _vLines << QString::fromUtf8("- %1 (%2)")
                .arg(_jsonLayer["name"].toString(),
                    _jsonLayer["type"].toString());
        }
    }

    return _vLines.join("\n");
}
