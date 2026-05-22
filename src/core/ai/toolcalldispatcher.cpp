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

namespace
{

bool isValidOverlayOperationValue(const QString& _strOperation)
{
    const QString _strNormalized = _strOperation.trimmed().toLower();
    return _strNormalized.isEmpty()
        || _strNormalized == "intersect"
        || _strNormalized == "intersection"
        || _strNormalized == "union";
}

bool isValidQueryOperatorValue(const QString& _strOperator)
{
    const QString _strNormalized = _strOperator.trimmed().toLower();
    return _strNormalized == "="
        || _strNormalized == "=="
        || _strNormalized == "eq"
        || _strNormalized == "!="
        || _strNormalized == "<>"
        || _strNormalized == "ne"
        || _strNormalized == ">"
        || _strNormalized == "gt"
        || _strNormalized == ">="
        || _strNormalized == "gte"
        || _strNormalized == "<"
        || _strNormalized == "lt"
        || _strNormalized == "<="
        || _strNormalized == "lte"
        || _strNormalized == "contains";
}

bool isValidSpatialRelationValue(const QString& _strRelation)
{
    const QString _strNormalized = _strRelation.trimmed().toLower();
    return _strNormalized.isEmpty()
        || _strNormalized == "intersects"
        || _strNormalized == "intersect"
        || _strNormalized == "within"
        || _strNormalized == "contains";
}

QJsonObject buildSourceAssetProperty()
{
    QJsonObject _sourceProp;
    _sourceProp["type"] = "string";
    _sourceProp["description"] =
        "可选源分析资产 ID 或演示别名，来自 get_analysis_context 的 assets 列表；未提供时使用当前选中资产。支持别名：建筑/buildings、道路/roads、POI/pois、学校/schools、医院/hospitals、城市/places、水系/waterways、人口/population";
    return _sourceProp;
}

} // namespace

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
        || _strName == "run_neighborhood_analysis"
        || _strName == "run_buffer_analysis"
        || _strName == "run_overlay_analysis"
        || _strName == "run_attribute_query"
        || _strName == "run_spatial_query"
        || _strName == "run_proximity_query") {
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
        QJsonObject _properties;
        _properties["source_asset_id"] = buildSourceAssetProperty();
        _params["properties"] = _properties;

        QJsonObject _fn;
        _fn["name"] = "run_basic_statistics";
        _fn["description"] = "对指定或当前选中资产的统一数值视图执行基础统计分析。";
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
        _properties["source_asset_id"] = buildSourceAssetProperty();
        _properties["bin_count"] = _binProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;
        _params["required"] = QJsonArray{ "bin_count" };

        QJsonObject _fn;
        _fn["name"] = "run_frequency_statistics";
        _fn["description"] = "对指定或当前选中资产的统一数值视图执行频率统计分析。";
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
        _properties["source_asset_id"] = buildSourceAssetProperty();
        _properties["window_size"] = _windowProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;
        _params["required"] = QJsonArray{ "window_size" };

        QJsonObject _fn;
        _fn["name"] = "run_neighborhood_analysis";
        _fn["description"] = "对指定或当前选中栅格资产执行邻域分析。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
    }

    {
        QJsonObject _distanceProp;
        _distanceProp["type"] = "number";
        _distanceProp["description"] = "缓冲距离，必须大于 0，单位为源图层 CRS 单位";

        QJsonObject _segmentsProp;
        _segmentsProp["type"] = "integer";
        _segmentsProp["description"] = "圆弧分段数，可选，默认 8，必须大于 0";

        QJsonObject _properties;
        _properties["source_asset_id"] = buildSourceAssetProperty();
        _properties["distance"] = _distanceProp;
        _properties["segments"] = _segmentsProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;
        _params["required"] = QJsonArray{ "distance" };

        QJsonObject _fn;
        _fn["name"] = "run_buffer_analysis";
        _fn["description"] = "对指定或当前选中矢量资产执行缓冲区分析，并将结果写出为可加载地图的结果图层。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
    }

    {
        QJsonObject _overlayAssetProp;
        _overlayAssetProp["type"] = "string";
        _overlayAssetProp["description"] =
            "参与叠加分析的第二个矢量分析资产 ID，必须来自 get_analysis_context 的 assets 列表";

        QJsonObject _operationProp;
        _operationProp["type"] = "string";
        _operationProp["enum"] = QJsonArray{ "intersect", "union" };
        _operationProp["description"] =
            "叠加操作，可选 intersect 或 union；未提供时默认 intersect";

        QJsonObject _properties;
        _properties["source_asset_id"] = buildSourceAssetProperty();
        _properties["overlay_asset_id"] = _overlayAssetProp;
        _properties["operation"] = _operationProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;
        _params["required"] = QJsonArray{ "overlay_asset_id" };

        QJsonObject _fn;
        _fn["name"] = "run_overlay_analysis";
        _fn["description"] = "对指定或当前选中矢量资产与另一个矢量资产执行 Intersect 或 Union 叠加分析，并将结果写出为可加载地图的结果图层。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
    }

    {
        QJsonObject _fieldProp;
        _fieldProp["type"] = "string";
        _fieldProp["description"] =
            "属性查询字段名，必须来自 get_analysis_context 的 vector_fields";

        QJsonObject _operatorProp;
        _operatorProp["type"] = "string";
        _operatorProp["enum"] = QJsonArray{
            "=", "!=", ">", ">=", "<", "<=", "contains"
        };
        _operatorProp["description"] =
            "属性查询运算符。数值字段支持大小比较，文本字段支持 =、!=、contains";

        QJsonObject _valueProp;
        _valueProp["type"] = "string";
        _valueProp["description"] = "属性查询值";

        QJsonObject _properties;
        _properties["source_asset_id"] = buildSourceAssetProperty();
        _properties["field_name"] = _fieldProp;
        _properties["operator"] = _operatorProp;
        _properties["value"] = _valueProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;
        _params["required"] = QJsonArray{ "field_name", "operator", "value" };

        QJsonObject _fn;
        _fn["name"] = "run_attribute_query";
        _fn["description"] = "按属性条件筛选指定或当前选中矢量资产要素，将命中结果写出为高亮图层。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
    }

    {
        QJsonObject _targetAssetProp;
        _targetAssetProp["type"] = "string";
        _targetAssetProp["description"] =
            "指定区域矢量资产 ID，必须来自 get_analysis_context 的 assets 列表";

        QJsonObject _relationProp;
        _relationProp["type"] = "string";
        _relationProp["enum"] = QJsonArray{ "intersects", "within", "contains" };
        _relationProp["description"] =
            "源要素与指定区域的空间关系，默认 intersects";

        QJsonObject _properties;
        _properties["source_asset_id"] = buildSourceAssetProperty();
        _properties["target_asset_id"] = _targetAssetProp;
        _properties["relation"] = _relationProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;
        _params["required"] = QJsonArray{ "target_asset_id" };

        QJsonObject _fn;
        _fn["name"] = "run_spatial_query";
        _fn["description"] = "按空间关系筛选指定或当前选中矢量资产要素，例如与指定区域相交的所有道路，并将命中结果写出为高亮图层。";
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
    }

    {
        QJsonObject _referenceAssetProp;
        _referenceAssetProp["type"] = "string";
        _referenceAssetProp["description"] =
            "参考矢量资产 ID 或演示别名，例如 道路/roads、医院/hospitals；必须来自 get_analysis_context 的 assets 列表";

        QJsonObject _distanceProp;
        _distanceProp["type"] = "number";
        _distanceProp["description"] =
            "邻近距离，必须大于 0，使用米作为演示输入单位";

        QJsonObject _segmentsProp;
        _segmentsProp["type"] = "integer";
        _segmentsProp["description"] = "缓冲圆弧分段数，可选，默认 8，必须大于 0";

        QJsonObject _invertProp;
        _invertProp["type"] = "boolean";
        _invertProp["description"] =
            "为 true 时返回距离范围外的源要素；例如“没有医院”“不在范围内”应设为 true";

        QJsonObject _properties;
        _properties["source_asset_id"] = buildSourceAssetProperty();
        _properties["reference_asset_id"] = _referenceAssetProp;
        _properties["distance"] = _distanceProp;
        _properties["segments"] = _segmentsProp;
        _properties["invert_match"] = _invertProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;
        _params["required"] =
            QJsonArray{ "source_asset_id", "reference_asset_id", "distance" };

        QJsonObject _fn;
        _fn["name"] = "run_proximity_query";
        _fn["description"] =
            "找出距离参考图层指定米数范围内或范围外的源矢量要素。例如“找出距离道路 50 米内的建筑”使用 source_asset_id=建筑、reference_asset_id=道路、distance=50；“哪些学校周围 1 公里内没有医院”使用 source_asset_id=学校、reference_asset_id=医院、distance=1000、invert_match=true。";
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
    } else if (_strToolName == "run_buffer_analysis") {
        const double _dDistance = _jsonArgs["distance"].toDouble(0.0);
        const int _nSegments = _jsonArgs.contains("segments")
            ? _jsonArgs["segments"].toInt(8)
            : 8;
        if (_dDistance <= 0.0) {
            emit toolFailed(_strToolName, tr("参数错误：distance 必须大于 0"));
            return;
        }
        if (_nSegments <= 0) {
            emit toolFailed(_strToolName, tr("参数错误：segments 必须大于 0"));
            return;
        }
    } else if (_strToolName == "run_overlay_analysis") {
        const QString _strOverlayAssetId =
            _jsonArgs["overlay_asset_id"].toString().trimmed();
        if (_strOverlayAssetId.isEmpty()) {
            emit toolFailed(_strToolName, tr("参数错误：overlay_asset_id 不能为空"));
            return;
        }
        if (_jsonArgs.contains("operation")
            && !isValidOverlayOperationValue(_jsonArgs["operation"].toString())) {
            emit toolFailed(_strToolName,
                tr("参数错误：operation 必须是 intersect 或 union"));
            return;
        }
    } else if (_strToolName == "run_attribute_query") {
        const QString _strFieldName =
            _jsonArgs["field_name"].toString().trimmed();
        const QString _strOperator =
            _jsonArgs["operator"].toString().trimmed();
        if (_strFieldName.isEmpty()) {
            emit toolFailed(_strToolName, tr("参数错误：field_name 不能为空"));
            return;
        }
        if (!isValidQueryOperatorValue(_strOperator)) {
            emit toolFailed(_strToolName,
                tr("参数错误：operator 必须是 =、!=、>、>=、<、<= 或 contains"));
            return;
        }
    } else if (_strToolName == "run_spatial_query") {
        const QString _strTargetAssetId =
            _jsonArgs["target_asset_id"].toString().trimmed();
        if (_strTargetAssetId.isEmpty()) {
            emit toolFailed(_strToolName, tr("参数错误：target_asset_id 不能为空"));
            return;
        }
        if (_jsonArgs.contains("relation")
            && !isValidSpatialRelationValue(_jsonArgs["relation"].toString())) {
            emit toolFailed(_strToolName,
                tr("参数错误：relation 必须是 intersects、within 或 contains"));
            return;
        }
    } else if (_strToolName == "run_proximity_query") {
        const QString _strReferenceAssetId =
            _jsonArgs["reference_asset_id"].toString().trimmed();
        const double _dDistance = _jsonArgs["distance"].toDouble(0.0);
        const int _nSegments = _jsonArgs.contains("segments")
            ? _jsonArgs["segments"].toInt(8)
            : 8;
        if (_strReferenceAssetId.isEmpty()) {
            emit toolFailed(_strToolName, tr("参数错误：reference_asset_id 不能为空"));
            return;
        }
        if (_dDistance <= 0.0) {
            emit toolFailed(_strToolName, tr("参数错误：distance 必须大于 0"));
            return;
        }
        if (_nSegments <= 0) {
            emit toolFailed(_strToolName, tr("参数错误：segments 必须大于 0"));
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
        _vLines << QString::fromUtf8("当前资产 ID：%1")
            .arg(_jsonCurrentAsset["id"].toString());
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
        const QString _strGeomType =
            _jsonCurrentAsset["vector_geometry_type"].toString().trimmed();
        if (!_strGeomType.isEmpty()) {
            _vLines << QString::fromUtf8("矢量几何类型：%1").arg(_strGeomType);
            _vLines << QString::fromUtf8("矢量要素数：%1")
                .arg(_jsonCurrentAsset["vector_feature_count"].toInt());
            QStringList _vFields;
            for (const QJsonValue& _jsonFieldVal :
                _jsonCurrentAsset["vector_fields"].toArray()) {
                _vFields << _jsonFieldVal.toString();
            }
            QStringList _vNumericFields;
            for (const QJsonValue& _jsonFieldVal :
                _jsonCurrentAsset["vector_numeric_fields"].toArray()) {
                _vNumericFields << _jsonFieldVal.toString();
            }
            if (!_vFields.isEmpty()) {
                _vLines << QString::fromUtf8("矢量字段：%1").arg(_vFields.join(", "));
            }
            if (!_vNumericFields.isEmpty()) {
                _vLines << QString::fromUtf8("数值字段：%1")
                    .arg(_vNumericFields.join(", "));
            }
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
        QStringList _vAliases;
        for (const QJsonValue& _jsonAliasVal :
            _jsonAsset["demo_aliases"].toArray()) {
            _vAliases << _jsonAliasVal.toString();
        }
        const QString _strAliasText = _vAliases.isEmpty()
            ? QString()
            : QString::fromUtf8("，别名：%1").arg(_vAliases.join(", "));
        _vLines << QString::fromUtf8("- %1 (%2)，ID：%3%4")
            .arg(_jsonAsset["name"].toString(),
                _jsonAsset["asset_type"].toString(),
                _jsonAsset["id"].toString(),
                _strAliasText);
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
