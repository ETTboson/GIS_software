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
    case AnalysisWorkflowCoordinator::AnalysisTaskType::Buffer:
        return QString::fromUtf8("缓冲分析");
    case AnalysisWorkflowCoordinator::AnalysisTaskType::Overlay:
        return QString::fromUtf8("叠加分析");
    case AnalysisWorkflowCoordinator::AnalysisTaskType::SpatialQuery:
        return QString::fromUtf8("空间查询");
    case AnalysisWorkflowCoordinator::AnalysisTaskType::RasterCalc:
        return QString::fromUtf8("栅格计算");
    case AnalysisWorkflowCoordinator::AnalysisTaskType::None:
    default:
        return QString::fromUtf8("未确定");
    }
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
            "1. 缓冲分析\n"
            "2. 叠加分析\n"
            "3. 空间查询\n"
            "4. 栅格计算");
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
            "run_buffer_analysis",
            "run_overlay_analysis",
            "run_spatial_query",
            "run_raster_calc"
        };

        QJsonObject _radiusProp;
        _radiusProp["type"] = "number";

        QJsonObject _overlayProp;
        _overlayProp["type"] = "string";
        _overlayProp["enum"] = QJsonArray{
            "intersection",
            "union",
            "difference"
        };

        QJsonObject _queryProp;
        _queryProp["type"] = "string";

        QJsonObject _expressionProp;
        _expressionProp["type"] = "string";

        QJsonObject _properties;
        _properties["tool_name"] = _toolNameProp;
        _properties["radius_meters"] = _radiusProp;
        _properties["overlay_type"] = _overlayProp;
        _properties["query_expression"] = _queryProp;
        _properties["expression"] = _expressionProp;

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
        case AnalysisTaskType::Buffer:
            _jsonArgs["tool_name"] = "run_buffer_analysis";
            break;
        case AnalysisTaskType::Overlay:
            _jsonArgs["tool_name"] = "run_overlay_analysis";
            break;
        case AnalysisTaskType::SpatialQuery:
            _jsonArgs["tool_name"] = "run_spatial_query";
            break;
        case AnalysisTaskType::RasterCalc:
            _jsonArgs["tool_name"] = "run_raster_calc";
            break;
        case AnalysisTaskType::None:
        default:
            break;
        }
    }

    double _dRadiusMeters = 0.0;
    if (tryExtractRadiusMeters(_strUserText, _dRadiusMeters)) {
        _jsonArgs["radius_meters"] = _dRadiusMeters;
    }

    const QString _strOverlayType = normalizeOverlayType(_strUserText);
    if (!_strOverlayType.isEmpty()) {
        _jsonArgs["overlay_type"] = _strOverlayType;
    }

    QString _strQueryExpression;
    if (tryExtractQueryExpression(_strUserText, _strQueryExpression)) {
        _jsonArgs["query_expression"] = _strQueryExpression;
    }

    QString _strRasterExpression;
    if (tryExtractRasterExpression(_strUserText, _strRasterExpression)) {
        _jsonArgs["expression"] = _strRasterExpression;
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
}

void AnalysisWorkflowCoordinator::recalculateMissingParams()
{
    mState.vMissingParams.clear();
    switch (mState.type) {
    case AnalysisTaskType::Buffer:
        if (!mState.bHasRadius) {
            mState.vMissingParams << "radius_meters";
        }
        break;
    case AnalysisTaskType::Overlay:
        if (mState.strOverlayType.trimmed().isEmpty()) {
            mState.vMissingParams << "overlay_type";
        }
        break;
    case AnalysisTaskType::SpatialQuery:
        if (mState.strQueryExpression.trimmed().isEmpty()) {
            mState.vMissingParams << "query_expression";
        }
        break;
    case AnalysisTaskType::RasterCalc:
        if (mState.strRasterExpression.trimmed().isEmpty()) {
            mState.vMissingParams << "expression";
        }
        break;
    case AnalysisTaskType::None:
    default:
        mState.vMissingParams << "task_type";
        break;
    }
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
    if (_result.strToolName == "run_buffer_analysis") {
        _result.jsonToolArgs["radius_meters"] = mState.dRadiusMeters;
    } else if (_result.strToolName == "run_overlay_analysis") {
        _result.jsonToolArgs["overlay_type"] = mState.strOverlayType;
    } else if (_result.strToolName == "run_spatial_query") {
        _result.jsonToolArgs["query_expression"] = mState.strQueryExpression;
    } else if (_result.strToolName == "run_raster_calc") {
        _result.jsonToolArgs["expression"] = mState.strRasterExpression;
    }
    return _result;
}

AnalysisWorkflowCoordinator::TransitionResult
AnalysisWorkflowCoordinator::applyStateUpdateArgs(const QJsonObject& _jsonArgs)
{
    if (_jsonArgs.contains("tool_name")) {
        const QString _strToolName = _jsonArgs["tool_name"].toString().trimmed();
        if (_strToolName == "run_buffer_analysis") {
            mState.type = AnalysisTaskType::Buffer;
        } else if (_strToolName == "run_overlay_analysis") {
            mState.type = AnalysisTaskType::Overlay;
        } else if (_strToolName == "run_spatial_query") {
            mState.type = AnalysisTaskType::SpatialQuery;
        } else if (_strToolName == "run_raster_calc") {
            mState.type = AnalysisTaskType::RasterCalc;
        }
    }

    if (_jsonArgs.contains("radius_meters")) {
        const double _dRadiusMeters = _jsonArgs["radius_meters"].toDouble(0.0);
        if (_dRadiusMeters > 0.0) {
            mState.bHasRadius = true;
            mState.dRadiusMeters = _dRadiusMeters;
        }
    }

    if (_jsonArgs.contains("overlay_type")) {
        mState.strOverlayType = normalizeOverlayType(
            _jsonArgs["overlay_type"].toString());
    }

    if (_jsonArgs.contains("query_expression")) {
        mState.strQueryExpression = normalizeExpression(
            _jsonArgs["query_expression"].toString());
    }

    if (_jsonArgs.contains("expression")) {
        mState.strRasterExpression = normalizeExpression(
            _jsonArgs["expression"].toString());
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
    if (mState.bHasRadius) {
        _vLines << QString::fromUtf8("  缓冲半径：%1 米")
            .arg(mState.dRadiusMeters, 0, 'f', 2);
    }
    if (!mState.strOverlayType.isEmpty()) {
        _vLines << QString::fromUtf8("  叠加类型：%1").arg(mState.strOverlayType);
    }
    if (!mState.strQueryExpression.isEmpty()) {
        _vLines << QString::fromUtf8("  查询表达式：%1")
            .arg(mState.strQueryExpression);
    }
    if (!mState.strRasterExpression.isEmpty()) {
        _vLines << QString::fromUtf8("  栅格表达式：%1")
            .arg(mState.strRasterExpression);
    }
    return _vLines.join("\n");
}

QString AnalysisWorkflowCoordinator::buildMissingParamLines() const
{
    QStringList _vLines;
    for (const QString& _strParam : mState.vMissingParams) {
        if (_strParam == "task_type") {
            _vLines << QString::fromUtf8("分析类型：例如 缓冲分析 / 叠加分析 / 空间查询 / 栅格计算");
        } else if (_strParam == "radius_meters") {
            _vLines << QString::fromUtf8("缓冲半径：例如 500 米");
        } else if (_strParam == "overlay_type") {
            _vLines << QString::fromUtf8("叠加类型：例如 intersection、union、difference");
        } else if (_strParam == "query_expression") {
            _vLines << QString::fromUtf8("查询表达式：例如 population > 1000");
        } else if (_strParam == "expression") {
            _vLines << QString::fromUtf8("栅格表达式：例如 (A + B) / 2");
        }
    }

    if (_vLines.isEmpty()) {
        _vLines << QString::fromUtf8("参数已完整，正在准备执行。");
    }
    return _vLines.join("\n");
}

QString AnalysisWorkflowCoordinator::currentToolName() const
{
    switch (mState.type) {
    case AnalysisTaskType::Buffer:
        return "run_buffer_analysis";
    case AnalysisTaskType::Overlay:
        return "run_overlay_analysis";
    case AnalysisTaskType::SpatialQuery:
        return "run_spatial_query";
    case AnalysisTaskType::RasterCalc:
        return "run_raster_calc";
    case AnalysisTaskType::None:
    default:
        return QString();
    }
}

QString AnalysisWorkflowCoordinator::normalizeOverlayType(const QString& _strValue)
{
    const QString _strText = _strValue.trimmed().toLower();
    if (_strText.contains(QString::fromUtf8("交集"))
        || _strText.contains("intersection")) {
        return "intersection";
    }
    if (_strText.contains(QString::fromUtf8("并集"))
        || _strText.contains("union")) {
        return "union";
    }
    if (_strText.contains(QString::fromUtf8("差集"))
        || _strText.contains("difference")) {
        return "difference";
    }
    return QString();
}

QString AnalysisWorkflowCoordinator::normalizeExpression(const QString& _strValue)
{
    QString _strText = _strValue.trimmed();
    if (_strText.startsWith('"') && _strText.endsWith('"') && _strText.length() > 1) {
        _strText = _strText.mid(1, _strText.length() - 2).trimmed();
    }
    return _strText;
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
    if (normalizeOverlayType(_strText).length() > 0) {
        return true;
    }
    if (_strText.contains(QString::fromUtf8("半径"))
        || _strText.contains("radius", Qt::CaseInsensitive)
        || _strText.contains(QString::fromUtf8("表达式"))
        || _strText.contains("=")
        || _strText.contains(">")
        || _strText.contains("<")) {
        return true;
    }
    return false;
}

AnalysisWorkflowCoordinator::AnalysisTaskType
AnalysisWorkflowCoordinator::detectTaskType(const QString& _strUserText)
{
    const QString _strText = _strUserText.toLower();
    if (_strText.contains(QString::fromUtf8("栅格计算"))
        || _strText.contains("raster")) {
        return AnalysisTaskType::RasterCalc;
    }
    if (_strText.contains(QString::fromUtf8("空间查询"))
        || _strText.contains("spatial query")) {
        return AnalysisTaskType::SpatialQuery;
    }
    if (_strText.contains(QString::fromUtf8("叠加"))
        || _strText.contains(QString::fromUtf8("并集"))
        || _strText.contains(QString::fromUtf8("交集"))
        || _strText.contains(QString::fromUtf8("差集"))
        || _strText.contains("overlay")
        || _strText.contains("union")
        || _strText.contains("intersection")
        || _strText.contains("difference")) {
        return AnalysisTaskType::Overlay;
    }
    if (_strText.contains(QString::fromUtf8("缓冲"))
        || _strText.contains("buffer")) {
        return AnalysisTaskType::Buffer;
    }
    if (_strText.contains(QString::fromUtf8("查询表达式"))
        || _strText.contains(QString::fromUtf8("查询条件"))) {
        return AnalysisTaskType::SpatialQuery;
    }
    return AnalysisTaskType::None;
}

bool AnalysisWorkflowCoordinator::tryExtractRadiusMeters(
    const QString& _strText,
    double& _dRadiusMeters)
{
    static const QRegularExpression S_RE_RADIUS(
        R"((?:半径|radius)[^0-9]{0,8}(\d+(?:\.\d+)?))",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch _match = S_RE_RADIUS.match(_strText);
    if (!_match.hasMatch()) {
        return false;
    }

    _dRadiusMeters = _match.captured(1).toDouble();
    return _dRadiusMeters > 0.0;
}

bool AnalysisWorkflowCoordinator::tryExtractQueryExpression(
    const QString& _strText,
    QString& _strExpression)
{
    static const QRegularExpression S_RE_EXPR(
        R"((?:查询表达式|查询条件|expression)\s*[:：=]?\s*(.+)$)",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch _match = S_RE_EXPR.match(_strText.trimmed());
    if (_match.hasMatch()) {
        _strExpression = normalizeExpression(_match.captured(1));
        return !_strExpression.isEmpty();
    }

    if (_strText.contains("=") || _strText.contains(">")
        || _strText.contains("<")) {
        _strExpression = normalizeExpression(_strText.trimmed());
        return !_strExpression.isEmpty();
    }
    return false;
}

bool AnalysisWorkflowCoordinator::tryExtractRasterExpression(
    const QString& _strText,
    QString& _strExpression)
{
    static const QRegularExpression S_RE_EXPR(
        R"((?:栅格表达式|栅格计算表达式|表达式|expression)\s*[:：=]?\s*(.+)$)",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch _match = S_RE_EXPR.match(_strText.trimmed());
    if (!_match.hasMatch()) {
        return false;
    }

    _strExpression = normalizeExpression(_match.captured(1));
    return !_strExpression.isEmpty();
}
