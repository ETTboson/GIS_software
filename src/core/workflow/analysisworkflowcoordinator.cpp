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
            "1. 基础统计\n"
            "2. 频率统计\n"
            "3. 邻域分析");
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
            "run_neighborhood_analysis"
        };

        QJsonObject _binProp;
        _binProp["type"] = "integer";

        QJsonObject _windowProp;
        _windowProp["type"] = "integer";

        QJsonObject _properties;
        _properties["tool_name"] = _toolNameProp;
        _properties["bin_count"] = _binProp;
        _properties["window_size"] = _windowProp;

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
    if (_result.strToolName == "run_frequency_statistics") {
        _result.jsonToolArgs["bin_count"] = mState.nBinCount;
    } else if (_result.strToolName == "run_neighborhood_analysis") {
        _result.jsonToolArgs["window_size"] = mState.nWindowSize;
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
    return _vLines.join("\n");
}

QString AnalysisWorkflowCoordinator::buildMissingParamLines() const
{
    QStringList _vLines;
    for (const QString& _strParam : mState.vMissingParams) {
        if (_strParam == "task_type") {
            _vLines << QString::fromUtf8("分析类型：例如 基础统计 / 频率统计 / 邻域分析");
        } else if (_strParam == "bin_count") {
            _vLines << QString::fromUtf8("分箱数：例如 10");
        } else if (_strParam == "window_size") {
            _vLines << QString::fromUtf8("邻域窗口：例如 3 或 5");
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
    case AnalysisTaskType::BasicStatistics:
        return "run_basic_statistics";
    case AnalysisTaskType::FrequencyStatistics:
        return "run_frequency_statistics";
    case AnalysisTaskType::NeighborhoodAnalysis:
        return "run_neighborhood_analysis";
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
        || _strText.contains("window", Qt::CaseInsensitive)) {
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
    if (_strText.contains(QString::fromUtf8("邻域"))
        || _strText.contains("neighborhood")) {
        return AnalysisTaskType::NeighborhoodAnalysis;
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
