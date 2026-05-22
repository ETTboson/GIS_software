#include "analysisworkflowcoordinator.h"

#include "core/interfaces/iaitoolhost.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>

namespace
{

QString analysisTaskTypeToString(
    AnalysisWorkflowCoordinator::AnalysisTaskType _type)
{
    switch (_type) {
    case AnalysisWorkflowCoordinator::AnalysisTaskType::BasicStatistics:
        return QString::fromUtf8("基础统计");
    case AnalysisWorkflowCoordinator::AnalysisTaskType::FrequencyStatistics:
        return QString::fromUtf8("频率统计");
    case AnalysisWorkflowCoordinator::AnalysisTaskType::NeighborhoodAnalysis:
        return QString::fromUtf8("邻域分析");
    case AnalysisWorkflowCoordinator::AnalysisTaskType::BufferAnalysis:
        return QString::fromUtf8("缓冲区分析");
    case AnalysisWorkflowCoordinator::AnalysisTaskType::OverlayAnalysis:
        return QString::fromUtf8("叠加分析");
    case AnalysisWorkflowCoordinator::AnalysisTaskType::AttributeQuery:
        return QString::fromUtf8("属性查询");
    case AnalysisWorkflowCoordinator::AnalysisTaskType::SpatialQuery:
        return QString::fromUtf8("空间查询");
    case AnalysisWorkflowCoordinator::AnalysisTaskType::ProximityQuery:
        return QString::fromUtf8("邻近查询");
    case AnalysisWorkflowCoordinator::AnalysisTaskType::None:
    default:
        return QString::fromUtf8("未确定");
    }
}

bool assetHasCapability(const QJsonObject& _jsonAsset,
    const QString& _strCapability)
{
    const QJsonArray _jsonCaps = _jsonAsset["capabilities"].toArray();
    for (const QJsonValue& _jsonVal : _jsonCaps) {
        if (_jsonVal.toString() == _strCapability) {
            return true;
        }
    }
    return false;
}

bool assetIsAreaVector(const QJsonObject& _jsonAsset)
{
    return assetHasCapability(_jsonAsset, QStringLiteral("spatial_vector"))
        && _jsonAsset["vector_geometry_type"].toString()
            .toLower().contains(QStringLiteral("polygon"));
}

QStringList aliasKeywordsForAssetRef(const QString& _strAssetRef)
{
    const QString _strRef = _strAssetRef.trimmed().toLower();
    QStringList _vKeywords;
    if (_strRef.contains(QString::fromUtf8("建筑"))
        || _strRef.contains(QStringLiteral("building"))) {
        _vKeywords << QStringLiteral("buildings") << QString::fromUtf8("建筑");
    }
    if (_strRef.contains(QString::fromUtf8("道路"))
        || _strRef.contains(QStringLiteral("road"))) {
        _vKeywords << QStringLiteral("roads") << QString::fromUtf8("道路");
    }
    if (_strRef.contains(QStringLiteral("poi"))) {
        _vKeywords << QStringLiteral("pois") << QStringLiteral("poi");
    }
    if (_strRef.contains(QString::fromUtf8("城市"))
        || _strRef.contains(QString::fromUtf8("地点"))
        || _strRef.contains(QStringLiteral("place"))) {
        _vKeywords << QStringLiteral("places") << QStringLiteral("place");
    }
    if (_strRef.contains(QString::fromUtf8("水系"))
        || _strRef.contains(QString::fromUtf8("河流"))
        || _strRef.contains(QStringLiteral("water"))) {
        _vKeywords << QStringLiteral("waterways") << QStringLiteral("water");
    }
    if (_strRef.contains(QString::fromUtf8("人口"))
        || _strRef.contains(QStringLiteral("population"))) {
        _vKeywords << QStringLiteral("places") << QStringLiteral("population");
    }
    if (_strRef.contains(QString::fromUtf8("学校"))
        || _strRef.contains(QStringLiteral("school"))) {
        _vKeywords << QStringLiteral("schools") << QStringLiteral("school")
                   << QStringLiteral("poi") << QStringLiteral("pois");
    }
    if (_strRef.contains(QString::fromUtf8("医院"))
        || _strRef.contains(QStringLiteral("hospital"))
        || _strRef.contains(QStringLiteral("clinic"))) {
        _vKeywords << QStringLiteral("hospitals") << QStringLiteral("hospital")
                   << QStringLiteral("clinic") << QStringLiteral("poi")
                   << QStringLiteral("pois");
    }
    if (_vKeywords.isEmpty() && !_strRef.isEmpty()) {
        _vKeywords << _strRef;
    }
    return _vKeywords;
}

QString cleanedProximityAssetRef(QString _strRef)
{
    _strRef = _strRef.trimmed();
    _strRef.remove(QString::fromUtf8("哪些"));
    _strRef.remove(QString::fromUtf8("哪个"));
    _strRef.remove(QString::fromUtf8("所有"));
    _strRef.remove(QString::fromUtf8("全部"));
    _strRef.remove(QString::fromUtf8("找出"));
    _strRef.remove(QString::fromUtf8("查询"));
    _strRef.remove(QString::fromUtf8("筛选"));
    _strRef.remove(QRegularExpression(QStringLiteral("[？?。.]$")));
    return _strRef.trimmed();
}

bool textRequestsProximityInvertMatch(const QString& _strText)
{
    const QString _strNormalized = _strText.toLower();
    return _strNormalized.contains(QString::fromUtf8("没有"))
        || _strNormalized.contains(QString::fromUtf8("无"))
        || _strNormalized.contains(QString::fromUtf8("不在"))
        || _strNormalized.contains(QString::fromUtf8("之外"))
        || _strNormalized.contains(QStringLiteral("without"))
        || _strNormalized.contains(QStringLiteral(" no "));
}

void fillKnownProximityAliases(const QString& _strText,
    QString& _strSourceAssetId,
    QString& _strReferenceAssetId)
{
    const QString _strNormalized = _strText.toLower();
    if (_strSourceAssetId.trimmed().isEmpty()
        && (_strNormalized.contains(QString::fromUtf8("学校"))
            || _strNormalized.contains(QStringLiteral("school")))) {
        _strSourceAssetId = QString::fromUtf8("学校");
    }
    if (_strReferenceAssetId.trimmed().isEmpty()
        && (_strNormalized.contains(QString::fromUtf8("医院"))
            || _strNormalized.contains(QStringLiteral("hospital"))
            || _strNormalized.contains(QStringLiteral("clinic")))) {
        _strReferenceAssetId = QString::fromUtf8("医院");
    }
    if (_strSourceAssetId.trimmed().isEmpty()
        && (_strNormalized.contains(QString::fromUtf8("建筑"))
            || _strNormalized.contains(QStringLiteral("building")))) {
        _strSourceAssetId = QString::fromUtf8("建筑");
    }
    if (_strReferenceAssetId.trimmed().isEmpty()
        && (_strNormalized.contains(QString::fromUtf8("道路"))
            || _strNormalized.contains(QStringLiteral("road")))) {
        _strReferenceAssetId = QString::fromUtf8("道路");
    }
}

bool parseSpatialRelationValue(const QString& _strRelation,
    QString& _strRelationId)
{
    const QString _strNormalized = _strRelation.trimmed().toLower();
    if (_strNormalized.isEmpty()
        || _strNormalized == "intersects"
        || _strNormalized == "intersect"
        || _strNormalized == "intersection"
        || _strNormalized.contains(QString::fromUtf8("相交"))) {
        _strRelationId = QStringLiteral("intersects");
        return true;
    }
    if (_strNormalized == "within"
        || _strNormalized.contains(QString::fromUtf8("范围内"))
        || _strNormalized.contains(QString::fromUtf8("位于"))
        || _strNormalized.contains(QString::fromUtf8("在")) ) {
        _strRelationId = QStringLiteral("within");
        return true;
    }
    if (_strNormalized == "contains"
        || _strNormalized.contains(QString::fromUtf8("包含"))) {
        _strRelationId = QStringLiteral("contains");
        return true;
    }
    return false;
}

QString overlayOperationToToolValue(OverlayOperationType _eOperation)
{
    return (_eOperation == OverlayOperationType::Union)
        ? QStringLiteral("union")
        : QStringLiteral("intersect");
}

QString overlayOperationToDisplayName(OverlayOperationType _eOperation)
{
    return (_eOperation == OverlayOperationType::Union)
        ? QString::fromUtf8("联合")
        : QString::fromUtf8("交集");
}

bool parseOverlayOperationValue(const QString& _strOperation,
    OverlayOperationType& _eOperation)
{
    const QString _strNormalized = _strOperation.trimmed().toLower();
    if (_strNormalized.isEmpty()
        || _strNormalized == "intersect"
        || _strNormalized == "intersection") {
        _eOperation = OverlayOperationType::Intersect;
        return true;
    }
    if (_strNormalized == "union") {
        _eOperation = OverlayOperationType::Union;
        return true;
    }
    return false;
}

} // namespace

void AnalysisWorkflowCoordinator::setToolHost(IAIToolHost* _pToolHost)
{
    mpToolHost = _pToolHost;
}

void AnalysisWorkflowCoordinator::reset()
{
    mState = AnalysisWorkflowState();
    mstrLastUserText.clear();
}

bool AnalysisWorkflowCoordinator::isAnalysisModeActive() const
{
    return mState.bActive;
}

bool AnalysisWorkflowCoordinator::shouldEnterAnalysisMode(
    const QString& _strUserText) const
{
    return detectTaskType(_strUserText) != AnalysisTaskType::None;
}

AnalysisWorkflowCoordinator::AnalysisTurnKind
AnalysisWorkflowCoordinator::classifyTurn(const QString& _strUserText) const
{
    if (isCancelText(_strUserText)) {
        return AnalysisTurnKind::Cancel;
    }

    const AnalysisTaskType _type = detectTaskType(_strUserText);
    if (!mState.bActive) {
        return (_type == AnalysisTaskType::None)
            ? AnalysisTurnKind::IntentOnly
            : AnalysisTurnKind::StateUpdate;
    }

    if (_type == AnalysisTaskType::None
        && !looksLikeStateUpdateInput(_strUserText)
        && (isCapabilityIntent(_strUserText)
            || !shouldEnterAnalysisMode(_strUserText))) {
        return AnalysisTurnKind::ExitAnalysisMode;
    }

    return looksLikeStateUpdateInput(_strUserText)
        ? AnalysisTurnKind::StateUpdate
        : AnalysisTurnKind::IntentOnly;
}

void AnalysisWorkflowCoordinator::beginOrContinue(const QString& _strUserText)
{
    mstrLastUserText = _strUserText.trimmed();
    mState.bActive = true;
    inferTaskTypeFromText(_strUserText);
    recalculateMissingParams();
}

QString AnalysisWorkflowCoordinator::buildDeterministicReply() const
{
    if (mState.type == AnalysisTaskType::None) {
        return QString::fromUtf8(
            "请选择一个选项：\n"
            "1. 基础统计\n"
            "2. 频率统计\n"
            "3. 邻域分析\n"
            "4. 缓冲区分析\n"
            "5. 叠加分析\n"
            "6. 属性查询\n"
            "7. 空间查询\n"
            "8. 邻近查询");
    }

    return buildNeedMoreInfoReply();
}

QString AnalysisWorkflowCoordinator::buildNeedMoreInfoReply() const
{
    QStringList _vLines;
    if (!mState.strLastError.trimmed().isEmpty()) {
        _vLines << mState.strLastError.trimmed();
        _vLines << QString();
    }

    _vLines << QString::fromUtf8("还需要这些信息：");
    _vLines << buildMissingParamLines();
    return _vLines.join("\n").trimmed();
}

QJsonArray AnalysisWorkflowCoordinator::buildStateUpdateToolsDefinition() const
{
    QJsonArray _jsonTools;

    {
        QJsonObject _toolNameProp;
        _toolNameProp["type"] = "string";
        _toolNameProp["enum"] = QJsonArray{
            "run_basic_statistics",
            "run_frequency_statistics",
            "run_neighborhood_analysis",
            "run_buffer_analysis",
            "run_overlay_analysis",
            "run_attribute_query",
            "run_spatial_query",
            "run_proximity_query"
        };

        QJsonObject _binProp;
        _binProp["type"] = "integer";

        QJsonObject _windowProp;
        _windowProp["type"] = "integer";

        QJsonObject _distanceProp;
        _distanceProp["type"] = "number";

        QJsonObject _segmentsProp;
        _segmentsProp["type"] = "integer";

        QJsonObject _overlayAssetProp;
        _overlayAssetProp["type"] = "string";

        QJsonObject _operationProp;
        _operationProp["type"] = "string";
        _operationProp["enum"] = QJsonArray{ "intersect", "union" };

        QJsonObject _fieldProp;
        _fieldProp["type"] = "string";

        QJsonObject _queryOperatorProp;
        _queryOperatorProp["type"] = "string";
        _queryOperatorProp["enum"] = QJsonArray{
            "=", "!=", ">", ">=", "<", "<=", "contains"
        };

        QJsonObject _queryValueProp;
        _queryValueProp["type"] = "string";

        QJsonObject _targetAssetProp;
        _targetAssetProp["type"] = "string";

        QJsonObject _sourceAssetProp;
        _sourceAssetProp["type"] = "string";

        QJsonObject _referenceAssetProp;
        _referenceAssetProp["type"] = "string";

        QJsonObject _invertProp;
        _invertProp["type"] = "boolean";

        QJsonObject _relationProp;
        _relationProp["type"] = "string";
        _relationProp["enum"] = QJsonArray{ "intersects", "within", "contains" };

        QJsonObject _properties;
        _properties["tool_name"] = _toolNameProp;
        _properties["bin_count"] = _binProp;
        _properties["window_size"] = _windowProp;
        _properties["distance"] = _distanceProp;
        _properties["segments"] = _segmentsProp;
        _properties["overlay_asset_id"] = _overlayAssetProp;
        _properties["operation"] = _operationProp;
        _properties["field_name"] = _fieldProp;
        _properties["operator"] = _queryOperatorProp;
        _properties["value"] = _queryValueProp;
        _properties["target_asset_id"] = _targetAssetProp;
        _properties["source_asset_id"] = _sourceAssetProp;
        _properties["reference_asset_id"] = _referenceAssetProp;
        _properties["invert_match"] = _invertProp;
        _properties["relation"] = _relationProp;

        QJsonObject _params;
        _params["type"] = "object";
        _params["properties"] = _properties;

        QJsonObject _fn;
        _fn["name"] = "set_analysis_params";
        _fn["description"] = QString::fromUtf8(
            "内部分析状态更新工具。用户补充分析类型或参数时调用，只更新槽位。");
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
        _fn["name"] = "cancel_analysis";
        _fn["description"] = QString::fromUtf8("内部取消工具。");
        _fn["parameters"] = _params;

        QJsonObject _tool;
        _tool["type"] = "function";
        _tool["function"] = _fn;
        _jsonTools.append(_tool);
    }

    return _jsonTools;
}

QJsonArray AnalysisWorkflowCoordinator::buildStateUpdateMessages(
    const QString& _strBasePrompt,
    const QJsonArray& _jsonRawHistory,
    const QString& _strLatestUserText) const
{
    QJsonArray _jsonMessages;
    _jsonMessages.append(QJsonObject{
        { "role", "system" },
        { "content", _strBasePrompt }
    });
    _jsonMessages.append(QJsonObject{
        { "role", "system" },
        { "content", buildStateSnapshot(_strLatestUserText) }
    });

    const QJsonArray _jsonHistory = extractRecentDialogue(
        _jsonRawHistory, _strLatestUserText);
    for (const QJsonValue& _jsonVal : _jsonHistory) {
        _jsonMessages.append(_jsonVal);
    }

    _jsonMessages.append(QJsonObject{
        { "role", "user" },
        { "content", _strLatestUserText }
    });
    return _jsonMessages;
}

AnalysisWorkflowCoordinator::TransitionResult
AnalysisWorkflowCoordinator::applyStateUpdateTool(const QString& _strToolName,
    const QJsonObject& _jsonArgs)
{
    if (_strToolName == "cancel_analysis") {
        return handleDirectUserAction(QString::fromUtf8("取消"));
    }

    if (_strToolName != "set_analysis_params") {
        TransitionResult _result;
        _result.kind = TransitionKind::Error;
        _result.strError = QString::fromUtf8("分析模式收到未知内部工具：%1")
            .arg(_strToolName);
        return _result;
    }

    return applyStateUpdateArgs(_jsonArgs);
}

AnalysisWorkflowCoordinator::TransitionResult
AnalysisWorkflowCoordinator::handleDirectUserAction(const QString& _strUserText)
{
    TransitionResult _result;
    if (!isCancelText(_strUserText)) {
        return _result;
    }

    _result.kind = TransitionKind::FinishWithText;
    _result.strFinalText = QString::fromUtf8("已取消当前分析流程。");
    return _result;
}

bool AnalysisWorkflowCoordinator::tryApplyLocalStateUpdate(
    const QString& _strUserText,
    TransitionResult& _outTransition)
{
    _outTransition = TransitionResult();

    if (isCancelText(_strUserText)) {
        _outTransition = handleDirectUserAction(_strUserText);
        return true;
    }

    QJsonObject _jsonArgs;

    const AnalysisTaskType _type = detectTaskType(_strUserText);
    if (_type != AnalysisTaskType::None) {
        switch (_type) {
        case AnalysisTaskType::BasicStatistics:
            _jsonArgs["tool_name"] = "run_basic_statistics";
            break;
        case AnalysisTaskType::FrequencyStatistics:
            _jsonArgs["tool_name"] = "run_frequency_statistics";
            break;
        case AnalysisTaskType::NeighborhoodAnalysis:
            _jsonArgs["tool_name"] = "run_neighborhood_analysis";
            break;
        case AnalysisTaskType::BufferAnalysis:
            _jsonArgs["tool_name"] = "run_buffer_analysis";
            break;
        case AnalysisTaskType::OverlayAnalysis:
            _jsonArgs["tool_name"] = "run_overlay_analysis";
            break;
        case AnalysisTaskType::AttributeQuery:
            _jsonArgs["tool_name"] = "run_attribute_query";
            break;
        case AnalysisTaskType::SpatialQuery:
            _jsonArgs["tool_name"] = "run_spatial_query";
            break;
        case AnalysisTaskType::ProximityQuery:
            _jsonArgs["tool_name"] = "run_proximity_query";
            break;
        case AnalysisTaskType::None:
        default:
            break;
        }
    }

    int _nBinCount = 0;
    if (tryExtractBinCount(_strUserText, _nBinCount)) {
        _jsonArgs["bin_count"] = _nBinCount;
    }

    int _nWindowSize = 0;
    if (tryExtractWindowSize(_strUserText, _nWindowSize)) {
        _jsonArgs["window_size"] = _nWindowSize;
    }

    if (_type == AnalysisTaskType::BufferAnalysis
        || mState.type == AnalysisTaskType::BufferAnalysis) {
        double _dBufferDistance = 0.0;
        if (tryExtractBufferDistance(_strUserText, _dBufferDistance)) {
            _jsonArgs["distance"] = _dBufferDistance;
        }

        int _nBufferSegments = 0;
        if (tryExtractBufferSegments(_strUserText, _nBufferSegments)) {
            _jsonArgs["segments"] = _nBufferSegments;
        }
    }

    if (_type == AnalysisTaskType::OverlayAnalysis
        || mState.type == AnalysisTaskType::OverlayAnalysis) {
        OverlayOperationType _eOverlayOperation = OverlayOperationType::Intersect;
        if (tryExtractOverlayOperation(_strUserText, _eOverlayOperation)) {
            _jsonArgs["operation"] = overlayOperationToToolValue(_eOverlayOperation);
        }

        QString _strOverlayAssetId;
        if (tryExtractOverlayAssetId(_strUserText, _strOverlayAssetId)) {
            _jsonArgs["overlay_asset_id"] = _strOverlayAssetId;
        }
    }

    if (_type == AnalysisTaskType::AttributeQuery
        || mState.type == AnalysisTaskType::AttributeQuery) {
        QString _strFieldName;
        QString _strOperatorId;
        QString _strValueText;
        if (tryExtractAttributeQueryParts(
                _strUserText, _strFieldName, _strOperatorId, _strValueText)) {
            _jsonArgs["field_name"] = _strFieldName;
            _jsonArgs["operator"] = _strOperatorId;
            _jsonArgs["value"] = _strValueText;
        }
    }

    if (_type == AnalysisTaskType::SpatialQuery
        || mState.type == AnalysisTaskType::SpatialQuery) {
        QString _strTargetAssetId;
        if (tryExtractSpatialTargetAssetId(_strUserText, _strTargetAssetId)) {
            _jsonArgs["target_asset_id"] = _strTargetAssetId;
        }

        QString _strRelationId;
        if (tryExtractSpatialRelation(_strUserText, _strRelationId)) {
            _jsonArgs["relation"] = _strRelationId;
        }
    }

    if (_type == AnalysisTaskType::ProximityQuery
        || mState.type == AnalysisTaskType::ProximityQuery) {
        if (textRequestsProximityInvertMatch(_strUserText)) {
            _jsonArgs["invert_match"] = true;
        }

        double _dDistance = 0.0;
        if (tryExtractBufferDistance(_strUserText, _dDistance)) {
            _jsonArgs["distance"] = _dDistance;
        }

        int _nBufferSegments = 0;
        if (tryExtractBufferSegments(_strUserText, _nBufferSegments)) {
            _jsonArgs["segments"] = _nBufferSegments;
        }

        QString _strSourceAssetId;
        QString _strReferenceAssetId;
        if (tryExtractProximityAssets(
                _strUserText, _strSourceAssetId, _strReferenceAssetId)) {
            if (!_strSourceAssetId.isEmpty()) {
                _jsonArgs["source_asset_id"] = _strSourceAssetId;
            }
            if (!_strReferenceAssetId.isEmpty()) {
                _jsonArgs["reference_asset_id"] = _strReferenceAssetId;
            }
        }
    }

    if (_jsonArgs.isEmpty()) {
        return false;
    }

    _outTransition = applyStateUpdateArgs(_jsonArgs);
    return true;
}

void AnalysisWorkflowCoordinator::inferTaskTypeFromText(const QString& _strUserText)
{
    const AnalysisTaskType _type = detectTaskType(_strUserText);
    if (_type != AnalysisTaskType::None) {
        mState.type = _type;
    }

    if (mState.type == AnalysisTaskType::OverlayAnalysis) {
        OverlayOperationType _eOverlayOperation = OverlayOperationType::Intersect;
        if (tryExtractOverlayOperation(_strUserText, _eOverlayOperation)) {
            mState.eOverlayOperation = _eOverlayOperation;
        }
    } else if (mState.type == AnalysisTaskType::SpatialQuery) {
        QString _strRelationId;
        if (tryExtractSpatialRelation(_strUserText, _strRelationId)) {
            mState.strSpatialRelationId = _strRelationId;
        }
    }
}

void AnalysisWorkflowCoordinator::recalculateMissingParams()
{
    mState.vMissingParams.clear();
    switch (mState.type) {
    case AnalysisTaskType::BasicStatistics:
        break;
    case AnalysisTaskType::FrequencyStatistics:
        if (!mState.bHasBinCount) {
            mState.vMissingParams << "bin_count";
        }
        break;
    case AnalysisTaskType::NeighborhoodAnalysis:
        if (!mState.bHasWindowSize) {
            mState.vMissingParams << "window_size";
        }
        break;
    case AnalysisTaskType::BufferAnalysis:
        if (!mState.bHasBufferDistance) {
            mState.vMissingParams << "distance";
        }
        break;
    case AnalysisTaskType::OverlayAnalysis:
        if (!mState.bHasOverlayAssetId) {
            tryAutoSelectSingleOverlayAsset();
        }
        if (!mState.bHasOverlayAssetId) {
            mState.vMissingParams << "overlay_asset_id";
        }
        break;
    case AnalysisTaskType::AttributeQuery:
        if (!mState.bHasQueryFieldName) {
            mState.vMissingParams << "field_name";
        }
        if (!mState.bHasQueryOperator) {
            mState.vMissingParams << "operator";
        }
        if (!mState.bHasQueryValue) {
            mState.vMissingParams << "value";
        }
        break;
    case AnalysisTaskType::SpatialQuery:
        if (!mState.bHasSpatialTargetAssetId) {
            tryAutoSelectSingleSpatialTargetAsset();
        }
        if (!mState.bHasSpatialTargetAssetId) {
            mState.vMissingParams << "target_asset_id";
        }
        break;
    case AnalysisTaskType::ProximityQuery:
        if (!mState.bHasSourceAssetId) {
            mState.vMissingParams << "source_asset_id";
        }
        if (!mState.bHasProximityReferenceAssetId) {
            mState.vMissingParams << "reference_asset_id";
        }
        if (!mState.bHasBufferDistance) {
            mState.vMissingParams << "distance";
        }
        break;
    case AnalysisTaskType::None:
    default:
        mState.vMissingParams << "task_type";
        break;
    }
}

bool AnalysisWorkflowCoordinator::tryAutoSelectSingleOverlayAsset()
{
    if (mpToolHost == nullptr) {
        return false;
    }

    const QJsonObject _jsonContext = mpToolHost->getAnalysisContext();
    const QString _strSelectedAssetId =
        _jsonContext["selected_asset_id"].toString().trimmed();
    QStringList _vCandidateIds;
    for (const QJsonValue& _jsonVal : _jsonContext["assets"].toArray()) {
        const QJsonObject _jsonAsset = _jsonVal.toObject();
        const QString _strAssetId = _jsonAsset["id"].toString().trimmed();
        if (_strAssetId.isEmpty()
            || _strAssetId == _strSelectedAssetId
            || !assetHasCapability(_jsonAsset, QStringLiteral("spatial_vector"))) {
            continue;
        }
        _vCandidateIds << _strAssetId;
    }

    if (_vCandidateIds.size() != 1) {
        return false;
    }

    mState.bHasOverlayAssetId = true;
    mState.strOverlayAssetId = _vCandidateIds.first();
    return true;
}

bool AnalysisWorkflowCoordinator::tryAutoSelectSingleSpatialTargetAsset()
{
    if (mpToolHost == nullptr) {
        return false;
    }

    const QJsonObject _jsonContext = mpToolHost->getAnalysisContext();
    const QString _strSelectedAssetId =
        _jsonContext["selected_asset_id"].toString().trimmed();
    QStringList _vCandidateIds;
    for (const QJsonValue& _jsonVal : _jsonContext["assets"].toArray()) {
        const QJsonObject _jsonAsset = _jsonVal.toObject();
        const QString _strAssetId = _jsonAsset["id"].toString().trimmed();
        if (_strAssetId.isEmpty()
            || _strAssetId == _strSelectedAssetId
            || !assetIsAreaVector(_jsonAsset)) {
            continue;
        }
        _vCandidateIds << _strAssetId;
    }

    if (_vCandidateIds.size() != 1) {
        return false;
    }

    mState.bHasSpatialTargetAssetId = true;
    mState.strSpatialTargetAssetId = _vCandidateIds.first();
    return true;
}

bool AnalysisWorkflowCoordinator::tryResolveAssetRef(const QString& _strAssetRef,
    QString& _strAssetId) const
{
    if (mpToolHost == nullptr || _strAssetRef.trimmed().isEmpty()) {
        return false;
    }

    const QString _strRef = _strAssetRef.trimmed().toLower();
    const QJsonObject _jsonContext = mpToolHost->getAnalysisContext();
    for (const QJsonValue& _jsonVal : _jsonContext["assets"].toArray()) {
        const QJsonObject _jsonAsset = _jsonVal.toObject();
        const QString _strAssetIdCurrent = _jsonAsset["id"].toString().trimmed();
        if (_strAssetIdCurrent.compare(_strAssetRef.trimmed(),
                Qt::CaseInsensitive) == 0) {
            _strAssetId = _strAssetIdCurrent;
            return true;
        }
    }

    const QStringList _vKeywords = aliasKeywordsForAssetRef(_strRef);
    for (const QJsonValue& _jsonVal : _jsonContext["assets"].toArray()) {
        const QJsonObject _jsonAsset = _jsonVal.toObject();
        QStringList _vFieldNames;
        for (const QJsonValue& _jsonFieldVal :
            _jsonAsset["vector_fields"].toArray()) {
            _vFieldNames << _jsonFieldVal.toString();
        }
        QString _strHaystack = QStringLiteral("%1 %2 %3 %4")
            .arg(_jsonAsset["name"].toString(),
                _jsonAsset["source_path"].toString(),
                _jsonAsset["vector_source_uri"].toString(),
                _vFieldNames.join(QStringLiteral(" ")))
            .toLower();
        for (const QJsonValue& _jsonAliasVal :
            _jsonAsset["demo_aliases"].toArray()) {
            _strHaystack += QStringLiteral(" %1")
                .arg(_jsonAliasVal.toString().toLower());
        }
        for (const QString& _strKeyword : _vKeywords) {
            if (!_strKeyword.trimmed().isEmpty()
                && _strHaystack.contains(_strKeyword.trimmed().toLower())) {
                _strAssetId = _jsonAsset["id"].toString().trimmed();
                return !_strAssetId.isEmpty();
            }
        }
    }
    return false;
}

bool AnalysisWorkflowCoordinator::tryExtractProximityAssets(
    const QString& _strText,
    QString& _strSourceAssetId,
    QString& _strReferenceAssetId) const
{
    static const QRegularExpression S_RE_NEAR_PATTERN(
        QString::fromUtf8(R"(距离\s*([^0-9，,。]+?)\s*\d+(?:\.\d+)?\s*(?:公里|千米|km|KM|米|m|M)?\s*(?:内|以内|范围内)?\s*的\s*([^，,。]+))"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch _match = S_RE_NEAR_PATTERN.match(_strText);
    if (_match.hasMatch()) {
        _strReferenceAssetId =
            cleanedProximityAssetRef(_match.captured(1));
        _strSourceAssetId =
            cleanedProximityAssetRef(_match.captured(2));
    }

    static const QRegularExpression S_RE_AROUND_PATTERN(
        QString::fromUtf8(R"((?:哪些|哪个|找出|查询|筛选|所有|全部)?\s*([^0-9，,。？?]+?)\s*(?:周围|附近)\s*\d+(?:\.\d+)?\s*(?:公里|千米|km|KM|米|m)?\s*(?:内|以内|范围内)?\s*(?:没有|无|有|存在|包含|邻近|靠近)?\s*([^，,。？?]+))"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch _matchAround =
        S_RE_AROUND_PATTERN.match(_strText);
    if (_matchAround.hasMatch()) {
        if (_strSourceAssetId.trimmed().isEmpty()) {
            _strSourceAssetId =
                cleanedProximityAssetRef(_matchAround.captured(1));
        }
        if (_strReferenceAssetId.trimmed().isEmpty()) {
            _strReferenceAssetId =
                cleanedProximityAssetRef(_matchAround.captured(2));
        }
    }

    static const QRegularExpression S_RE_SOURCE_ID(
        R"((?:source_asset_id|源资产)\D{0,12}([A-Za-z0-9\-]+))",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch _matchSource = S_RE_SOURCE_ID.match(_strText);
    if (_matchSource.hasMatch()) {
        _strSourceAssetId = _matchSource.captured(1).trimmed();
    }

    static const QRegularExpression S_RE_REFERENCE_ID(
        R"((?:reference_asset_id|参考资产|参考图层)\D{0,12}([A-Za-z0-9\-]+))",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch _matchReference = S_RE_REFERENCE_ID.match(_strText);
    if (_matchReference.hasMatch()) {
        _strReferenceAssetId = _matchReference.captured(1).trimmed();
    }

    fillKnownProximityAliases(_strText, _strSourceAssetId, _strReferenceAssetId);

    return !_strSourceAssetId.trimmed().isEmpty()
        || !_strReferenceAssetId.trimmed().isEmpty();
}

AnalysisWorkflowCoordinator::TransitionResult
AnalysisWorkflowCoordinator::buildExecutionTransition() const
{
    TransitionResult _result;
    if (!mState.vMissingParams.isEmpty()) {
        return _result;
    }

    _result.kind = TransitionKind::ExecuteTool;
    _result.strToolName = currentToolName();
    if (mState.bHasSourceAssetId) {
        _result.jsonToolArgs["source_asset_id"] = mState.strSourceAssetId;
    }
    if (_result.strToolName == "run_frequency_statistics") {
        _result.jsonToolArgs["bin_count"] = mState.nBinCount;
    } else if (_result.strToolName == "run_neighborhood_analysis") {
        _result.jsonToolArgs["window_size"] = mState.nWindowSize;
    } else if (_result.strToolName == "run_buffer_analysis") {
        _result.jsonToolArgs["distance"] = mState.dBufferDistance;
        _result.jsonToolArgs["segments"] = mState.nBufferSegments;
    } else if (_result.strToolName == "run_overlay_analysis") {
        _result.jsonToolArgs["overlay_asset_id"] = mState.strOverlayAssetId;
        _result.jsonToolArgs["operation"] =
            overlayOperationToToolValue(mState.eOverlayOperation);
    } else if (_result.strToolName == "run_attribute_query") {
        _result.jsonToolArgs["field_name"] = mState.strQueryFieldName;
        _result.jsonToolArgs["operator"] = mState.strQueryOperatorId;
        _result.jsonToolArgs["value"] = mState.strQueryValueText;
    } else if (_result.strToolName == "run_spatial_query") {
        _result.jsonToolArgs["target_asset_id"] = mState.strSpatialTargetAssetId;
        _result.jsonToolArgs["relation"] = mState.strSpatialRelationId;
    } else if (_result.strToolName == "run_proximity_query") {
        _result.jsonToolArgs["reference_asset_id"] =
            mState.strProximityReferenceAssetId;
        _result.jsonToolArgs["distance"] = mState.dBufferDistance;
        _result.jsonToolArgs["segments"] = mState.nBufferSegments;
        _result.jsonToolArgs["invert_match"] = mState.bProximityInvertMatch;
    }
    return _result;
}

AnalysisWorkflowCoordinator::TransitionResult
AnalysisWorkflowCoordinator::applyStateUpdateArgs(const QJsonObject& _jsonArgs)
{
    if (_jsonArgs.contains("tool_name")) {
        const QString _strToolName = _jsonArgs["tool_name"].toString().trimmed();
        if (_strToolName == "run_basic_statistics") {
            mState.type = AnalysisTaskType::BasicStatistics;
        } else if (_strToolName == "run_frequency_statistics") {
            mState.type = AnalysisTaskType::FrequencyStatistics;
        } else if (_strToolName == "run_neighborhood_analysis") {
            mState.type = AnalysisTaskType::NeighborhoodAnalysis;
        } else if (_strToolName == "run_buffer_analysis") {
            mState.type = AnalysisTaskType::BufferAnalysis;
        } else if (_strToolName == "run_overlay_analysis") {
            mState.type = AnalysisTaskType::OverlayAnalysis;
        } else if (_strToolName == "run_attribute_query") {
            mState.type = AnalysisTaskType::AttributeQuery;
        } else if (_strToolName == "run_spatial_query") {
            mState.type = AnalysisTaskType::SpatialQuery;
        } else if (_strToolName == "run_proximity_query") {
            mState.type = AnalysisTaskType::ProximityQuery;
        }
    }

    if (_jsonArgs.contains("bin_count")) {
        const int _nBinCount = _jsonArgs["bin_count"].toInt(0);
        if (_nBinCount >= 2) {
            mState.bHasBinCount = true;
            mState.nBinCount = _nBinCount;
        }
    }

    if (_jsonArgs.contains("window_size")) {
        const int _nWindowSize = _jsonArgs["window_size"].toInt(0);
        if (_nWindowSize >= 3 && (_nWindowSize % 2) == 1) {
            mState.bHasWindowSize = true;
            mState.nWindowSize = _nWindowSize;
        }
    }

    if (_jsonArgs.contains("distance")) {
        const double _dBufferDistance = _jsonArgs["distance"].toDouble(0.0);
        if (_dBufferDistance > 0.0) {
            mState.bHasBufferDistance = true;
            mState.dBufferDistance = _dBufferDistance;
        }
    }

    if (_jsonArgs.contains("segments")) {
        const int _nBufferSegments = _jsonArgs["segments"].toInt(0);
        if (_nBufferSegments > 0) {
            mState.nBufferSegments = _nBufferSegments;
        }
    }

    if (_jsonArgs.contains("operation")) {
        OverlayOperationType _eOverlayOperation = OverlayOperationType::Intersect;
        if (parseOverlayOperationValue(
            _jsonArgs["operation"].toString(), _eOverlayOperation)) {
            mState.eOverlayOperation = _eOverlayOperation;
        }
    }

    if (_jsonArgs.contains("overlay_asset_id")) {
        const QString _strOverlayAssetId =
            _jsonArgs["overlay_asset_id"].toString().trimmed();
        if (!_strOverlayAssetId.isEmpty()) {
            mState.bHasOverlayAssetId = true;
            mState.strOverlayAssetId = _strOverlayAssetId;
        }
    }

    if (_jsonArgs.contains("field_name")) {
        const QString _strFieldName =
            _jsonArgs["field_name"].toString().trimmed();
        if (!_strFieldName.isEmpty()) {
            mState.bHasQueryFieldName = true;
            mState.strQueryFieldName = _strFieldName;
        }
    }

    if (_jsonArgs.contains("operator")) {
        const QString _strOperatorId =
            _jsonArgs["operator"].toString().trimmed();
        if (!_strOperatorId.isEmpty()) {
            mState.bHasQueryOperator = true;
            mState.strQueryOperatorId = _strOperatorId;
        }
    }

    if (_jsonArgs.contains("value")) {
        mState.bHasQueryValue = true;
        mState.strQueryValueText = _jsonArgs["value"].toString();
    }

    if (_jsonArgs.contains("target_asset_id")) {
        const QString _strTargetAssetId =
            _jsonArgs["target_asset_id"].toString().trimmed();
        if (!_strTargetAssetId.isEmpty()) {
            mState.bHasSpatialTargetAssetId = true;
            mState.strSpatialTargetAssetId = _strTargetAssetId;
        }
    }

    if (_jsonArgs.contains("source_asset_id")) {
        const QString _strSourceAssetId =
            _jsonArgs["source_asset_id"].toString().trimmed();
        if (!_strSourceAssetId.isEmpty()) {
            mState.bHasSourceAssetId = true;
            mState.strSourceAssetId = _strSourceAssetId;
        }
    }

    if (_jsonArgs.contains("reference_asset_id")) {
        const QString _strReferenceAssetId =
            _jsonArgs["reference_asset_id"].toString().trimmed();
        if (!_strReferenceAssetId.isEmpty()) {
            mState.bHasProximityReferenceAssetId = true;
            mState.strProximityReferenceAssetId = _strReferenceAssetId;
        }
    }

    if (_jsonArgs.contains("invert_match")) {
        mState.bProximityInvertMatch =
            _jsonArgs["invert_match"].toBool(false);
    }

    if (_jsonArgs.contains("relation")) {
        QString _strRelationId;
        if (parseSpatialRelationValue(
                _jsonArgs["relation"].toString(), _strRelationId)) {
            mState.strSpatialRelationId = _strRelationId;
        }
    }

    recalculateMissingParams();
    mState.strLastError.clear();
    return buildExecutionTransition();
}

QJsonArray AnalysisWorkflowCoordinator::extractRecentDialogue(
    const QJsonArray& _jsonRawHistory,
    const QString& _strLatestUserText) const
{
    QJsonArray _jsonRecent;
    int _nKept = 0;
    for (int _nMsgIdx = _jsonRawHistory.size() - 1;
        _nMsgIdx >= 0 && _nKept < 6; --_nMsgIdx) {
        const QJsonObject _jsonMsg = _jsonRawHistory[_nMsgIdx].toObject();
        const QString _strRole = _jsonMsg["role"].toString();
        if (_strRole != "user" && _strRole != "assistant") {
            continue;
        }

        const QString _strContent = _jsonMsg["content"].toString().trimmed();
        if (_strContent.isEmpty()) {
            continue;
        }
        if (_strRole == "user" && _strContent == _strLatestUserText.trimmed()) {
            continue;
        }

        _jsonRecent.prepend(_jsonMsg);
        ++_nKept;
    }
    return _jsonRecent;
}

QString AnalysisWorkflowCoordinator::buildStateSnapshot(
    const QString& _strLatestUserText) const
{
    QStringList _vLines;
    _vLines << QString::fromUtf8("当前分析状态");
    _vLines << QString::fromUtf8("任务类型：%1")
        .arg(analysisTaskTypeToString(mState.type));
    _vLines << QString::fromUtf8("已收集参数：");
    _vLines << buildCollectedParamLines();
    _vLines << QString::fromUtf8("缺失参数：");
    _vLines << buildMissingParamLines();
    if (!mState.strLastError.trimmed().isEmpty()) {
        _vLines << QString::fromUtf8("last_error：%1").arg(mState.strLastError);
    }
    _vLines << QString::fromUtf8("最新用户输入：%1").arg(_strLatestUserText.trimmed());
    _vLines << QString::fromUtf8("本轮任务：更新分析状态槽位。");
    return _vLines.join("\n");
}

QString AnalysisWorkflowCoordinator::buildCollectedParamLines() const
{
    QStringList _vLines;
    _vLines << QString::fromUtf8("  分析类型：%1")
        .arg(analysisTaskTypeToString(mState.type));
    if (mState.bHasBinCount) {
        _vLines << QString::fromUtf8("  分箱数：%1").arg(mState.nBinCount);
    }
    if (mState.bHasWindowSize) {
        _vLines << QString::fromUtf8("  邻域窗口：%1").arg(mState.nWindowSize);
    }
    if (mState.bHasBufferDistance) {
        _vLines << QString::fromUtf8("  缓冲距离：%1").arg(mState.dBufferDistance);
        _vLines << QString::fromUtf8("  缓冲分段数：%1").arg(mState.nBufferSegments);
    }
    if (mState.bHasOverlayAssetId) {
        _vLines << QString::fromUtf8("  叠加资产 ID：%1")
            .arg(mState.strOverlayAssetId);
    }
    if (mState.type == AnalysisTaskType::OverlayAnalysis) {
        _vLines << QString::fromUtf8("  叠加操作：%1")
            .arg(overlayOperationToDisplayName(mState.eOverlayOperation));
    }
    if (mState.bHasQueryFieldName) {
        _vLines << QString::fromUtf8("  查询字段：%1").arg(mState.strQueryFieldName);
    }
    if (mState.bHasQueryOperator) {
        _vLines << QString::fromUtf8("  查询运算符：%1").arg(mState.strQueryOperatorId);
    }
    if (mState.bHasQueryValue) {
        _vLines << QString::fromUtf8("  查询值：%1").arg(mState.strQueryValueText);
    }
    if (mState.bHasSpatialTargetAssetId) {
        _vLines << QString::fromUtf8("  指定区域资产 ID：%1")
            .arg(mState.strSpatialTargetAssetId);
        _vLines << QString::fromUtf8("  空间关系：%1")
            .arg(mState.strSpatialRelationId);
    }
    if (mState.bHasSourceAssetId) {
        _vLines << QString::fromUtf8("  源资产 ID：%1")
            .arg(mState.strSourceAssetId);
    }
    if (mState.bHasProximityReferenceAssetId) {
        _vLines << QString::fromUtf8("  邻近参考资产 ID：%1")
            .arg(mState.strProximityReferenceAssetId);
    }
    if (mState.type == AnalysisTaskType::ProximityQuery) {
        _vLines << QString::fromUtf8("  邻近匹配模式：%1")
            .arg(mState.bProximityInvertMatch
                ? QString::fromUtf8("距离范围外")
                : QString::fromUtf8("距离范围内"));
    }
    return _vLines.join("\n");
}

QString AnalysisWorkflowCoordinator::buildMissingParamLines() const
{
    QStringList _vLines;
    for (const QString& _strParam : mState.vMissingParams) {
        if (_strParam == "task_type") {
            _vLines << QString::fromUtf8("分析类型：例如 基础统计 / 频率统计 / 邻域分析 / 缓冲区分析 / 叠加分析");
        } else if (_strParam == "bin_count") {
            _vLines << QString::fromUtf8("分箱数：例如 10");
        } else if (_strParam == "window_size") {
            _vLines << QString::fromUtf8("邻域窗口：例如 3 或 5");
        } else if (_strParam == "distance") {
            _vLines << QString::fromUtf8("距离：例如 100 米 或 1 公里");
        } else if (_strParam == "overlay_asset_id") {
            _vLines << buildOverlayCandidateLines();
        } else if (_strParam == "field_name") {
            _vLines << QString::fromUtf8("字段名：例如 score 或 population");
        } else if (_strParam == "operator") {
            _vLines << QString::fromUtf8("运算符：= / != / > / >= / < / <= / contains");
        } else if (_strParam == "value") {
            _vLines << QString::fromUtf8("查询值：例如 100000 或 school");
        } else if (_strParam == "target_asset_id") {
            _vLines << buildSpatialTargetCandidateLines();
        } else if (_strParam == "source_asset_id") {
            _vLines << QString::fromUtf8("被筛选资产：例如 学校 / schools / 建筑 / buildings");
        } else if (_strParam == "reference_asset_id") {
            _vLines << QString::fromUtf8("参考资产：例如 医院 / hospitals / 道路 / roads");
        }
    }

    if (_vLines.isEmpty()) {
        _vLines << QString::fromUtf8("参数已完整，正在准备执行。");
    }
    return _vLines.join("\n");
}

QStringList AnalysisWorkflowCoordinator::buildOverlayCandidateLines() const
{
    QStringList _vLines;
    if (mpToolHost == nullptr) {
        _vLines << QString::fromUtf8("叠加资产：请指定另一个已导入矢量资产的 overlay_asset_id");
        return _vLines;
    }

    const QJsonObject _jsonContext = mpToolHost->getAnalysisContext();
    const QString _strSelectedAssetId =
        _jsonContext["selected_asset_id"].toString().trimmed();
    QStringList _vCandidates;
    for (const QJsonValue& _jsonVal : _jsonContext["assets"].toArray()) {
        const QJsonObject _jsonAsset = _jsonVal.toObject();
        const QString _strAssetId = _jsonAsset["id"].toString().trimmed();
        if (_strAssetId.isEmpty()
            || _strAssetId == _strSelectedAssetId
            || !assetHasCapability(_jsonAsset, QStringLiteral("spatial_vector"))) {
            continue;
        }

        _vCandidates << QString::fromUtf8("%1（ID：%2）")
            .arg(_jsonAsset["name"].toString(), _strAssetId);
    }

    if (_vCandidates.isEmpty()) {
        _vLines << QString::fromUtf8("叠加资产：请先导入另一个矢量资产");
    } else {
        _vLines << QString::fromUtf8("叠加资产：请选择一个候选 overlay_asset_id");
        for (const QString& _strCandidate : _vCandidates) {
            _vLines << QString::fromUtf8("  - %1").arg(_strCandidate);
        }
    }
    return _vLines;
}

QStringList AnalysisWorkflowCoordinator::buildSpatialTargetCandidateLines() const
{
    QStringList _vLines;
    if (mpToolHost == nullptr) {
        _vLines << QString::fromUtf8("指定区域：请指定另一个面状矢量资产的 target_asset_id");
        return _vLines;
    }

    const QJsonObject _jsonContext = mpToolHost->getAnalysisContext();
    const QString _strSelectedAssetId =
        _jsonContext["selected_asset_id"].toString().trimmed();
    QStringList _vCandidates;
    for (const QJsonValue& _jsonVal : _jsonContext["assets"].toArray()) {
        const QJsonObject _jsonAsset = _jsonVal.toObject();
        const QString _strAssetId = _jsonAsset["id"].toString().trimmed();
        if (_strAssetId.isEmpty()
            || _strAssetId == _strSelectedAssetId
            || !assetIsAreaVector(_jsonAsset)) {
            continue;
        }

        _vCandidates << QString::fromUtf8("%1（ID：%2）")
            .arg(_jsonAsset["name"].toString(), _strAssetId);
    }

    if (_vCandidates.isEmpty()) {
        _vLines << QString::fromUtf8("指定区域：请先导入另一个面状矢量资产");
    } else {
        _vLines << QString::fromUtf8("指定区域：请选择一个候选 target_asset_id");
        for (const QString& _strCandidate : _vCandidates) {
            _vLines << QString::fromUtf8("  - %1").arg(_strCandidate);
        }
    }
    return _vLines;
}

QString AnalysisWorkflowCoordinator::currentToolName() const
{
    switch (mState.type) {
    case AnalysisTaskType::BasicStatistics:
        return "run_basic_statistics";
    case AnalysisTaskType::FrequencyStatistics:
        return "run_frequency_statistics";
    case AnalysisTaskType::NeighborhoodAnalysis:
        return "run_neighborhood_analysis";
    case AnalysisTaskType::BufferAnalysis:
        return "run_buffer_analysis";
    case AnalysisTaskType::OverlayAnalysis:
        return "run_overlay_analysis";
    case AnalysisTaskType::AttributeQuery:
        return "run_attribute_query";
    case AnalysisTaskType::SpatialQuery:
        return "run_spatial_query";
    case AnalysisTaskType::ProximityQuery:
        return "run_proximity_query";
    case AnalysisTaskType::None:
    default:
        return QString();
    }
}

bool AnalysisWorkflowCoordinator::isCancelText(const QString& _strText)
{
    const QString _strNormalized = _strText.trimmed().toLower();
    return _strNormalized == QString::fromUtf8("取消")
        || _strNormalized == QString::fromUtf8("取消分析")
        || _strNormalized == "cancel";
}

bool AnalysisWorkflowCoordinator::isCapabilityIntent(const QString& _strText)
{
    const QString _strNormalized = _strText.toLower();
    return _strNormalized.contains(QString::fromUtf8("你能做什么"))
        || _strNormalized.contains(QString::fromUtf8("你可以做什么"))
        || _strNormalized.contains(QString::fromUtf8("功能"))
        || _strNormalized.contains(QString::fromUtf8("帮助"));
}

bool AnalysisWorkflowCoordinator::looksLikeStateUpdateInput(
    const QString& _strText) const
{
    if (detectTaskType(_strText) != AnalysisTaskType::None) {
        return true;
    }
    if (_strText.contains(QString::fromUtf8("分箱"))
        || _strText.contains("bin", Qt::CaseInsensitive)
        || _strText.contains(QString::fromUtf8("窗口"))
        || _strText.contains("window", Qt::CaseInsensitive)
        || _strText.contains(QString::fromUtf8("缓冲距离"))
        || _strText.contains(QString::fromUtf8("距离"))
        || _strText.contains("distance", Qt::CaseInsensitive)
        || _strText.contains("segment", Qt::CaseInsensitive)
        || _strText.contains("overlay_asset_id", Qt::CaseInsensitive)
        || _strText.contains("operation", Qt::CaseInsensitive)
        || _strText.contains("intersect", Qt::CaseInsensitive)
        || _strText.contains("union", Qt::CaseInsensitive)
        || _strText.contains("asset_id", Qt::CaseInsensitive)
        || _strText.contains("field_name", Qt::CaseInsensitive)
        || _strText.contains("operator", Qt::CaseInsensitive)
        || _strText.contains("target_asset_id", Qt::CaseInsensitive)
        || _strText.contains("source_asset_id", Qt::CaseInsensitive)
        || _strText.contains("reference_asset_id", Qt::CaseInsensitive)
        || _strText.contains("relation", Qt::CaseInsensitive)
        || _strText.contains(QString::fromUtf8("属性查询"))
        || _strText.contains(QString::fromUtf8("空间查询"))
        || _strText.contains(QString::fromUtf8("字段"))
        || _strText.contains(QString::fromUtf8("运算符"))
        || _strText.contains(QString::fromUtf8("找出"))
        || _strText.contains(QString::fromUtf8("邻近"))
        || _strText.contains(QString::fromUtf8("参考"))
        || _strText.contains(QString::fromUtf8("周围"))
        || _strText.contains(QString::fromUtf8("附近"))
        || _strText.contains(QString::fromUtf8("公里"))
        || _strText.contains(QString::fromUtf8("学校"))
        || _strText.contains(QString::fromUtf8("医院"))
        || _strText.contains(QString::fromUtf8("指定区域"))
        || _strText.contains(QString::fromUtf8("叠加资产"))
        || _strText.contains(QString::fromUtf8("交集"))
        || _strText.contains(QString::fromUtf8("相交"))
        || _strText.contains(QString::fromUtf8("联合"))
        || _strText.contains(QString::fromUtf8("并集"))) {
        return true;
    }
    if (mState.type == AnalysisTaskType::OverlayAnalysis
        && QRegularExpression(R"(^\s*[A-Za-z0-9][A-Za-z0-9\-]{5,}\s*$)")
            .match(_strText).hasMatch()) {
        return true;
    }
    if (!_strText.trimmed().isEmpty()
        && QRegularExpression("^\\d+$").match(_strText.trimmed()).hasMatch()) {
        return true;
    }
    return false;
}

AnalysisWorkflowCoordinator::AnalysisTaskType
AnalysisWorkflowCoordinator::detectTaskType(const QString& _strUserText)
{
    const QString _strText = _strUserText.toLower();
    const bool _bQueryIntent = _strText.contains(QString::fromUtf8("查询"))
        || _strText.contains(QString::fromUtf8("筛选"))
        || _strText.contains(QString::fromUtf8("找出"))
        || _strText.contains(QString::fromUtf8("哪些"))
        || _strText.contains(QString::fromUtf8("哪个"))
        || _strText.contains("query")
        || _strText.contains("filter");
    const bool _bProximityIntent = _strText.contains(QString::fromUtf8("距离"))
        || _strText.contains(QString::fromUtf8("邻近"))
        || _strText.contains(QString::fromUtf8("周围"))
        || _strText.contains(QString::fromUtf8("附近"))
        || _strText.contains(QString::fromUtf8("公里"))
        || _strText.contains(QString::fromUtf8("米内"))
        || _strText.contains("near")
        || _strText.contains("proximity");
    if (_bQueryIntent && _bProximityIntent) {
        return AnalysisTaskType::ProximityQuery;
    }
    if (_bQueryIntent
        && (_strText.contains(QString::fromUtf8("空间"))
            || _strText.contains(QString::fromUtf8("区域"))
            || _strText.contains(QString::fromUtf8("相交"))
            || _strText.contains(QString::fromUtf8("范围"))
            || _strText.contains("intersect")
            || _strText.contains("within")
            || _strText.contains("contains"))) {
        return AnalysisTaskType::SpatialQuery;
    }
    if (_bQueryIntent
        || _strText.contains(QString::fromUtf8("属性查询"))
        || _strText.contains(QString::fromUtf8("按属性"))
        || _strText.contains("where")) {
        return AnalysisTaskType::AttributeQuery;
    }
    if (_strText.contains(QString::fromUtf8("叠加"))
        || _strText.contains(QString::fromUtf8("交集"))
        || _strText.contains(QString::fromUtf8("相交"))
        || _strText.contains(QString::fromUtf8("联合"))
        || _strText.contains(QString::fromUtf8("并集"))
        || _strText.contains("overlay")
        || _strText.contains("intersect")
        || _strText.contains("intersection")
        || _strText.contains("union")) {
        return AnalysisTaskType::OverlayAnalysis;
    }
    if (_strText.contains(QString::fromUtf8("邻域"))
        || _strText.contains("neighborhood")) {
        return AnalysisTaskType::NeighborhoodAnalysis;
    }
    if (_strText.contains(QString::fromUtf8("缓冲"))
        || _strText.contains("buffer")) {
        return AnalysisTaskType::BufferAnalysis;
    }
    if (_strText.contains(QString::fromUtf8("频率"))
        || _strText.contains(QString::fromUtf8("直方"))
        || _strText.contains("frequency")) {
        return AnalysisTaskType::FrequencyStatistics;
    }
    if (_strText.contains(QString::fromUtf8("基础统计"))
        || _strText.contains(QString::fromUtf8("统计摘要"))
        || _strText.contains(QString::fromUtf8("最小值"))
        || _strText.contains("basic statistics")) {
        return AnalysisTaskType::BasicStatistics;
    }
    return AnalysisTaskType::None;
}

bool AnalysisWorkflowCoordinator::tryExtractBinCount(const QString& _strText,
    int& _nBinCount)
{
    static const QRegularExpression S_RE_BIN(
        R"((?:分箱数|分箱|bin(?:_count)?)\D{0,6}(\d+))",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch _match = S_RE_BIN.match(_strText);
    if (!_match.hasMatch()) {
        return false;
    }

    _nBinCount = _match.captured(1).toInt();
    return _nBinCount >= 2;
}

bool AnalysisWorkflowCoordinator::tryExtractWindowSize(const QString& _strText,
    int& _nWindowSize)
{
    static const QRegularExpression S_RE_WINDOW(
        R"((?:窗口大小|邻域窗口|窗口|window(?:_size)?)\D{0,6}(\d+))",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch _match = S_RE_WINDOW.match(_strText);
    if (!_match.hasMatch()) {
        return false;
    }

    _nWindowSize = _match.captured(1).toInt();
    return _nWindowSize >= 3 && (_nWindowSize % 2) == 1;
}

bool AnalysisWorkflowCoordinator::tryExtractBufferDistance(const QString& _strText,
    double& _dBufferDistance)
{
    static const QRegularExpression S_RE_DISTANCE_AFTER(
        QString::fromUtf8(R"((?:缓冲距离|距离|缓冲|周围|附近|buffer(?:_distance)?|distance)\D{0,12}(-?\d+(?:\.\d+)?)\s*(公里|千米|km|KM|米|m|M)?)"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression S_RE_DISTANCE_BEFORE(
        QString::fromUtf8(R"((-?\d+(?:\.\d+)?)\s*(公里|千米|km|KM|米|m|M)?\D{0,8}(?:缓冲|buffer|周围|附近|内|以内))"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression S_RE_DISTANCE_WITH_UNIT(
        QString::fromUtf8(R"((-?\d+(?:\.\d+)?)\s*(公里|千米|km|KM|米|m|M)\s*(?:内|以内|范围内)?)"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression S_RE_NUMBER_ONLY(
        R"(^\s*(-?\d+(?:\.\d+)?)\s*$)",
        QRegularExpression::CaseInsensitiveOption);
    auto _distanceScale = [](const QString& _strUnit) -> double {
        const QString _strNormalized = _strUnit.trimmed().toLower();
        if (_strNormalized == QString::fromUtf8("公里")
            || _strNormalized == QString::fromUtf8("千米")
            || _strNormalized == QStringLiteral("km")) {
            return 1000.0;
        }
        return 1.0;
    };

    QRegularExpressionMatch _match = S_RE_DISTANCE_AFTER.match(_strText);
    if (!_match.hasMatch()) {
        _match = S_RE_DISTANCE_BEFORE.match(_strText);
    }
    if (!_match.hasMatch()) {
        _match = S_RE_DISTANCE_WITH_UNIT.match(_strText);
    }
    if (!_match.hasMatch()) {
        _match = S_RE_NUMBER_ONLY.match(_strText);
    }
    if (!_match.hasMatch()) {
        return false;
    }

    _dBufferDistance = _match.captured(1).toDouble()
        * _distanceScale(_match.lastCapturedIndex() >= 2
            ? _match.captured(2)
            : QString());
    return _dBufferDistance > 0.0;
}

bool AnalysisWorkflowCoordinator::tryExtractBufferSegments(const QString& _strText,
    int& _nBufferSegments)
{
    static const QRegularExpression S_RE_SEGMENTS(
        R"((?:圆弧分段数|分段数|分段|segments?)\D{0,6}(\d+))",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch _match = S_RE_SEGMENTS.match(_strText);
    if (!_match.hasMatch()) {
        return false;
    }

    _nBufferSegments = _match.captured(1).toInt();
    return _nBufferSegments > 0;
}

bool AnalysisWorkflowCoordinator::tryExtractOverlayAssetId(const QString& _strText,
    QString& _strOverlayAssetId)
{
    static const QRegularExpression S_RE_OVERLAY_ID(
        R"((?:overlay_asset_id|asset_id|资产ID|资产编号|叠加资产)\D{0,12}([A-Za-z0-9\-]+))",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression S_RE_ID_ONLY(
        R"(^\s*([A-Za-z0-9][A-Za-z0-9\-]{5,})\s*$)",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch _match = S_RE_OVERLAY_ID.match(_strText);
    if (_match.hasMatch()) {
        _strOverlayAssetId = _match.captured(1).trimmed();
        return !_strOverlayAssetId.isEmpty();
    }

    const QRegularExpressionMatch _matchIdOnly = S_RE_ID_ONLY.match(_strText);
    if (!_matchIdOnly.hasMatch()) {
        return false;
    }

    _strOverlayAssetId = _matchIdOnly.captured(1).trimmed();
    return !_strOverlayAssetId.isEmpty();
}

bool AnalysisWorkflowCoordinator::tryExtractOverlayOperation(
    const QString& _strText,
    OverlayOperationType& _eOperation)
{
    const QString _strNormalized = _strText.toLower();
    if (_strNormalized.contains(QString::fromUtf8("联合"))
        || _strNormalized.contains(QString::fromUtf8("并集"))
        || _strNormalized.contains("union")) {
        _eOperation = OverlayOperationType::Union;
        return true;
    }

    if (_strNormalized.contains(QString::fromUtf8("交集"))
        || _strNormalized.contains(QString::fromUtf8("相交"))
        || _strNormalized.contains("intersect")
        || _strNormalized.contains("intersection")) {
        _eOperation = OverlayOperationType::Intersect;
        return true;
    }

    static const QRegularExpression S_RE_OPERATION(
        R"((?:operation|叠加操作|操作)\D{0,8}(intersect|intersection|union))",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch _match = S_RE_OPERATION.match(_strText);
    if (!_match.hasMatch()) {
        return false;
    }

    return parseOverlayOperationValue(_match.captured(1), _eOperation);
}

bool AnalysisWorkflowCoordinator::tryExtractAttributeQueryParts(
    const QString& _strText,
    QString& _strFieldName,
    QString& _strOperatorId,
    QString& _strValueText)
{
    static const QRegularExpression S_RE_ATTRIBUTE_CONDITION(
        R"(([A-Za-z_][A-Za-z0-9_]*)\s*(>=|<=|!=|<>|=|>|<|contains)\s*([^\s，,。]+))",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch _match = S_RE_ATTRIBUTE_CONDITION.match(_strText);
    if (!_match.hasMatch()) {
        return false;
    }

    _strFieldName = _match.captured(1).trimmed();
    _strOperatorId = _match.captured(2).trimmed();
    _strValueText = _match.captured(3).trimmed();
    return !_strFieldName.isEmpty()
        && !_strOperatorId.isEmpty()
        && !_strValueText.isEmpty();
}

bool AnalysisWorkflowCoordinator::tryExtractSpatialTargetAssetId(
    const QString& _strText,
    QString& _strTargetAssetId)
{
    static const QRegularExpression S_RE_TARGET_ID(
        R"((?:target_asset_id|asset_id|资产ID|资产编号|指定区域|区域资产)\D{0,12}([A-Za-z0-9\-]+))",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch _match = S_RE_TARGET_ID.match(_strText);
    if (!_match.hasMatch()) {
        return false;
    }

    _strTargetAssetId = _match.captured(1).trimmed();
    return !_strTargetAssetId.isEmpty();
}

bool AnalysisWorkflowCoordinator::tryExtractSpatialRelation(
    const QString& _strText,
    QString& _strRelationId)
{
    const QString _strNormalized = _strText.toLower();
    if (_strNormalized.contains(QString::fromUtf8("相交"))
        || _strNormalized.contains("intersect")) {
        _strRelationId = QStringLiteral("intersects");
        return true;
    }
    if (_strNormalized.contains(QString::fromUtf8("包含"))
        || _strNormalized.contains("contains")) {
        _strRelationId = QStringLiteral("contains");
        return true;
    }
    if (_strNormalized.contains(QString::fromUtf8("范围内"))
        || _strNormalized.contains(QString::fromUtf8("位于"))
        || _strNormalized.contains("within")) {
        _strRelationId = QStringLiteral("within");
        return true;
    }

    static const QRegularExpression S_RE_RELATION(
        R"((?:relation|空间关系|关系)\D{0,8}(intersects?|within|contains))",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch _match = S_RE_RELATION.match(_strText);
    if (!_match.hasMatch()) {
        return false;
    }

    return parseSpatialRelationValue(_match.captured(1), _strRelationId);
}
