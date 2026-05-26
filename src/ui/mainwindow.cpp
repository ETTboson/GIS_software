#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "core/ai/aimanager.h"
#include "service/analysis/attributequeryservice.h"
#include "service/analysis/spatialanalysisservice.h"
#include "service/analysis/statisticalanalysisservice.h"
#include "service/data/datarepository.h"
#include "service/data/dataservice.h"
#include "service/data/spatialdatabaseservice.h"
#include "ui/components/imagebutton.h"
#include "ui/docks/aidockwidget.h"
#include "ui/docks/analysisworkspacedockwidget.h"
#include "ui/map/map3dviewwidget.h"
#include "ui/map/mapcanvasmanager.h"
#include "ui/map/mapcanvaswidget.h"
#include "ui/visualization/visualizationmanager.h"

#include <memory>

#include <QAbstractButton>
#include <QAction>
#include <QColorDialog>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QInputDialog>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileInfo>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QStringList>
#include <QTableView>
#include <QTextEdit>
#include <QVBoxLayout>

#include <qgslayertreemodel.h>
#include <qgslayertreemapcanvasbridge.h>
#include <qgsfeature.h>
#include <qgsfeatureiterator.h>
#include <qgsfeaturesink.h>
#include <qgsfields.h>
#include <qgsmaplayer.h>
#include <qgsproject.h>
#include <qgsrasterlayer.h>
#include <qgsvectorfilewriter.h>
#include <qgsvectorlayer.h>

namespace
{

QString dataAssetTypeDisplayName(DataAssetType _eAssetType)
{
    switch (_eAssetType) {
    case DataAssetType::Raster:
        return QObject::tr("栅格资产");
    case DataAssetType::Vector:
        return QObject::tr("矢量资产");
    case DataAssetType::Table:
        return QObject::tr("表格资产");
    case DataAssetType::MetadataDocument:
        return QObject::tr("元数据文档");
    case DataAssetType::None:
    default:
        return QObject::tr("未指定资产");
    }
}

QString toolDisplayName(const QString& _strToolId)
{
    if (_strToolId == "basic_statistics") {
        return QObject::tr("基础统计");
    }
    if (_strToolId == "frequency_statistics") {
        return QObject::tr("频率统计");
    }
    if (_strToolId == "neighborhood_analysis") {
        return QObject::tr("邻域分析");
    }
    if (_strToolId == "attribute_query") {
        return QObject::tr("属性查询");
    }
    if (_strToolId == "buffer_analysis") {
        return QObject::tr("缓冲分析");
    }
    if (_strToolId == "overlay_analysis") {
        return QObject::tr("叠加分析");
    }
    if (_strToolId == "spatial_query") {
        return QObject::tr("空间查询");
    }
    if (_strToolId == "proximity_query") {
        return QObject::tr("邻近查询");
    }
    if (_strToolId == "raster_calc") {
        return QObject::tr("栅格计算");
    }
    return _strToolId;
}

bool tryParseOverlayOperation(const QString& _strValue,
    OverlayOperationType& _eOperation)
{
    const QString _strNormalized = _strValue.trimmed().toLower();
    if (_strNormalized.isEmpty() || _strNormalized == "intersect"
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

bool isValidQueryOperator(const QString& _strOperatorId)
{
    const QString _strNormalized = _strOperatorId.trimmed().toLower();
    return _strNormalized == QStringLiteral("=")
        || _strNormalized == QStringLiteral("==")
        || _strNormalized == QStringLiteral("eq")
        || _strNormalized == QStringLiteral("!=")
        || _strNormalized == QStringLiteral("<>")
        || _strNormalized == QStringLiteral("ne")
        || _strNormalized == QStringLiteral(">")
        || _strNormalized == QStringLiteral("gt")
        || _strNormalized == QStringLiteral(">=")
        || _strNormalized == QStringLiteral("gte")
        || _strNormalized == QStringLiteral("<")
        || _strNormalized == QStringLiteral("lt")
        || _strNormalized == QStringLiteral("<=")
        || _strNormalized == QStringLiteral("lte")
        || _strNormalized == QStringLiteral("contains");
}

bool isValidSpatialRelation(const QString& _strRelationId)
{
    const QString _strNormalized = _strRelationId.trimmed().toLower();
    return _strNormalized == QStringLiteral("intersects")
        || _strNormalized == QStringLiteral("intersect")
        || _strNormalized == QStringLiteral("within")
        || _strNormalized == QStringLiteral("contains");
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

QJsonArray buildDemoAliasArray(const AnalysisDataAsset& _assetInput)
{
    const QString _strHaystack = QStringLiteral("%1 %2 %3")
        .arg(_assetInput.strName,
            _assetInput.strSourcePath,
            _assetInput.dataVector.vFieldNames.join(QStringLiteral(" ")))
        .toLower();
    QJsonArray _jsonAliases;
    if (_strHaystack.contains(QStringLiteral("building"))) {
        _jsonAliases.append(QString::fromUtf8("建筑"));
        _jsonAliases.append(QStringLiteral("buildings"));
    }
    if (_strHaystack.contains(QStringLiteral("road"))) {
        _jsonAliases.append(QString::fromUtf8("道路"));
        _jsonAliases.append(QStringLiteral("roads"));
    }
    if (_strHaystack.contains(QStringLiteral("poi"))) {
        _jsonAliases.append(QStringLiteral("POI"));
        _jsonAliases.append(QStringLiteral("pois"));
        _jsonAliases.append(QString::fromUtf8("学校"));
        _jsonAliases.append(QStringLiteral("schools"));
        _jsonAliases.append(QString::fromUtf8("医院"));
        _jsonAliases.append(QStringLiteral("hospitals"));
    }
    if (_strHaystack.contains(QStringLiteral("place"))
        || _strHaystack.contains(QStringLiteral("population"))) {
        _jsonAliases.append(QString::fromUtf8("城市"));
        _jsonAliases.append(QStringLiteral("places"));
        _jsonAliases.append(QStringLiteral("population"));
    }
    if (_strHaystack.contains(QStringLiteral("water"))) {
        _jsonAliases.append(QString::fromUtf8("水系"));
        _jsonAliases.append(QStringLiteral("waterways"));
    }
    return _jsonAliases;
}

bool assetHasField(const AnalysisDataAsset& _assetInput,
    const QString& _strFieldName)
{
    for (const QString& _strCurrentField : _assetInput.dataVector.vFieldNames) {
        if (_strCurrentField.compare(_strFieldName, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

QString proximitySubsetForAssetRef(const AnalysisDataAsset& _assetInput,
    const QString& _strAssetRef)
{
    if (!assetHasField(_assetInput, QStringLiteral("fclass"))) {
        return QString();
    }

    const QString _strRef = _strAssetRef.trimmed().toLower();
    if (_strRef.contains(QString::fromUtf8("学校"))
        || _strRef.contains(QStringLiteral("school"))) {
        return QStringLiteral(
            "\"fclass\" IN ('school','college','university','kindergarten')");
    }
    if (_strRef.contains(QString::fromUtf8("医院"))
        || _strRef.contains(QStringLiteral("hospital"))
        || _strRef.contains(QStringLiteral("clinic"))) {
        return QStringLiteral(
            "\"fclass\" IN ('hospital','clinic','doctors')");
    }
    return QString();
}

QJsonArray capabilityArrayToJson(AnalysisCapabilities _flagsCapabilities)
{
    QJsonArray _jsonCaps;
    if (_flagsCapabilities.testFlag(AnalysisCapability::Statistical)) {
        _jsonCaps.append("statistical");
    }
    if (_flagsCapabilities.testFlag(AnalysisCapability::SpatialRaster)) {
        _jsonCaps.append("spatial_raster");
    }
    if (_flagsCapabilities.testFlag(AnalysisCapability::SpatialVector)) {
        _jsonCaps.append("spatial_vector");
    }
    if (_flagsCapabilities.testFlag(AnalysisCapability::AttributeQuery)) {
        _jsonCaps.append("attribute_query");
    }
    return _jsonCaps;
}

QString capabilityText(AnalysisCapabilities _flagsCapabilities)
{
    QStringList _vCaps;
    if (_flagsCapabilities.testFlag(AnalysisCapability::Statistical)) {
        _vCaps << QObject::tr("统计分析");
    }
    if (_flagsCapabilities.testFlag(AnalysisCapability::SpatialRaster)) {
        _vCaps << QObject::tr("栅格空间分析");
    }
    if (_flagsCapabilities.testFlag(AnalysisCapability::SpatialVector)) {
        _vCaps << QObject::tr("矢量空间分析");
    }
    if (_flagsCapabilities.testFlag(AnalysisCapability::AttributeQuery)) {
        _vCaps << QObject::tr("属性查询");
    }
    return _vCaps.isEmpty() ? QObject::tr("无") : _vCaps.join(", ");
}

QString normalizedLayerSourceKey(const QString& _strSource)
{
    QString _strSourceText = _strSource.trimmed();
    if (_strSourceText.isEmpty()) {
        return QString();
    }

    QString _strDatabasePath;
    QString _strTableName;
    if (SpatialDatabaseService::parseLayerSourceUri(
            _strSourceText, _strDatabasePath, _strTableName)) {
        return QStringLiteral("%1|layername=%2")
            .arg(QFileInfo(_strDatabasePath).absoluteFilePath().toLower(),
                _strTableName.trimmed().toLower());
    }

    const int _nProviderOptionPos = _strSourceText.indexOf('|');
    if (_nProviderOptionPos >= 0) {
        _strSourceText = _strSourceText.left(_nProviderOptionPos);
    }
    return QFileInfo(_strSourceText).absoluteFilePath().toLower();
}

bool stringListContainsExact(const QStringList& _vstrValues,
    const QString& _strNeedle)
{
    for (const QString& _strValue : _vstrValues) {
        if (_strValue == _strNeedle) {
            return true;
        }
    }
    return false;
}

void appendUniqueLayerSourceKey(QStringList& _vstrKeys,
    const QString& _strSource)
{
    const QString _strKey = normalizedLayerSourceKey(_strSource);
    if (!_strKey.isEmpty() && !stringListContainsExact(_vstrKeys, _strKey)) {
        _vstrKeys.append(_strKey);
    }
}

QStringList sourceKeysForAsset(const AnalysisDataAsset& _assetInput)
{
    QStringList _vstrKeys;
    appendUniqueLayerSourceKey(_vstrKeys, _assetInput.strSourcePath);
    appendUniqueLayerSourceKey(_vstrKeys, _assetInput.dataVector.strSourceUri);
    if (!_assetInput.dataVector.strDatabasePath.trimmed().isEmpty()
        && !_assetInput.dataVector.strTableName.trimmed().isEmpty()) {
        appendUniqueLayerSourceKey(_vstrKeys,
            SpatialDatabaseService::buildLayerSourceUri(
                _assetInput.dataVector.strDatabasePath,
                _assetInput.dataVector.strTableName));
    }
    return _vstrKeys;
}

QStringList sourceKeysForLayer(QgsMapLayer* _pLayerInput)
{
    QStringList _vstrKeys;
    if (_pLayerInput == nullptr) {
        return _vstrKeys;
    }

    appendUniqueLayerSourceKey(_vstrKeys, _pLayerInput->source());
    return _vstrKeys;
}

bool sourceKeyListsIntersect(const QStringList& _vstrLeft,
    const QStringList& _vstrRight)
{
    for (const QString& _strLeftKey : _vstrLeft) {
        if (stringListContainsExact(_vstrRight, _strLeftKey)) {
            return true;
        }
    }
    return false;
}

QJsonObject buildAssetContextObject(const AnalysisDataAsset& _assetInput)
{
    QJsonObject _jsonAsset;
    QJsonArray _jsonFieldNames;
    for (const QString& _strFieldName : _assetInput.dataVector.vFieldNames) {
        _jsonFieldNames.append(_strFieldName);
    }
    QJsonArray _jsonNumericFieldNames;
    for (const QString& _strFieldName : _assetInput.dataVector.vNumericFieldNames) {
        _jsonNumericFieldNames.append(_strFieldName);
    }

    _jsonAsset["id"] = _assetInput.strAssetId;
    _jsonAsset["name"] = _assetInput.strName;
    _jsonAsset["source_path"] = _assetInput.strSourcePath;
    _jsonAsset["source_format"] = _assetInput.strSourceFormat;
    _jsonAsset["asset_type"] = dataAssetTypeDisplayName(_assetInput.eAssetType);
    _jsonAsset["capabilities"] = capabilityArrayToJson(_assetInput.flagsCapabilities);
    _jsonAsset["summary"] = _assetInput.strSummary;
    _jsonAsset["has_numeric_data"] = _assetInput.bHasNumericDataset;
    _jsonAsset["numeric_rows"] = _assetInput.dataNumeric.nRows;
    _jsonAsset["numeric_cols"] = _assetInput.dataNumeric.nCols;
    _jsonAsset["vector_geometry_type"] = _assetInput.dataVector.strGeometryType;
    _jsonAsset["vector_feature_count"] = _assetInput.dataVector.nFeatureCount;
    _jsonAsset["vector_fields"] = _jsonFieldNames;
    _jsonAsset["vector_numeric_fields"] = _jsonNumericFieldNames;
    _jsonAsset["vector_source_uri"] = _assetInput.dataVector.strSourceUri;
    _jsonAsset["vector_database_path"] = _assetInput.dataVector.strDatabasePath;
    _jsonAsset["vector_table_name"] = _assetInput.dataVector.strTableName;
    _jsonAsset["vector_geometry_column"] = _assetInput.dataVector.strGeometryColumn;
    _jsonAsset["demo_aliases"] = buildDemoAliasArray(_assetInput);
    return _jsonAsset;
}

} // namespace

MainWindow::MainWindow(QWidget* _pParent)
    : QMainWindow(_pParent)
    , mpUI(new Ui::MainWindow)
    , mpDataService(nullptr)
    , mpDataRepository(nullptr)
    , mpStatisticalAnalysisService(nullptr)
    , mpSpatialAnalysisService(nullptr)
    , mpAttributeQueryService(nullptr)
    , mpAIManager(nullptr)
    , mpMapCanvasManager(nullptr)
    , mpctrlDockLayer(nullptr)
    , mpctrlDockAI(nullptr)
    , mpctrlDockAttribute(nullptr)
    , mpctrlDockAnalysisWorkspace(nullptr)
    , mpctrlDockLog(nullptr)
    , mpctrlLayerTreeView(nullptr)
    , mpctrlAttrTable(nullptr)
    , mpctrlLogView(nullptr)
    , mpctrlLabelCoord(nullptr)
    , mpctrlLabelScale(nullptr)
    , mpctrlLabelStatus(nullptr)
    , mpVisualizationManager(nullptr)
    , mbHasPendingRun(false)
{
    mpUI->setupUi(this);

    initModules();
    initRibbonButtons();
    initDockWidgets();
    initStatusBar();
    initConnections();
}

MainWindow::~MainWindow()
{
    delete mpDataService;
    mpDataService = nullptr;

    delete mpDataRepository;
    mpDataRepository = nullptr;

    delete mpStatisticalAnalysisService;
    mpStatisticalAnalysisService = nullptr;

    delete mpSpatialAnalysisService;
    mpSpatialAnalysisService = nullptr;

    delete mpAttributeQueryService;
    mpAttributeQueryService = nullptr;

    delete mpAIManager;
    mpAIManager = nullptr;

    delete mpMapCanvasManager;
    mpMapCanvasManager = nullptr;

    delete mpVisualizationManager;
    mpVisualizationManager = nullptr;

    delete mpUI;
    mpUI = nullptr;
}

void MainWindow::initModules()
{
    mpDataService = new DataService(this);
    mpDataRepository = new DataRepository(this);
    mpStatisticalAnalysisService = new StatisticalAnalysisService(this);
    mpSpatialAnalysisService = new SpatialAnalysisService(this);
    mpAttributeQueryService = new AttributeQueryService(this);
    mpAIManager = new AIManager(this);
    mpMapCanvasManager = new MapCanvasManager(this);
    mpVisualizationManager = new VisualizationManager(this);

    mpAIManager->setToolHost(this);

    MapCanvasWidget* _pCanvas = mpMapCanvasManager->createCanvas(mpUI->wgtMapCanvas);

    QVBoxLayout* _pLayout = new QVBoxLayout(mpUI->wgtMapCanvas);
    _pLayout->setContentsMargins(0, 0, 0, 0);
    _pLayout->setSpacing(0);
    _pLayout->addWidget(_pCanvas);
    mpUI->wgtMapCanvas->setLayout(_pLayout);
}

QJsonObject MainWindow::getAnalysisContext() const
{
    QJsonObject _jsonContext;
    _jsonContext["has_current_asset"] = (mpDataRepository != nullptr)
        ? mpDataRepository->hasCurrentAsset()
        : false;
    _jsonContext["has_numeric_data"] = (mpDataRepository != nullptr
        && mpDataRepository->hasCurrentAsset())
        ? mpDataRepository->currentAsset().bHasNumericDataset
        : false;

    if (mpDataRepository != nullptr && mpDataRepository->hasCurrentAsset()) {
        const AnalysisDataAsset _assetCurrent = mpDataRepository->currentAsset();
        _jsonContext["selected_asset_id"] = _assetCurrent.strAssetId;
        _jsonContext["current_asset"] = buildAssetContextObject(_assetCurrent);
        _jsonContext["ready_data_path"] = _assetCurrent.strSourcePath;
    }

    QJsonArray _jsonAssets;
    if (mpDataRepository != nullptr) {
        const QList<AnalysisDataAsset> _vAssets = mpDataRepository->getAssets();
        for (const AnalysisDataAsset& _assetCurrent : _vAssets) {
            _jsonAssets.append(buildAssetContextObject(_assetCurrent));
        }
    }
    _jsonContext["assets"] = _jsonAssets;

    QJsonArray _jsonLayers;
    if (mpDataService != nullptr) {
        const QList<LayerInfo> _vLayers = mpDataService->getLayers();
        for (const LayerInfo& _layerInfo : _vLayers) {
            QJsonObject _jsonLayer;
            _jsonLayer["name"] = _layerInfo.strName;
            _jsonLayer["type"] = _layerInfo.strType;
            _jsonLayer["file_path"] = _layerInfo.strFilePath;
            _jsonLayer["visible"] = _layerInfo.bVisible;
            _jsonLayers.append(_jsonLayer);
        }
    }
    _jsonContext["layers"] = _jsonLayers;
    return _jsonContext;
}

bool MainWindow::executeAnalysisTool(const QString& _strToolName,
    const QJsonObject& _jsonArgs,
    QString& _strResult,
    QString& _strError)
{
    _strResult.clear();
    _strError.clear();

    if (mpDataRepository == nullptr || !mpDataRepository->hasAssets()) {
        _strError = tr("当前没有已导入的分析资产，请先导入数据");
        return false;
    }

    const QString _strSourceAssetRef =
        _jsonArgs["source_asset_id"].toString().trimmed();
    AnalysisDataAsset _assetSource;
    if (!_strSourceAssetRef.isEmpty()) {
        _assetSource = findAssetByIdOrAlias(_strSourceAssetRef);
        if (_assetSource.strAssetId.trimmed().isEmpty()) {
            _strError = tr("参数错误：未找到 source_asset_id 对应的分析资产：%1")
                .arg(_strSourceAssetRef);
            return false;
        }
    } else if (mpDataRepository->hasCurrentAsset()) {
        _assetSource = mpDataRepository->currentAsset();
    } else {
        _strError = tr("当前没有已选择的分析资产，请提供 source_asset_id 或先选择数据");
        return false;
    }

    AnalysisRunConfig _configRun;
    _configRun.strSourceAssetId = _assetSource.strAssetId;
    if (_strToolName == "run_basic_statistics") {
        _configRun.strToolId = "basic_statistics";
    } else if (_strToolName == "run_frequency_statistics") {
        const int _nBinCount = _jsonArgs["bin_count"].toInt(0);
        if (_nBinCount < 2) {
            _strError = tr("参数错误：bin_count 必须大于等于 2");
            return false;
        }
        _configRun.strToolId = "frequency_statistics";
        _configRun.nFrequencyBins = _nBinCount;
    } else if (_strToolName == "run_neighborhood_analysis") {
        const int _nWindowSize = _jsonArgs["window_size"].toInt(0);
        if (_nWindowSize < 3 || (_nWindowSize % 2) == 0) {
            _strError = tr("参数错误：window_size 必须是不小于 3 的奇数");
            return false;
        }
        _configRun.strToolId = "neighborhood_analysis";
        _configRun.nNeighborhoodWindow = _nWindowSize;
    } else if (_strToolName == "run_buffer_analysis") {
        const double _dDistance = _jsonArgs["distance"].toDouble(0.0);
        const int _nSegments = _jsonArgs.contains("segments")
            ? _jsonArgs["segments"].toInt(8)
            : 8;
        if (_dDistance <= 0.0) {
            _strError = tr("参数错误：distance 必须大于 0");
            return false;
        }
        if (_nSegments <= 0) {
            _strError = tr("参数错误：segments 必须大于 0");
            return false;
        }
        _configRun.strToolId = "buffer_analysis";
        _configRun.dBufferDistance = _dDistance;
        _configRun.nBufferSegments = _nSegments;
    } else if (_strToolName == "run_overlay_analysis") {
        const QString _strOverlayAssetId =
            _jsonArgs["overlay_asset_id"].toString().trimmed();
        OverlayOperationType _eOverlayOperation = OverlayOperationType::Intersect;
        if (_jsonArgs.contains("operation")
            && !tryParseOverlayOperation(
                _jsonArgs["operation"].toString(), _eOverlayOperation)) {
            _strError = tr("参数错误：operation 必须是 intersect 或 union");
            return false;
        }
        if (_strOverlayAssetId.isEmpty()) {
            _strError = tr("参数错误：overlay_asset_id 不能为空");
            return false;
        }

        const AnalysisDataAsset _assetOverlay =
            findAssetByIdOrAlias(_strOverlayAssetId);
        if (_assetOverlay.strAssetId.trimmed().isEmpty()) {
            _strError = tr("参数错误：未找到 overlay_asset_id 对应的分析资产");
            return false;
        }
        if (!_assetOverlay.flagsCapabilities.testFlag(
            AnalysisCapability::SpatialVector)) {
            _strError = tr("参数错误：叠加资产“%1”不是可用矢量资产")
                .arg(_assetOverlay.strName);
            return false;
        }

        _configRun.strToolId = "overlay_analysis";
        _configRun.strOverlayAssetId = _assetOverlay.strAssetId;
        _configRun.eOverlayOperation = _eOverlayOperation;
    } else if (_strToolName == "run_attribute_query") {
        const QString _strFieldName =
            _jsonArgs["field_name"].toString().trimmed();
        const QString _strOperatorId =
            _jsonArgs["operator"].toString().trimmed();
        const QString _strValueText = _jsonArgs["value"].toString();
        if (_strFieldName.isEmpty()) {
            _strError = tr("参数错误：field_name 不能为空");
            return false;
        }
        if (!isValidQueryOperator(_strOperatorId)) {
            _strError = tr("参数错误：operator 必须是 =、!=、>、>=、<、<= 或 contains");
            return false;
        }

        _configRun.strToolId = "attribute_query";
        _configRun.strQueryFieldName = _strFieldName;
        _configRun.strQueryOperatorId = _strOperatorId;
        _configRun.strQueryValueText = _strValueText;
    } else if (_strToolName == "run_spatial_query") {
        const QString _strTargetAssetId =
            _jsonArgs["target_asset_id"].toString().trimmed();
        const QString _strRelationId = _jsonArgs.contains("relation")
            ? _jsonArgs["relation"].toString().trimmed()
            : QStringLiteral("intersects");
        if (_strTargetAssetId.isEmpty()) {
            _strError = tr("参数错误：target_asset_id 不能为空");
            return false;
        }
        if (!isValidSpatialRelation(_strRelationId)) {
            _strError = tr("参数错误：relation 必须是 intersects、within 或 contains");
            return false;
        }

        const AnalysisDataAsset _assetTarget =
            findAssetByIdOrAlias(_strTargetAssetId);
        if (_assetTarget.strAssetId.trimmed().isEmpty()) {
            _strError = tr("参数错误：未找到 target_asset_id 对应的分析资产");
            return false;
        }
        if (!_assetTarget.flagsCapabilities.testFlag(
            AnalysisCapability::SpatialVector)) {
            _strError = tr("参数错误：目标区域资产“%1”不是可用矢量资产")
                .arg(_assetTarget.strName);
            return false;
        }

        _configRun.strToolId = "spatial_query";
        _configRun.strSpatialTargetAssetId = _assetTarget.strAssetId;
        _configRun.strSpatialRelationId = _strRelationId;
    } else if (_strToolName == "run_proximity_query") {
        const QString _strReferenceAssetId =
            _jsonArgs["reference_asset_id"].toString().trimmed();
        const double _dDistance = _jsonArgs["distance"].toDouble(0.0);
        const int _nSegments = _jsonArgs.contains("segments")
            ? _jsonArgs["segments"].toInt(8)
            : 8;
        const bool _bInvertMatch = _jsonArgs.contains("invert_match")
            && _jsonArgs["invert_match"].toBool(false);
        const QString _strSourceSubsetExpression =
            _jsonArgs["source_subset_expression"].toString().trimmed();
        const QString _strReferenceSubsetExpression =
            _jsonArgs["reference_subset_expression"].toString().trimmed();
        if (_strReferenceAssetId.isEmpty()) {
            _strError = tr("参数错误：reference_asset_id 不能为空");
            return false;
        }
        if (_dDistance <= 0.0) {
            _strError = tr("参数错误：distance 必须大于 0");
            return false;
        }
        if (_nSegments <= 0) {
            _strError = tr("参数错误：segments 必须大于 0");
            return false;
        }

        const AnalysisDataAsset _assetReference =
            findAssetByIdOrAlias(_strReferenceAssetId);
        if (_assetReference.strAssetId.trimmed().isEmpty()) {
            _strError = tr("参数错误：未找到 reference_asset_id 对应的分析资产");
            return false;
        }
        if (!_assetReference.flagsCapabilities.testFlag(
            AnalysisCapability::SpatialVector)) {
            _strError = tr("参数错误：参考资产“%1”不是可用矢量资产")
                .arg(_assetReference.strName);
            return false;
        }

        _configRun.strToolId = "proximity_query";
        _configRun.strProximityReferenceAssetId = _assetReference.strAssetId;
        _configRun.bProximityInvertMatch = _bInvertMatch;
        _configRun.strSourceSubsetExpression =
            _strSourceSubsetExpression.isEmpty()
            ? proximitySubsetForAssetRef(_assetSource, _strSourceAssetRef)
            : _strSourceSubsetExpression;
        _configRun.strProximityReferenceSubsetExpression =
            _strReferenceSubsetExpression.isEmpty()
            ? proximitySubsetForAssetRef(_assetReference, _strReferenceAssetId)
            : _strReferenceSubsetExpression;
        _configRun.dProximityDistance = _dDistance;
        _configRun.nBufferSegments = _nSegments;
    } else {
        _strError = tr("未知分析工具：%1").arg(_strToolName);
        return false;
    }

    if (!assetSupportsTool(_assetSource, _configRun.strToolId)) {
        _strError = tr("源资产“%1”不支持工具“%2”")
            .arg(_assetSource.strName, toolDisplayName(_configRun.strToolId));
        return false;
    }

    AnalysisResult _resultCaptured;
    bool _bCaptured = false;
    auto _captureResult = [&](const AnalysisResult& _resultCurrent) {
        if (_bCaptured) {
            return;
        }
        _resultCaptured = _resultCurrent;
        _bCaptured = true;
    };

    const QMetaObject::Connection _okConnStat = connect(
        mpStatisticalAnalysisService,
        &StatisticalAnalysisService::analysisFinished,
        this,
        _captureResult);
    const QMetaObject::Connection _failConnStat = connect(
        mpStatisticalAnalysisService,
        &StatisticalAnalysisService::analysisFailed,
        this,
        _captureResult);
    const QMetaObject::Connection _okConnSpatial = connect(
        mpSpatialAnalysisService,
        &SpatialAnalysisService::analysisFinished,
        this,
        _captureResult);
    const QMetaObject::Connection _failConnSpatial = connect(
        mpSpatialAnalysisService,
        &SpatialAnalysisService::analysisFailed,
        this,
        _captureResult);
    const QMetaObject::Connection _okConnAttribute = connect(
        mpAttributeQueryService,
        &AttributeQueryService::analysisFinished,
        this,
        _captureResult);
    const QMetaObject::Connection _failConnAttribute = connect(
        mpAttributeQueryService,
        &AttributeQueryService::analysisFailed,
        this,
        _captureResult);

    runToolForAsset(_assetSource, _configRun);

    disconnect(_okConnStat);
    disconnect(_failConnStat);
    disconnect(_okConnSpatial);
    disconnect(_failConnSpatial);
    disconnect(_okConnAttribute);
    disconnect(_failConnAttribute);

    if (!_bCaptured) {
        _strError = tr("分析执行后未返回结果");
        return false;
    }

    if (_resultCaptured.bSuccess) {
        _strResult = _resultCaptured.strDesc;
        return true;
    }

    _strError = _resultCaptured.strDesc;
    return false;
}

void MainWindow::initRibbonButtons()
{
    mpUI->btnOpenData->setIconName("open_data");
    mpUI->btnAddLayer->setIconName("add_layer");
    mpUI->btnSaveProject->setIconName("save_project");
    mpUI->btnExportResult->setIconName("export_result");
    mpUI->btnBufferAnalysis->setIconName("buffer");
    mpUI->btnOverlayAnalysis->setIconName("overlay");
    mpUI->btnSpatialQuery->setIconName("query");
    mpUI->btnRasterCalc->setIconName("raster_calc");
    mpUI->btnAIAnalyze->setIconName("ai_analyze");
    mpUI->btnAIChat->setIconName("ai_chat");

    mpUI->actionToggleAnalysisPanel->setText(tr("分析工作区"));
    mpUI->actionToggleAnalysisPanel->setCheckable(true);
    mpUI->actionBufferAnalysis->setText(tr("缓冲分析"));
    mpUI->actionOverlayAnalysis->setText(tr("叠加分析"));
    mpUI->actionSpatialQuery->setText(tr("空间查询"));
    mpUI->actionRasterCalc->setText(tr("栅格计算"));

    mpUI->btnBufferAnalysis->setToolTip(tr("打开统一分析工作区并聚焦缓冲分析入口。"));
    mpUI->btnOverlayAnalysis->setToolTip(tr("打开统一分析工作区并聚焦叠加分析入口。"));
    mpUI->btnSpatialQuery->setToolTip(tr("打开统一分析工作区并聚焦空间查询入口。"));
    mpUI->btnRasterCalc->setToolTip(tr("打开统一分析工作区并聚焦栅格计算入口。"));
}

void MainWindow::initDockWidgets()
{
    createLayerDock();
    createAIDock();
    createAttributeDock();
    createAnalysisWorkspaceDock();
    createLogDock();
}

void MainWindow::createLayerDock()
{
    mpctrlDockLayer = new QDockWidget(tr("图层"), this);
    mpctrlDockLayer->setObjectName("dockLayer");
    mpctrlDockLayer->setAllowedAreas(
        Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget* _pctrlContainer = new QWidget(mpctrlDockLayer);
    QVBoxLayout* _pLayerLayout = new QVBoxLayout(_pctrlContainer);
    _pLayerLayout->setContentsMargins(4, 4, 4, 4);
    _pLayerLayout->setSpacing(4);

    mpctrlLayerTreeView = new QgsLayerTreeView(_pctrlContainer);

    QgsLayerTreeModel* _pTreeModel = new QgsLayerTreeModel(
        QgsProject::instance()->layerTreeRoot(), mpctrlLayerTreeView);
    _pTreeModel->setFlag(QgsLayerTreeModel::AllowNodeReorder);
    _pTreeModel->setFlag(QgsLayerTreeModel::AllowNodeRename);
    _pTreeModel->setFlag(QgsLayerTreeModel::AllowNodeChangeVisibility);
    _pTreeModel->setFlag(QgsLayerTreeModel::ShowLegendAsTree);
    mpctrlLayerTreeView->setModel(_pTreeModel);

    MapCanvasWidget* _pActiveCanvas = mpMapCanvasManager->activeCanvas();
    if (_pActiveCanvas != nullptr) {
        QgsLayerTreeMapCanvasBridge* _pBridge = new QgsLayerTreeMapCanvasBridge(
            QgsProject::instance()->layerTreeRoot(),
            _pActiveCanvas->mapCanvas(),
            this);
        Q_UNUSED(_pBridge)
    }

    _pLayerLayout->addWidget(mpctrlLayerTreeView);
    mpctrlDockLayer->setWidget(_pctrlContainer);
    addDockWidget(Qt::LeftDockWidgetArea, mpctrlDockLayer);
    mpctrlDockLayer->setVisible(true);

    connect(mpUI->actionToggleLayerPanel, &QAction::toggled,
        mpctrlDockLayer, &QDockWidget::setVisible);
    connect(mpctrlDockLayer, &QDockWidget::visibilityChanged,
        mpUI->actionToggleLayerPanel, &QAction::setChecked);
}

void MainWindow::createAIDock()
{
    mpctrlDockAI = new AIDockWidget(this);
    mpctrlDockAI->setAIManager(mpAIManager);
    addDockWidget(Qt::RightDockWidgetArea, mpctrlDockAI);
    mpctrlDockAI->setVisible(false);
    {
        QSignalBlocker _blocker(mpUI->actionToggleAIPanel);
        mpUI->actionToggleAIPanel->setChecked(false);
    }

    connect(mpUI->actionToggleAIPanel, &QAction::toggled,
        mpctrlDockAI, &QDockWidget::setVisible);
    connect(mpctrlDockAI, &QDockWidget::visibilityChanged,
        mpUI->actionToggleAIPanel, &QAction::setChecked);
}

void MainWindow::createAttributeDock()
{
    mpctrlDockAttribute = new QDockWidget(tr("属性表"), this);
    mpctrlDockAttribute->setObjectName("dockAttribute");
    mpctrlDockAttribute->setAllowedAreas(Qt::AllDockWidgetAreas);

    mpctrlAttrTable = new QTableView(mpctrlDockAttribute);
    mpctrlDockAttribute->setWidget(mpctrlAttrTable);

    addDockWidget(Qt::RightDockWidgetArea, mpctrlDockAttribute);
    mpctrlDockAttribute->setVisible(false);

    connect(mpUI->actionToggleAttrPanel, &QAction::toggled,
        mpctrlDockAttribute, &QDockWidget::setVisible);
    connect(mpctrlDockAttribute, &QDockWidget::visibilityChanged,
        mpUI->actionToggleAttrPanel, &QAction::setChecked);
}

void MainWindow::createAnalysisWorkspaceDock()
{
    mpctrlDockAnalysisWorkspace = new AnalysisWorkspaceDockWidget(this);
    mpctrlDockAnalysisWorkspace->setDataRepository(mpDataRepository);
    mpVisualizationManager->attachWorkspace(mpctrlDockAnalysisWorkspace);

    addDockWidget(Qt::RightDockWidgetArea, mpctrlDockAnalysisWorkspace);
    mpctrlDockAnalysisWorkspace->setVisible(false);

    connect(mpUI->actionToggleAnalysisPanel, &QAction::toggled,
        mpctrlDockAnalysisWorkspace, &QDockWidget::setVisible);
    connect(mpctrlDockAnalysisWorkspace, &QDockWidget::visibilityChanged,
        mpUI->actionToggleAnalysisPanel, &QAction::setChecked);
}

void MainWindow::createLogDock()
{
    mpctrlDockLog = new QDockWidget(tr("日志"), this);
    mpctrlDockLog->setObjectName("dockLog");
    mpctrlDockLog->setAllowedAreas(
        Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    mpctrlLogView = new QTextEdit(mpctrlDockLog);
    mpctrlLogView->setReadOnly(true);
    mpctrlLogView->setMaximumHeight(180);

    mpctrlDockLog->setWidget(mpctrlLogView);
    addDockWidget(Qt::BottomDockWidgetArea, mpctrlDockLog);
    mpctrlDockLog->setVisible(false);

    connect(mpUI->actionToggleLogPanel, &QAction::toggled,
        mpctrlDockLog, &QDockWidget::setVisible);
    connect(mpctrlDockLog, &QDockWidget::visibilityChanged,
        mpUI->actionToggleLogPanel, &QAction::setChecked);
}

void MainWindow::initStatusBar()
{
    mpctrlLabelCoord = new QLabel(tr("  经度: --  纬度: --  "), this);
    mpctrlLabelCoord->setMinimumWidth(200);

    mpctrlLabelScale = new QLabel(tr("  比例尺: 1:--  "), this);
    mpctrlLabelScale->setMinimumWidth(120);

    mpctrlLabelStatus = new QLabel(tr("  就绪  "), this);

    statusBar()->addWidget(mpctrlLabelCoord);
    statusBar()->addWidget(mpctrlLabelScale);
    statusBar()->addPermanentWidget(mpctrlLabelStatus);
}

void MainWindow::initConnections()
{
    connect(mpUI->actionOpenData, &QAction::triggered,
        this, &MainWindow::onOpenData);
    connect(mpUI->actionAddLayer, &QAction::triggered,
        this, &MainWindow::onAddLayer);
    connect(mpUI->actionSaveProject, &QAction::triggered,
        this, &MainWindow::onSaveProject);
    connect(mpUI->actionExportResult, &QAction::triggered,
        this, &MainWindow::onExportResult);
    connect(mpUI->actionExit, &QAction::triggered,
        this, &QWidget::close);
    connect(mpUI->actionZoomIn, &QAction::triggered,
        this, &MainWindow::onNavZoomIn);
    connect(mpUI->actionZoomOut, &QAction::triggered,
        this, &MainWindow::onNavZoomOut);
    connect(mpUI->actionFitView, &QAction::triggered,
        this, &MainWindow::onNavFitAll);
    connect(mpUI->actionNew3DView, &QAction::triggered,
        this, &MainWindow::onNew3DView);
    connect(mpUI->actionAIAnalyze, &QAction::triggered,
        this, &MainWindow::onAIAnalyze);
    connect(mpUI->actionAIChat, &QAction::triggered,
        this, &MainWindow::onAIChat);
    connect(mpUI->actionAbout, &QAction::triggered,
        this, &MainWindow::onAbout);
    connect(mpUI->actionBufferAnalysis, &QAction::triggered,
        this, &MainWindow::onBufferAnalysis);
    connect(mpUI->actionOverlayAnalysis, &QAction::triggered,
        this, &MainWindow::onOverlayAnalysis);
    connect(mpUI->actionSpatialQuery, &QAction::triggered,
        this, &MainWindow::onSpatialQuery);
    connect(mpUI->actionRasterCalc, &QAction::triggered,
        this, &MainWindow::onRasterCalc);

    connect(mpUI->btnOpenData, &ImageButton::clicked,
        this, &MainWindow::onOpenData);
    connect(mpUI->btnAddLayer, &ImageButton::clicked,
        this, &MainWindow::onAddLayer);
    connect(mpUI->btnSaveProject, &ImageButton::clicked,
        this, &MainWindow::onSaveProject);
    connect(mpUI->btnExportResult, &ImageButton::clicked,
        this, &MainWindow::onExportResult);
    connect(mpUI->btnAIAnalyze, &ImageButton::clicked,
        this, &MainWindow::onAIAnalyze);
    connect(mpUI->btnAIChat, &ImageButton::clicked,
        this, &MainWindow::onAIChat);
    connect(mpUI->btnBufferAnalysis, &ImageButton::clicked,
        this, &MainWindow::onBufferAnalysis);
    connect(mpUI->btnOverlayAnalysis, &ImageButton::clicked,
        this, &MainWindow::onOverlayAnalysis);
    connect(mpUI->btnSpatialQuery, &ImageButton::clicked,
        this, &MainWindow::onSpatialQuery);
    connect(mpUI->btnRasterCalc, &ImageButton::clicked,
        this, &MainWindow::onRasterCalc);

    connect(mpDataService, &DataService::layerLoaded,
        this, &MainWindow::onLayerLoaded);
    connect(mpDataService, &DataService::analysisAssetReady,
        this, &MainWindow::onAnalysisAssetReady);
    connect(mpDataService, &DataService::dataLoadFailed,
        this, &MainWindow::onDataLoadFailed);

    connect(mpStatisticalAnalysisService, &StatisticalAnalysisService::analysisFinished,
        this, &MainWindow::onAnalysisFinished);
    connect(mpStatisticalAnalysisService, &StatisticalAnalysisService::analysisFailed,
        this, &MainWindow::onAnalysisFailed);
    connect(mpStatisticalAnalysisService, &StatisticalAnalysisService::analysisProgress,
        this, &MainWindow::onAnalysisProgress);

    connect(mpSpatialAnalysisService, &SpatialAnalysisService::analysisFinished,
        this, &MainWindow::onAnalysisFinished);
    connect(mpSpatialAnalysisService, &SpatialAnalysisService::analysisFailed,
        this, &MainWindow::onAnalysisFailed);
    connect(mpSpatialAnalysisService, &SpatialAnalysisService::analysisProgress,
        this, &MainWindow::onAnalysisProgress);

    connect(mpAttributeQueryService, &AttributeQueryService::analysisFinished,
        this, &MainWindow::onAnalysisFinished);
    connect(mpAttributeQueryService, &AttributeQueryService::analysisFailed,
        this, &MainWindow::onAnalysisFailed);
    connect(mpAttributeQueryService, &AttributeQueryService::analysisProgress,
        this, &MainWindow::onAnalysisProgress);

    connect(mpDataRepository, &DataRepository::currentAssetChanged,
        this, &MainWindow::onCurrentAnalysisAssetChanged);
    connect(mpDataRepository, &DataRepository::currentAssetCleared,
        this, &MainWindow::onCurrentAnalysisAssetCleared);

    connect(mpctrlDockAnalysisWorkspace,
        &AnalysisWorkspaceDockWidget::basicStatisticsRequested,
        this, &MainWindow::onBasicStatisticsRequested);
    connect(mpctrlDockAnalysisWorkspace,
        &AnalysisWorkspaceDockWidget::frequencyStatisticsRequested,
        this, &MainWindow::onFrequencyStatisticsRequested);
    connect(mpctrlDockAnalysisWorkspace,
        &AnalysisWorkspaceDockWidget::neighborhoodAnalysisRequested,
        this, &MainWindow::onNeighborhoodAnalysisRequested);
    connect(mpctrlDockAnalysisWorkspace,
        &AnalysisWorkspaceDockWidget::bufferAnalysisRequested,
        this, &MainWindow::onBufferAnalysisRequested);
    connect(mpctrlDockAnalysisWorkspace,
        &AnalysisWorkspaceDockWidget::overlayAnalysisRequested,
        this, &MainWindow::onOverlayAnalysisRequested);
    connect(mpctrlDockAnalysisWorkspace,
        &AnalysisWorkspaceDockWidget::spatialQueryRequested,
        this, &MainWindow::onSpatialQueryRequested);
    connect(mpctrlDockAnalysisWorkspace,
        &AnalysisWorkspaceDockWidget::attributeQueryRequested,
        this, &MainWindow::onAttributeQueryRequested);
    connect(mpctrlDockAnalysisWorkspace,
        &AnalysisWorkspaceDockWidget::analysisAssetDeleteRequested,
        this, &MainWindow::onAnalysisAssetDeleteRequested);

    connect(mpMapCanvasManager, &MapCanvasManager::activeCanvasChanged,
        this, &MainWindow::onActiveCanvasChanged);

    reconnectCanvasSignals(mpMapCanvasManager->activeCanvas());
    initLayerTreeContextMenu();
}

void MainWindow::initLayerTreeContextMenu()
{
    if (mpctrlLayerTreeView == nullptr) {
        return;
    }

    mpctrlLayerTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mpctrlLayerTreeView, &QWidget::customContextMenuRequested,
        this, &MainWindow::onLayerTreeContextMenuRequested);
}

void MainWindow::reconnectCanvasSignals(MapCanvasWidget* _pCanvas)
{
    for (int _nCanvasIdx = 0; _nCanvasIdx < mpMapCanvasManager->canvasCount(); ++_nCanvasIdx) {
        MapCanvasWidget* _pOldCanvas = mpMapCanvasManager->canvasAt(_nCanvasIdx);
        if (_pOldCanvas != nullptr) {
            disconnect(_pOldCanvas, &MapCanvasWidget::coordChanged,
                this, &MainWindow::onCoordChanged);
            disconnect(_pOldCanvas, &MapCanvasWidget::scaleChanged,
                this, &MainWindow::onScaleChanged);
        }
    }

    if (_pCanvas == nullptr) {
        return;
    }

    connect(_pCanvas, &MapCanvasWidget::coordChanged,
        this, &MainWindow::onCoordChanged);
    connect(_pCanvas, &MapCanvasWidget::scaleChanged,
        this, &MainWindow::onScaleChanged);
}

void MainWindow::ensureWorkspaceVisible()
{
    if (mpctrlDockAnalysisWorkspace == nullptr) {
        return;
    }

    mpctrlDockAnalysisWorkspace->setVisible(true);
    mpctrlDockAnalysisWorkspace->raise();
    {
        QSignalBlocker _blocker(mpUI->actionToggleAnalysisPanel);
        mpUI->actionToggleAnalysisPanel->setChecked(true);
    }
}

QgsMapLayer* MainWindow::currentSelectedLayer() const
{
    if (mpctrlLayerTreeView == nullptr) {
        return nullptr;
    }

    return mpctrlLayerTreeView->currentLayer();
}

void MainWindow::selectLayerById(const QString& _strLayerId)
{
    if (mpctrlLayerTreeView == nullptr || _strLayerId.isEmpty()) {
        return;
    }

    QgsMapLayer* _pLayerTarget = QgsProject::instance()->mapLayer(_strLayerId);
    if (_pLayerTarget != nullptr) {
        mpctrlLayerTreeView->setCurrentLayer(_pLayerTarget);
    }
}

QStringList MainWindow::matchingLayerIdsForAsset(
    const AnalysisDataAsset& _assetInput) const
{
    const QStringList _vstrAssetKeys = sourceKeysForAsset(_assetInput);
    QStringList _vstrLayerIds;
    if (_vstrAssetKeys.isEmpty()) {
        return _vstrLayerIds;
    }

    const QMap<QString, QgsMapLayer*> _mapLayers =
        QgsProject::instance()->mapLayers();
    for (QgsMapLayer* _pLayerCurrent : _mapLayers) {
        if (_pLayerCurrent == nullptr) {
            continue;
        }
        if (sourceKeyListsIntersect(
                _vstrAssetKeys, sourceKeysForLayer(_pLayerCurrent))) {
            _vstrLayerIds.append(_pLayerCurrent->id());
        }
    }
    return _vstrLayerIds;
}

AnalysisDataAsset MainWindow::findAssetForLayer(QgsMapLayer* _pLayerInput) const
{
    if (mpDataRepository == nullptr || _pLayerInput == nullptr) {
        return AnalysisDataAsset();
    }

    const QStringList _vstrLayerKeys = sourceKeysForLayer(_pLayerInput);
    if (_vstrLayerKeys.isEmpty()) {
        return AnalysisDataAsset();
    }

    const QList<AnalysisDataAsset> _vAssets = mpDataRepository->getAssets();
    for (const AnalysisDataAsset& _assetCurrent : _vAssets) {
        if (sourceKeyListsIntersect(
                sourceKeysForAsset(_assetCurrent), _vstrLayerKeys)) {
            return _assetCurrent;
        }
    }
    return AnalysisDataAsset();
}

bool MainWindow::removeMapLayerById(const QString& _strLayerId,
    QString* _pstrLayerName)
{
    QgsMapLayer* _pLayerCurrent = QgsProject::instance()->mapLayer(_strLayerId);
    if (_pLayerCurrent == nullptr) {
        return false;
    }

    const QString _strLayerName = _pLayerCurrent->name();
    const QString _strLayerSource = _pLayerCurrent->source();
    if (_pstrLayerName != nullptr) {
        *_pstrLayerName = _strLayerName;
    }

    MapCanvasWidget* _pCanvas = mpMapCanvasManager == nullptr
        ? nullptr
        : mpMapCanvasManager->activeCanvas();
    if (_pCanvas != nullptr) {
        _pCanvas->removeLayer(_strLayerId);
    }
    if (QgsProject::instance()->mapLayer(_strLayerId) != nullptr) {
        QgsProject::instance()->removeMapLayer(_strLayerId);
    }

    if (mpDataService != nullptr) {
        mpDataService->removeLayerRecord(_strLayerSource, _strLayerName);
    }
    return true;
}

bool MainWindow::removeAnalysisAssetAndLinkedLayers(
    const AnalysisDataAsset& _assetTarget,
    bool _bAskConfirm)
{
    if (mpDataRepository == nullptr
        || _assetTarget.strAssetId.trimmed().isEmpty()) {
        return false;
    }

    const QString _strAssetName = _assetTarget.strName.trimmed().isEmpty()
        ? _assetTarget.strAssetId
        : _assetTarget.strName;
    if (_bAskConfirm) {
        const QMessageBox::StandardButton _buttonReply = QMessageBox::question(
            this,
            tr("删除数据资产"),
            tr("是否移除%1").arg(_strAssetName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (_buttonReply != QMessageBox::Yes) {
            return false;
        }
    }

    const QStringList _vstrLayerIds = matchingLayerIdsForAsset(_assetTarget);
    const bool _bWasPendingRun =
        mbHasPendingRun && mstrPendingRunAssetId == _assetTarget.strAssetId;
    const bool _bWasDisplayedResult =
        mstrDisplayedResultAssetId == _assetTarget.strAssetId
        || mResultLastSuccessful.strSourceAssetId == _assetTarget.strAssetId;

    if (!mpDataRepository->removeAssetById(_assetTarget.strAssetId)) {
        QMessageBox::warning(this,
            tr("删除数据资产"),
            tr("删除数据资产失败：记录已不存在。"));
        return false;
    }

    int _nRemovedLayerCount = 0;
    for (const QString& _strLayerId : _vstrLayerIds) {
        if (removeMapLayerById(_strLayerId)) {
            ++_nRemovedLayerCount;
        }
    }

    if (_bWasPendingRun) {
        clearPendingRun();
    }
    mmapLastSuccessfulRuns.remove(_assetTarget.strAssetId);

    if (_bWasDisplayedResult) {
        const QString _strMessage =
            tr("已删除数据资产“%1”。").arg(_strAssetName);
        mpctrlDockAnalysisWorkspace->clearCurrentResult(_strMessage);
        mpVisualizationManager->clearView(_strMessage);
        mstrDisplayedResultAssetId.clear();
        mResultLastSuccessful = AnalysisResult();
    }

    refreshImportedAssetHint();
    mpctrlLabelStatus->setText(
        tr("  已删除数据资产: %1  ").arg(_strAssetName));
    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(
            tr("[数据资产] 已删除 %1；同步移除图层 %2 个")
                .arg(_strAssetName)
                .arg(_nRemovedLayerCount));
    }
    return true;
}

void MainWindow::refreshImportedAssetHint()
{
    if (mpctrlDockAnalysisWorkspace == nullptr
        || mpDataRepository == nullptr) {
        return;
    }

    if (!mpDataRepository->hasAssets()) {
        mpctrlDockAnalysisWorkspace->setDataHint(
            tr("当前没有已导入数据资产。"));
        return;
    }

    const AnalysisDataAsset _assetCurrent = mpDataRepository->hasCurrentAsset()
        ? mpDataRepository->currentAsset()
        : mpDataRepository->getAssets().first();
    mpctrlDockAnalysisWorkspace->setDataHint(
        tr("已导入数据资产：%1\n类型：%2\n能力：%3")
            .arg(_assetCurrent.strName,
                dataAssetTypeDisplayName(_assetCurrent.eAssetType),
                capabilityText(_assetCurrent.flagsCapabilities)));
}

DataAssetType MainWindow::resolveAssetChoice(const AnalysisDataAsset& _assetInput)
{
    if (!_assetInput.bNeedsUserChoice) {
        return _assetInput.eAssetType;
    }

    QMessageBox _msgBox(this);
    _msgBox.setWindowTitle(tr("选择导入方式"));
    _msgBox.setIcon(QMessageBox::Question);
    _msgBox.setText(_assetInput.strChoicePrompt.isEmpty()
        ? tr("当前输入存在多种资产解释方式，请选择目标资产类型。")
        : _assetInput.strChoicePrompt);

    QAbstractButton* _pTableButton = _msgBox.addButton(
        dataAssetTypeDisplayName(DataAssetType::Table),
        QMessageBox::AcceptRole);
    QAbstractButton* _pVectorButton = _msgBox.addButton(
        dataAssetTypeDisplayName(DataAssetType::Vector),
        QMessageBox::ActionRole);
    _msgBox.addButton(QMessageBox::Cancel);

    _msgBox.exec();
    if (_msgBox.clickedButton() == _pTableButton) {
        return DataAssetType::Table;
    }
    if (_msgBox.clickedButton() == _pVectorButton) {
        return DataAssetType::Vector;
    }
    return DataAssetType::None;
}

bool MainWindow::assetSupportsTool(const AnalysisDataAsset& _assetInput,
    const QString& _strToolId) const
{
    if (_strToolId == "basic_statistics"
        || _strToolId == "frequency_statistics") {
        return _assetInput.flagsCapabilities.testFlag(AnalysisCapability::Statistical);
    }
    if (_strToolId == "neighborhood_analysis"
        || _strToolId == "raster_calc") {
        return _assetInput.flagsCapabilities.testFlag(AnalysisCapability::SpatialRaster);
    }
    if (_strToolId == "buffer_analysis"
        || _strToolId == "overlay_analysis"
        || _strToolId == "spatial_query"
        || _strToolId == "proximity_query") {
        return _assetInput.flagsCapabilities.testFlag(AnalysisCapability::SpatialVector);
    }
    if (_strToolId == "attribute_query") {
        return _assetInput.flagsCapabilities.testFlag(AnalysisCapability::AttributeQuery);
    }
    return false;
}

void MainWindow::openToolShortcut(const QString& _strToolId)
{
    ensureWorkspaceVisible();

    if (mpDataRepository == nullptr || !mpDataRepository->hasCurrentAsset()) {
        mpctrlDockAnalysisWorkspace->showDataPage(
            tr("请先导入并选择支持“%1”的数据资产。").arg(toolDisplayName(_strToolId)));
        return;
    }

    const AnalysisDataAsset _assetCurrent = mpDataRepository->currentAsset();
    if (!assetSupportsTool(_assetCurrent, _strToolId)) {
        mpctrlDockAnalysisWorkspace->showDataPage(
            tr("当前所选资产“%1”不支持“%2”，请先选择兼容数据。")
                .arg(_assetCurrent.strName, toolDisplayName(_strToolId)));
        return;
    }

    mpctrlDockAnalysisWorkspace->focusTool(
        _strToolId,
        tr("当前快捷入口：%1\n资产类型：%2\n资产能力：%3")
            .arg(toolDisplayName(_strToolId),
                dataAssetTypeDisplayName(_assetCurrent.eAssetType),
                capabilityText(_assetCurrent.flagsCapabilities)));
}

void MainWindow::runToolForCurrentAsset(const AnalysisRunConfig& _configRun)
{
    if (mpDataRepository == nullptr || !mpDataRepository->hasCurrentAsset()) {
        ensureWorkspaceVisible();
        mpctrlDockAnalysisWorkspace->showDataPage(
            tr("请先导入并选择一个可执行“%1”的数据资产。")
                .arg(toolDisplayName(_configRun.strToolId)));
        return;
    }

    const AnalysisDataAsset _assetCurrent = mpDataRepository->currentAsset();
    if (!assetSupportsTool(_assetCurrent, _configRun.strToolId)) {
        ensureWorkspaceVisible();
        mpctrlDockAnalysisWorkspace->showDataPage(
            tr("当前所选资产“%1”不支持“%2”，请先选择兼容数据。")
                .arg(_assetCurrent.strName, toolDisplayName(_configRun.strToolId)));
        return;
    }

    runToolForAsset(_assetCurrent, _configRun);
}

void MainWindow::runToolForAsset(const AnalysisDataAsset& _assetInput,
    const AnalysisRunConfig& _configRun)
{
    cachePendingRun(_assetInput.strAssetId, _configRun);
    ensureWorkspaceVisible();
    mpctrlDockAnalysisWorkspace->showResultsPage();
    mpctrlLabelStatus->setText(tr("  分析中...  "));

    if (_configRun.strToolId == "basic_statistics") {
        mpStatisticalAnalysisService->runBasicStatistics(_assetInput);
        return;
    }
    if (_configRun.strToolId == "frequency_statistics") {
        mpStatisticalAnalysisService->runFrequencyStatistics(
            _assetInput, _configRun.nFrequencyBins);
        return;
    }
    if (_configRun.strToolId == "neighborhood_analysis") {
        mpSpatialAnalysisService->runNeighborhoodAnalysis(
            _assetInput, _configRun.nNeighborhoodWindow);
        return;
    }
    if (_configRun.strToolId == "buffer_analysis") {
        mpSpatialAnalysisService->runBufferAnalysis(
            _assetInput,
            _configRun.dBufferDistance,
            _configRun.nBufferSegments);
        return;
    }
    if (_configRun.strToolId == "overlay_analysis") {
        const AnalysisDataAsset _assetOverlay = (mpDataRepository == nullptr)
            ? AnalysisDataAsset()
            : mpDataRepository->findAssetById(_configRun.strOverlayAssetId);
        mpSpatialAnalysisService->runOverlayAnalysis(
            _assetInput,
            _assetOverlay,
            _configRun.eOverlayOperation);
        return;
    }
    if (_configRun.strToolId == "spatial_query") {
        const AnalysisDataAsset _assetTarget = (mpDataRepository == nullptr)
            ? AnalysisDataAsset()
            : mpDataRepository->findAssetById(_configRun.strSpatialTargetAssetId);
        mpAttributeQueryService->runSpatialRelationQuery(
            _assetInput,
            _assetTarget,
            _configRun.strSpatialRelationId);
        return;
    }
    if (_configRun.strToolId == "proximity_query") {
        const AnalysisDataAsset _assetReference = (mpDataRepository == nullptr)
            ? AnalysisDataAsset()
            : mpDataRepository->findAssetById(_configRun.strProximityReferenceAssetId);
        mpAttributeQueryService->runProximityQuery(
            _assetInput,
            _assetReference,
            _configRun.dProximityDistance,
            _configRun.nBufferSegments,
            _configRun.bProximityInvertMatch,
            _configRun.strSourceSubsetExpression,
            _configRun.strProximityReferenceSubsetExpression);
        return;
    }
    if (_configRun.strToolId == "attribute_query") {
        mpAttributeQueryService->runAttributeQuery(
            _assetInput,
            _configRun.strQueryFieldName,
            _configRun.strQueryOperatorId,
            _configRun.strQueryValueText);
        return;
    }

    clearPendingRun();
    mpctrlDockAnalysisWorkspace->focusTool(
        _configRun.strToolId,
        tr("工具“%1”当前仅作为能力占位展示。").arg(toolDisplayName(_configRun.strToolId)));
    mpctrlLabelStatus->setText(tr("  就绪  "));
}

AnalysisDataAsset MainWindow::findAssetByIdOrAlias(
    const QString& _strAssetRef) const
{
    if (mpDataRepository == nullptr || _strAssetRef.trimmed().isEmpty()) {
        return AnalysisDataAsset();
    }

    AnalysisDataAsset _assetMatched =
        mpDataRepository->findAssetById(_strAssetRef.trimmed());
    if (!_assetMatched.strAssetId.trimmed().isEmpty()) {
        return _assetMatched;
    }

    const QString _strRef = _strAssetRef.trimmed().toLower();
    const QStringList _vKeywords = aliasKeywordsForAssetRef(_strRef);
    const QList<AnalysisDataAsset> _vAssets = mpDataRepository->getAssets();
    for (const AnalysisDataAsset& _assetCurrent : _vAssets) {
        const QString _strHaystack = QStringLiteral("%1 %2 %3 %4")
            .arg(_assetCurrent.strName,
                _assetCurrent.strSourcePath,
                _assetCurrent.dataVector.strSourceUri,
                _assetCurrent.dataVector.vFieldNames.join(QStringLiteral(" ")))
            .toLower();
        QString _strAliasHaystack = _strHaystack;
        const QJsonArray _jsonAliases = buildDemoAliasArray(_assetCurrent);
        for (const QJsonValue& _jsonAliasVal : _jsonAliases) {
            _strAliasHaystack += QStringLiteral(" %1")
                .arg(_jsonAliasVal.toString().toLower());
        }
        for (const QString& _strKeyword : _vKeywords) {
            if (_strKeyword.trimmed().isEmpty()) {
                continue;
            }
            if (_strAliasHaystack.contains(_strKeyword.trimmed().toLower())) {
                return _assetCurrent;
            }
        }
    }

    return AnalysisDataAsset();
}

void MainWindow::cachePendingRun(const QString& _strAssetId,
    const AnalysisRunConfig& _configRun)
{
    mbHasPendingRun = true;
    mstrPendingRunAssetId = _strAssetId;
    mConfigPendingRun = _configRun;
}

void MainWindow::clearPendingRun()
{
    mbHasPendingRun = false;
    mstrPendingRunAssetId.clear();
    mConfigPendingRun = AnalysisRunConfig();
}

void MainWindow::onOpenData()
{
    const QStringList _vFilePaths = QFileDialog::getOpenFileNames(
        this,
        tr("打开分析数据"),
        QString(),
        tr("分析数据 (*.csv *.xml *.asc *.txt *.shp *.geojson *.sqlite *.db *.tif *.tiff *.img);;"
           "CSV 数据 (*.csv);;XML 数据 (*.xml);;栅格数据 (*.asc *.txt *.tif *.tiff *.img);;"
           "矢量数据 (*.shp *.geojson);;空间数据库 (*.sqlite *.db);;全部文件 (*)"));
    if (_vFilePaths.isEmpty()) {
        return;
    }

    mpctrlLabelStatus->setText(
        tr("  正在解析 %1 个分析数据...  ").arg(_vFilePaths.size()));
    for (const QString& _strFilePath : _vFilePaths) {
        mpDataService->openDataForAnalysis(_strFilePath);
    }
    mpctrlLabelStatus->setText(
        tr("  已完成 %1 个分析数据加载请求  ").arg(_vFilePaths.size()));
}

void MainWindow::onAddLayer()
{
    const QStringList _vFilePaths = QFileDialog::getOpenFileNames(
        this,
        tr("添加图层"),
        QString(),
        tr("空间图层 (*.shp *.geojson *.sqlite *.db *.tif *.tiff *.img);;"
           "矢量图层 (*.shp *.geojson);;空间数据库 (*.sqlite *.db);;"
           "栅格图层 (*.tif *.tiff *.img);;全部文件 (*)"));
    if (_vFilePaths.isEmpty()) {
        return;
    }

    mpctrlLabelStatus->setText(
        tr("  正在添加 %1 个地图图层...  ").arg(_vFilePaths.size()));
    for (const QString& _strFilePath : _vFilePaths) {
        mpDataService->loadLayerToMap(_strFilePath);
    }
    mpctrlLabelStatus->setText(
        tr("  已完成 %1 个地图图层加载请求  ").arg(_vFilePaths.size()));
}

void MainWindow::onSaveProject()
{
}

void MainWindow::onExportResult()
{
    if (!mResultLastSuccessful.bSuccess
        || !mResultLastSuccessful.bHasOutputLayer
        || mResultLastSuccessful.strOutputPath.trimmed().isEmpty()) {
        QMessageBox::information(this,
            tr("导出结果"),
            tr("当前没有可导出的成功查询/分析结果图层。"));
        return;
    }

    const QString _strSuggestedName =
        mResultLastSuccessful.strOutputLayerName.trimmed().isEmpty()
        ? QStringLiteral("query_result")
        : mResultLastSuccessful.strOutputLayerName;
    const QString _strOutputPath = QFileDialog::getSaveFileName(
        this,
        tr("导出查询结果"),
        QStringLiteral("%1.geojson").arg(_strSuggestedName),
        tr("GeoJSON (*.geojson);;Shapefile (*.shp)"));
    if (_strOutputPath.isEmpty()) {
        return;
    }

    const QString _strExt = QFileInfo(_strOutputPath).suffix().toLower();
    const QString _strDriverName = (_strExt == QStringLiteral("shp"))
        ? QStringLiteral("ESRI Shapefile")
        : QStringLiteral("GeoJSON");

    QgsVectorLayer _layerSource(
        mResultLastSuccessful.strOutputPath,
        _strSuggestedName,
        QStringLiteral("ogr"));
    if (!_layerSource.isValid()) {
        QMessageBox::warning(this,
            tr("导出失败"),
            tr("无法读取结果图层：%1").arg(mResultLastSuccessful.strOutputPath));
        return;
    }

    QgsVectorFileWriter::SaveVectorOptions _options;
    _options.driverName = _strDriverName;
    _options.fileEncoding = QStringLiteral("UTF-8");
    _options.actionOnExistingFile = QgsVectorFileWriter::CreateOrOverwriteFile;

    std::unique_ptr<QgsVectorFileWriter> _pWriter(
        QgsVectorFileWriter::create(
            _strOutputPath,
            _layerSource.fields(),
            _layerSource.wkbType(),
            _layerSource.crs(),
            QgsProject::instance()->transformContext(),
            _options));

    if (!_pWriter || _pWriter->hasError() != QgsVectorFileWriter::NoError) {
        const QString _strError =
            (_pWriter != nullptr && !_pWriter->errorMessage().trimmed().isEmpty())
            ? _pWriter->errorMessage()
            : tr("未知写出错误");
        QMessageBox::warning(this,
            tr("导出失败"),
            tr("创建导出文件失败：%1").arg(_strError));
        return;
    }

    int _nExportedCount = 0;
    QgsFeature _featureCurrent;
    QgsFeatureIterator _itFeature = _layerSource.getFeatures();
    while (_itFeature.nextFeature(_featureCurrent)) {
        if (!_pWriter->addFeature(_featureCurrent)) {
            QMessageBox::warning(this,
                tr("导出失败"),
                tr("写入导出要素失败"));
            return;
        }
        ++_nExportedCount;
    }

    _pWriter.reset();
    mpctrlLabelStatus->setText(tr("  已导出结果  "));
    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(
            tr("[导出] 查询结果已导出: %1（%2 个要素）")
                .arg(_strOutputPath)
                .arg(_nExportedCount));
    }
    QMessageBox::information(this,
        tr("导出完成"),
        tr("已导出 %1 个要素到：\n%2")
            .arg(_nExportedCount)
            .arg(_strOutputPath));
}

void MainWindow::onNavPan()
{
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pCanvas != nullptr) {
        _pCanvas->setMapTool(MapToolType::Pan);
    }
    mpctrlLabelStatus->setText(tr("  工具: 平移  "));
}

void MainWindow::onNavZoomIn()
{
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pCanvas != nullptr) {
        _pCanvas->setMapTool(MapToolType::ZoomIn);
    }
    mpctrlLabelStatus->setText(tr("  工具: 放大  "));
}

void MainWindow::onNavZoomOut()
{
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pCanvas != nullptr) {
        _pCanvas->setMapTool(MapToolType::ZoomOut);
    }
    mpctrlLabelStatus->setText(tr("  工具: 缩小  "));
}

void MainWindow::onNavFitAll()
{
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pCanvas != nullptr) {
        _pCanvas->zoomToFullExtent();
    }
    mpctrlLabelStatus->setText(tr("  工具: 全图  "));
}

void MainWindow::onBufferAnalysis()
{
    openToolShortcut("buffer_analysis");
}

void MainWindow::onOverlayAnalysis()
{
    openToolShortcut("overlay_analysis");
}

void MainWindow::onSpatialQuery()
{
    openToolShortcut("spatial_query");
}

void MainWindow::onRasterCalc()
{
    openToolShortcut("raster_calc");
}

void MainWindow::onNew3DView()
{
    const QString _strDemPath = QFileDialog::getOpenFileName(
        this,
        tr("新建3D视图"),
        QString(),
        tr("DEM/栅格数据 (*.tif *.tiff *.img *.asc);;全部文件 (*)"));
    if (_strDemPath.isEmpty()) {
        return;
    }

    QMainWindow* _pWindow3D = new QMainWindow();
    _pWindow3D->setAttribute(Qt::WA_DeleteOnClose);

    Map3DViewWidget* _pView3D = new Map3DViewWidget(_pWindow3D);
    if (!_pView3D->initializeFromDem(_strDemPath)) {
        delete _pWindow3D;
        QMessageBox::warning(this,
            tr("新建3D视图"),
            tr("无法加载所选 DEM/栅格数据，请确认文件有效: %1").arg(_strDemPath));
        return;
    }

    const QString _strLayerName = QFileInfo(_strDemPath).fileName();
    _pWindow3D->setCentralWidget(_pView3D);
    _pWindow3D->setWindowTitle(tr("3D视图 - %1").arg(_strLayerName));
    _pWindow3D->resize(1100, 760);
    _pWindow3D->show();

    mpctrlLabelStatus->setText(
        tr("  已创建3D视图: %1  ").arg(_strLayerName));
}

void MainWindow::onAIAnalyze()
{
    mpctrlDockAI->focusInput();
}

void MainWindow::onAIChat()
{
    mpctrlDockAI->focusInput();
}

void MainWindow::onAbout()
{
    QMessageBox::about(this,
        tr("关于 GeoAI 智能空间分析平台"),
        tr("GeoAI 智能空间分析平台\n版本：1.0.0\n"));
}

void MainWindow::onLayerLoaded(const LayerInfo& _layerInfo)
{
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pCanvas != nullptr) {
        _pCanvas->loadLayer(_layerInfo);
    }

    mpctrlLabelStatus->setText(tr("  就绪  "));
    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(
            tr("[图层] 已添加到地图: %1 (%2)")
                .arg(_layerInfo.strFilePath, _layerInfo.strType));
    }
}

void MainWindow::onAnalysisAssetReady(const AnalysisDataAsset& _assetReady)
{
    AnalysisDataAsset _assetResolved = _assetReady;
    if (_assetResolved.bNeedsUserChoice) {
        const DataAssetType _eChosenType = resolveAssetChoice(_assetResolved);
        if (_eChosenType == DataAssetType::None) {
            mpctrlLabelStatus->setText(tr("  已取消导入  "));
            return;
        }

        if (_eChosenType != _assetResolved.eAssetType) {
            AnalysisDataAsset _assetAlternate;
            QString _strError;
            if (!mpDataService->buildAlternateAsset(
                _assetResolved, _eChosenType, _assetAlternate, _strError)) {
                onDataLoadFailed(_strError);
                return;
            }
            _assetResolved = _assetAlternate;
        }

        _assetResolved.bNeedsUserChoice = false;
        _assetResolved.strChoicePrompt.clear();
    }

    const AnalysisDataAsset _assetStored = mpDataRepository->upsertAsset(_assetResolved);
    ensureWorkspaceVisible();
    mpctrlDockAnalysisWorkspace->showDataPage(
        tr("已导入数据资产：%1\n类型：%2\n能力：%3")
            .arg(_assetStored.strName,
                dataAssetTypeDisplayName(_assetStored.eAssetType),
                capabilityText(_assetStored.flagsCapabilities)));

    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(
            tr("[数据资产] %1 -> %2 | 能力：%3")
                .arg(_assetStored.strSourcePath,
                    dataAssetTypeDisplayName(_assetStored.eAssetType),
                    capabilityText(_assetStored.flagsCapabilities)));
    }

    if (mmapLastSuccessfulRuns.contains(_assetStored.strAssetId)
        && assetSupportsTool(_assetStored,
            mmapLastSuccessfulRuns.value(_assetStored.strAssetId).strToolId)) {
        mpctrlLabelStatus->setText(tr("  正在按该资产最近一次成功配置自动刷新...  "));
        runToolForAsset(_assetStored,
            mmapLastSuccessfulRuns.value(_assetStored.strAssetId));
        return;
    }

    const QString _strMessage = tr(
        "已选择数据资产“%1”。\n"
        "请切换到 Tools 页运行兼容分析工具。")
        .arg(_assetStored.strName);
    mpctrlDockAnalysisWorkspace->clearCurrentResult(_strMessage);
    mpVisualizationManager->clearView(_strMessage);
    mstrDisplayedResultAssetId.clear();
    mResultLastSuccessful = AnalysisResult();
    mpctrlLabelStatus->setText(tr("  就绪  "));
}

void MainWindow::onAnalysisAssetDeleteRequested(const QString& _strAssetId)
{
    if (mpDataRepository == nullptr) {
        return;
    }

    const AnalysisDataAsset _assetTarget =
        mpDataRepository->findAssetById(_strAssetId);
    if (_assetTarget.strAssetId.trimmed().isEmpty()) {
        QMessageBox::information(this,
            tr("删除数据资产"),
            tr("未找到要删除的数据资产记录。"));
        return;
    }

    removeAnalysisAssetAndLinkedLayers(_assetTarget, true);
}

void MainWindow::onDataLoadFailed(const QString& _strErrorMsg)
{
    mpctrlLabelStatus->setText(tr("  数据加载失败  "));
    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(tr("[错误] %1").arg(_strErrorMsg));
    }
    QMessageBox::warning(this, tr("数据加载失败"), _strErrorMsg);
}

void MainWindow::onAnalysisFinished(const AnalysisResult& _result)
{
    mstrDisplayedResultAssetId = _result.strSourceAssetId;
    mResultLastSuccessful = _result;
    mpctrlLabelStatus->setText(tr("  分析完成  "));
    mpctrlDockAnalysisWorkspace->setCurrentResult(_result);
    mpctrlDockAnalysisWorkspace->addResultHistory(_result);

    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(tr("[分析] %1").arg(_result.strDesc));
    }

    mpVisualizationManager->updateFromResult(_result);

    if (_result.bHasOutputLayer
        && !_result.strOutputPath.trimmed().isEmpty()
        && mpDataService != nullptr) {
        mpctrlLabelStatus->setText(tr("  正在添加结果图层...  "));
        mpDataService->loadAnalysisResultLayer(_result);
    }

    if (mbHasPendingRun
        && mstrPendingRunAssetId == _result.strSourceAssetId) {
        mmapLastSuccessfulRuns[mstrPendingRunAssetId] = mConfigPendingRun;
    }
    clearPendingRun();
}

void MainWindow::onAnalysisFailed(const AnalysisResult& _result)
{
    mstrDisplayedResultAssetId = _result.strSourceAssetId;
    mResultLastSuccessful = AnalysisResult();
    mpctrlLabelStatus->setText(tr("  分析失败  "));
    mpctrlDockAnalysisWorkspace->setCurrentResult(_result);
    mpctrlDockAnalysisWorkspace->addResultHistory(_result);

    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(tr("[分析失败] %1").arg(_result.strDesc));
    }

    mpVisualizationManager->clearView(_result.strDesc);
    clearPendingRun();
}

void MainWindow::onAnalysisProgress(int _nPercent)
{
    mpctrlLabelStatus->setText(tr("  分析中 %1%...  ").arg(_nPercent));
}

void MainWindow::onCoordChanged(double _dLon, double _dLat)
{
    mpctrlLabelCoord->setText(
        tr("  经度: %1  纬度: %2  ")
            .arg(_dLon, 0, 'f', 6)
            .arg(_dLat, 0, 'f', 6));
}

void MainWindow::onScaleChanged(double _dScale)
{
    mpctrlLabelScale->setText(
        tr("  比例尺: 1:%1  ").arg(static_cast<long long>(_dScale)));
}

void MainWindow::onActiveCanvasChanged(MapCanvasWidget* _pCanvas)
{
    reconnectCanvasSignals(_pCanvas);
    mpctrlLabelCoord->setText(tr("  经度: --  纬度: --  "));
    mpctrlLabelScale->setText(tr("  比例尺: 1:--  "));
}

void MainWindow::onCurrentAnalysisAssetChanged(const AnalysisDataAsset& _assetCurrent)
{
    if (_assetCurrent.strAssetId == mstrDisplayedResultAssetId) {
        return;
    }

    const QString _strMessage = tr(
        "当前已切换到数据资产“%1”。\n"
        "结果区不会沿用其他资产的分析结果，请重新运行分析。")
        .arg(_assetCurrent.strName);
    mpctrlDockAnalysisWorkspace->clearCurrentResult(_strMessage);
    mpVisualizationManager->clearView(_strMessage);
    mstrDisplayedResultAssetId.clear();
    mResultLastSuccessful = AnalysisResult();
}

void MainWindow::onCurrentAnalysisAssetCleared()
{
    const QString _strMessage = tr("当前没有已选择的分析资产。");
    mpctrlDockAnalysisWorkspace->clearCurrentResult(_strMessage);
    mpVisualizationManager->clearView(_strMessage);
    mstrDisplayedResultAssetId.clear();
    mResultLastSuccessful = AnalysisResult();
}

void MainWindow::onLayerTreeContextMenuRequested(const QPoint& _posMenu)
{
    if (mpctrlLayerTreeView == nullptr) {
        return;
    }

    const QModelIndex _indexCurrent = mpctrlLayerTreeView->indexAt(_posMenu);
    if (!_indexCurrent.isValid()) {
        return;
    }

    mpctrlLayerTreeView->setCurrentIndex(_indexCurrent);
    QgsMapLayer* _pLayerCurrent = currentSelectedLayer();
    if (_pLayerCurrent == nullptr) {
        return;
    }

    QMenu _menuLayer(mpctrlLayerTreeView);
    QAction* _pActionZoom = _menuLayer.addAction(tr("缩放到图层范围"));
    _menuLayer.addSeparator();
    QAction* _pActionSimpleSymbol = _menuLayer.addAction(tr("简单符号"));
    QAction* _pActionFieldRenderer = _menuLayer.addAction(tr("字段渲染"));
    QAction* _pActionRasterGray = _menuLayer.addAction(tr("栅格灰度拉伸（2%-98%）"));
    QAction* _pActionRasterDefault = _menuLayer.addAction(tr("恢复栅格默认渲染"));
    QAction* _pActionRasterPseudo = _menuLayer.addAction(tr("栅格伪彩色"));
    _menuLayer.addSeparator();
    QAction* _pActionRemove = _menuLayer.addAction(tr("移除图层"));

    const bool _bIsVector = qobject_cast<QgsVectorLayer*>(_pLayerCurrent) != nullptr;
    const bool _bIsRaster = qobject_cast<QgsRasterLayer*>(_pLayerCurrent) != nullptr;
    _pActionSimpleSymbol->setEnabled(_bIsVector);
    _pActionFieldRenderer->setEnabled(_bIsVector);
    _pActionRasterGray->setEnabled(_bIsRaster);
    _pActionRasterDefault->setEnabled(_bIsRaster);
    _pActionRasterPseudo->setEnabled(_bIsRaster);

    QAction* _pActionTriggered = _menuLayer.exec(
        mpctrlLayerTreeView->viewport()->mapToGlobal(_posMenu));
    if (_pActionTriggered == _pActionZoom) {
        onZoomToSelectedLayer();
        return;
    }
    if (_pActionTriggered == _pActionSimpleSymbol) {
        onApplySimpleSymbol();
        return;
    }
    if (_pActionTriggered == _pActionFieldRenderer) {
        onApplyFieldRenderer();
        return;
    }
    if (_pActionTriggered == _pActionRasterGray) {
        onApplyRasterGrayRenderer();
        return;
    }
    if (_pActionTriggered == _pActionRasterDefault) {
        onApplyRasterDefaultRenderer();
        return;
    }
    if (_pActionTriggered == _pActionRasterPseudo) {
        onApplyRasterPseudoColorRenderer();
        return;
    }
    if (_pActionTriggered == _pActionRemove) {
        onRemoveSelectedLayer();
    }
}

void MainWindow::onRemoveSelectedLayer()
{
    QgsMapLayer* _pLayerCurrent = currentSelectedLayer();
    if (_pLayerCurrent == nullptr) {
        return;
    }

    const QString _strLayerId = _pLayerCurrent->id();
    const AnalysisDataAsset _assetLinked = findAssetForLayer(_pLayerCurrent);
    if (!_assetLinked.strAssetId.trimmed().isEmpty()) {
        removeAnalysisAssetAndLinkedLayers(_assetLinked, true);
        return;
    }

    QString _strLayerName;
    if (!removeMapLayerById(_strLayerId, &_strLayerName)) {
        return;
    }

    mpctrlLabelStatus->setText(tr("  已移除图层: %1  ").arg(_strLayerName));
    if (mpctrlLogView != nullptr) {
        mpctrlLogView->append(tr("[图层] 已移除: %1").arg(_strLayerName));
    }
}

void MainWindow::onZoomToSelectedLayer()
{
    QgsMapLayer* _pLayerCurrent = currentSelectedLayer();
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pLayerCurrent == nullptr || _pCanvas == nullptr) {
        return;
    }

    _pCanvas->zoomToLayerExtent(_pLayerCurrent->id());
    mpctrlLabelStatus->setText(
        tr("  已缩放到图层范围: %1  ").arg(_pLayerCurrent->name()));
}

void MainWindow::onApplySimpleSymbol()
{
    QgsMapLayer* _pLayerCurrent = currentSelectedLayer();
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pLayerCurrent == nullptr || _pCanvas == nullptr) {
        return;
    }
    if (qobject_cast<QgsVectorLayer*>(_pLayerCurrent) == nullptr) {
        QMessageBox::information(this,
            tr("简单符号"),
            tr("简单符号仅支持矢量图层。"));
        return;
    }

    const QColor _colorSymbol = QColorDialog::getColor(
        QColor("#1677FF"),
        this,
        tr("选择符号颜色"),
        QColorDialog::ShowAlphaChannel);
    if (!_colorSymbol.isValid()) {
        return;
    }

    bool _bOk = false;
    const double _dSizeValue = QInputDialog::getDouble(
        this,
        tr("简单符号"),
        tr("线宽 / 点大小"),
        1.5,
        0.1,
        30.0,
        1,
        &_bOk);
    if (!_bOk) {
        return;
    }

    const int _nOpacityPercent = QInputDialog::getInt(
        this,
        tr("简单符号"),
        tr("透明度百分比"),
        85,
        5,
        100,
        5,
        &_bOk);
    if (!_bOk) {
        return;
    }

    _pCanvas->applyVectorSimpleStyle(
        _pLayerCurrent->id(),
        _colorSymbol,
        _dSizeValue,
        static_cast<double>(_nOpacityPercent) / 100.0);
    mpctrlLabelStatus->setText(
        tr("  已应用简单符号: %1  ").arg(_pLayerCurrent->name()));
}

void MainWindow::onApplyFieldRenderer()
{
    QgsMapLayer* _pLayerCurrent = currentSelectedLayer();
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    QgsVectorLayer* _pVectorLayer = qobject_cast<QgsVectorLayer*>(_pLayerCurrent);
    if (_pVectorLayer == nullptr || _pCanvas == nullptr) {
        QMessageBox::information(this,
            tr("字段渲染"),
            tr("字段渲染仅支持矢量图层。"));
        return;
    }

    QStringList _vFieldNames;
    const QgsFields _fields = _pVectorLayer->fields();
    for (int _nFieldIdx = 0; _nFieldIdx < _fields.size(); ++_nFieldIdx) {
        _vFieldNames << _fields.at(_nFieldIdx).name();
    }
    if (_vFieldNames.isEmpty()) {
        QMessageBox::information(this,
            tr("字段渲染"),
            tr("当前图层没有可用字段。"));
        return;
    }

    bool _bOk = false;
    const QString _strFieldName = QInputDialog::getItem(
        this,
        tr("字段渲染"),
        tr("选择字段"),
        _vFieldNames,
        0,
        false,
        &_bOk);
    if (!_bOk || _strFieldName.trimmed().isEmpty()) {
        return;
    }

    const int _nClassCount = QInputDialog::getInt(
        this,
        tr("字段渲染"),
        tr("数值字段分级数量"),
        5,
        2,
        9,
        1,
        &_bOk);
    if (!_bOk) {
        return;
    }

    _pCanvas->applyVectorFieldRenderer(
        _pLayerCurrent->id(),
        _strFieldName,
        _nClassCount);
    mpctrlLabelStatus->setText(
        tr("  已按字段渲染: %1.%2  ")
            .arg(_pLayerCurrent->name(), _strFieldName));
}

void MainWindow::onApplyRasterGrayRenderer()
{
    QgsMapLayer* _pLayerCurrent = currentSelectedLayer();
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pLayerCurrent == nullptr || _pCanvas == nullptr) {
        return;
    }
    if (qobject_cast<QgsRasterLayer*>(_pLayerCurrent) == nullptr) {
        QMessageBox::information(this,
            tr("栅格灰度拉伸"),
            tr("灰度拉伸仅支持栅格图层。"));
        return;
    }
    if (_pCanvas->isRasterGrayRendererApplied(_pLayerCurrent->id())) {
        QMessageBox::information(this,
            tr("栅格灰度拉伸"),
            tr("当前栅格已经应用过灰度拉伸。"));
        return;
    }

    const QString _strLayerName = _pLayerCurrent->name();
    const QString _strNewLayerId =
        _pCanvas->applyRasterGrayRenderer(_pLayerCurrent->id());
    if (_strNewLayerId.isEmpty()) {
        QMessageBox::warning(this,
            tr("栅格灰度拉伸"),
            tr("灰度拉伸失败，请确认栅格数据有效后重试。"));
        return;
    }
    selectLayerById(_strNewLayerId);
    mpctrlLabelStatus->setText(
        tr("  已应用栅格灰度拉伸（2%-98%）: %1  ").arg(_strLayerName));
}

void MainWindow::onApplyRasterDefaultRenderer()
{
    QgsMapLayer* _pLayerCurrent = currentSelectedLayer();
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pLayerCurrent == nullptr || _pCanvas == nullptr) {
        return;
    }
    if (qobject_cast<QgsRasterLayer*>(_pLayerCurrent) == nullptr) {
        QMessageBox::information(this,
            tr("恢复栅格默认渲染"),
            tr("恢复默认渲染仅支持栅格图层。"));
        return;
    }
    if (_pCanvas->isRasterDefaultRendererApplied(_pLayerCurrent->id())) {
        QMessageBox::information(this,
            tr("恢复栅格默认渲染"),
            tr("当前栅格已经是默认渲染。"));
        return;
    }

    const QString _strLayerName = _pLayerCurrent->name();
    const QString _strNewLayerId =
        _pCanvas->applyRasterDefaultRenderer(_pLayerCurrent->id());
    if (_strNewLayerId.isEmpty()) {
        QMessageBox::warning(this,
            tr("恢复栅格默认渲染"),
            tr("恢复默认渲染失败，请确认栅格数据有效后重试。"));
        return;
    }
    selectLayerById(_strNewLayerId);
    mpctrlLabelStatus->setText(
        tr("  已恢复栅格默认渲染: %1  ").arg(_strLayerName));
}

void MainWindow::onApplyRasterPseudoColorRenderer()
{
    QgsMapLayer* _pLayerCurrent = currentSelectedLayer();
    MapCanvasWidget* _pCanvas = mpMapCanvasManager->activeCanvas();
    if (_pLayerCurrent == nullptr || _pCanvas == nullptr) {
        return;
    }
    if (qobject_cast<QgsRasterLayer*>(_pLayerCurrent) == nullptr) {
        QMessageBox::information(this,
            tr("栅格伪彩色"),
            tr("伪彩色渲染仅支持栅格图层。"));
        return;
    }
    if (_pCanvas->isRasterPseudoColorRendererApplied(_pLayerCurrent->id())) {
        QMessageBox::information(this,
            tr("栅格伪彩色"),
            tr("当前栅格已经应用过伪彩色渲染。"));
        return;
    }

    const QString _strLayerName = _pLayerCurrent->name();
    const QString _strNewLayerId =
        _pCanvas->applyRasterPseudoColorRenderer(_pLayerCurrent->id());
    if (_strNewLayerId.isEmpty()) {
        QMessageBox::warning(this,
            tr("栅格伪彩色"),
            tr("伪彩色渲染失败，请确认栅格数据有效后重试。"));
        return;
    }
    selectLayerById(_strNewLayerId);
    mpctrlLabelStatus->setText(
        tr("  已应用栅格伪彩色: %1  ").arg(_strLayerName));
}

void MainWindow::onBasicStatisticsRequested()
{
    AnalysisRunConfig _configRun;
    _configRun.strToolId = "basic_statistics";
    runToolForCurrentAsset(_configRun);
}

void MainWindow::onFrequencyStatisticsRequested(int _nFrequencyBins)
{
    AnalysisRunConfig _configRun;
    _configRun.strToolId = "frequency_statistics";
    _configRun.nFrequencyBins = _nFrequencyBins;
    runToolForCurrentAsset(_configRun);
}

void MainWindow::onNeighborhoodAnalysisRequested(int _nNeighborhoodWindow)
{
    AnalysisRunConfig _configRun;
    _configRun.strToolId = "neighborhood_analysis";
    _configRun.nNeighborhoodWindow = _nNeighborhoodWindow;
    runToolForCurrentAsset(_configRun);
}

void MainWindow::onBufferAnalysisRequested(double _dBufferDistance,
    int _nBufferSegments)
{
    AnalysisRunConfig _configRun;
    _configRun.strToolId = "buffer_analysis";
    _configRun.dBufferDistance = _dBufferDistance;
    _configRun.nBufferSegments = _nBufferSegments;
    runToolForCurrentAsset(_configRun);
}

void MainWindow::onOverlayAnalysisRequested(const QString& _strOverlayAssetId,
    OverlayOperationType _eOperation)
{
    AnalysisRunConfig _configRun;
    _configRun.strToolId = "overlay_analysis";
    _configRun.strOverlayAssetId = _strOverlayAssetId;
    _configRun.eOverlayOperation = _eOperation;
    runToolForCurrentAsset(_configRun);
}

void MainWindow::onSpatialQueryRequested(const QString& _strTargetAssetId,
    const QString& _strRelationId)
{
    AnalysisRunConfig _configRun;
    _configRun.strToolId = "spatial_query";
    _configRun.strSpatialTargetAssetId = _strTargetAssetId;
    _configRun.strSpatialRelationId = _strRelationId;
    runToolForCurrentAsset(_configRun);
}

void MainWindow::onAttributeQueryRequested(const QString& _strFieldName,
    const QString& _strOperatorId,
    const QString& _strValueText)
{
    AnalysisRunConfig _configRun;
    _configRun.strToolId = "attribute_query";
    _configRun.strQueryFieldName = _strFieldName;
    _configRun.strQueryOperatorId = _strOperatorId;
    _configRun.strQueryValueText = _strValueText;
    runToolForCurrentAsset(_configRun);
}
