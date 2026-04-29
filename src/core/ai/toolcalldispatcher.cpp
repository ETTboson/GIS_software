#include "toolcalldispatcher.h"

#include "core/ai/conversationcontext.h"
#include "core/interfaces/iaitoolhost.h"
#include "core/ai/memory/memoryfilemanager.h"

#include <QStringList>

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
    } else if (_strName == "run_basic_statistics"
        || _strName == "run_frequency_statistics"
        || _strName == "run_neighborhood_analysis") {
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
        _fn["description"] = "读取当前分析上下文，包括当前选中资产、能力集合、数值视图与已加载地图图层。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
    }

    {
        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = QJsonObject();

        QJsonObject _fn;
        _fn["name"] = "run_basic_statistics";
        _fn["description"] = "对当前选中资产的统一数值视图执行基础统计分析。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
    }

    {
        QJsonObject _binProp;
        _binProp["type"] = "integer";
        _binProp["description"] = "频率统计分箱数，必须大于等于 2";

        QJsonObject _properties;
        _properties["bin_count"] = _binProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;
        _params["required"] = QJsonArray{ "bin_count" };

        QJsonObject _fn;
        _fn["name"] = "run_frequency_statistics";
        _fn["description"] = "对当前选中资产的统一数值视图执行频率统计分析。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
    }

    {
        QJsonObject _windowProp;
        _windowProp["type"] = "integer";
        _windowProp["description"] = "邻域窗口大小，必须是不小于 3 的奇数";

        QJsonObject _properties;
        _properties["window_size"] = _windowProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;
        _params["required"] = QJsonArray{ "window_size" };

        QJsonObject _fn;
        _fn["name"] = "run_neighborhood_analysis";
        _fn["description"] = "对当前选中栅格资产执行邻域分析。";
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
    if (_strToolName == "run_frequency_statistics") {
        const int _nBinCount = _jsonArgs["bin_count"].toInt(0);
        if (_nBinCount < 2) {
            emit toolFailed(_strToolName, tr("参数错误：bin_count 必须大于等于 2"));
            return;
        }
    } else if (_strToolName == "run_neighborhood_analysis") {
        const int _nWindowSize = _jsonArgs["window_size"].toInt(0);
        if (_nWindowSize < 3 || (_nWindowSize % 2) == 0) {
            emit toolFailed(_strToolName, tr("参数错误：window_size 必须是不小于 3 的奇数"));
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
    _vLines << QString::fromUtf8("当前是否存在已选择资产：%1")
        .arg(_jsonContext["has_current_asset"].toBool() ? "是" : "否");

    const QJsonObject _jsonCurrentAsset = _jsonContext["current_asset"].toObject();
    if (!_jsonCurrentAsset.isEmpty()) {
        QStringList _vCapabilityNames;
        for (const QJsonValue& _jsonVal : _jsonCurrentAsset["capabilities"].toArray()) {
            _vCapabilityNames << _jsonVal.toString();
        }
        _vLines << QString::fromUtf8("当前资产名称：%1")
            .arg(_jsonCurrentAsset["name"].toString());
        _vLines << QString::fromUtf8("资产类型：%1")
            .arg(_jsonCurrentAsset["asset_type"].toString());
        _vLines << QString::fromUtf8("源格式：%1")
            .arg(_jsonCurrentAsset["source_format"].toString());
        _vLines << QString::fromUtf8("源路径：%1")
            .arg(_jsonCurrentAsset["source_path"].toString());
        _vLines << QString::fromUtf8("能力集合：%1")
            .arg(_vCapabilityNames.isEmpty()
                ? QString::fromUtf8("无")
                : _vCapabilityNames.join(", "));
        _vLines << QString::fromUtf8("可分析数值视图：%1")
            .arg(_jsonCurrentAsset["has_numeric_data"].toBool() ? "是" : "否");
        if (_jsonCurrentAsset["has_numeric_data"].toBool()) {
            _vLines << QString::fromUtf8("数值视图尺寸：%1 行 × %2 列")
                .arg(_jsonCurrentAsset["numeric_rows"].toInt())
                .arg(_jsonCurrentAsset["numeric_cols"].toInt());
        }
        const QString _strSummary = _jsonCurrentAsset["summary"].toString().trimmed();
        if (!_strSummary.isEmpty()) {
            _vLines << QString::fromUtf8("资产摘要：%1").arg(_strSummary);
        }
    }

    const QJsonArray _jsonAssets = _jsonContext["assets"].toArray();
    _vLines << QString::fromUtf8("分析资产数量：%1").arg(_jsonAssets.size());
    for (const QJsonValue& _jsonVal : _jsonAssets) {
        const QJsonObject _jsonAsset = _jsonVal.toObject();
        _vLines << QString::fromUtf8("- %1 (%2)")
            .arg(_jsonAsset["name"].toString(),
                _jsonAsset["asset_type"].toString());
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
