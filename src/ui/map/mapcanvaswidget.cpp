#include "mapcanvaswidget.h"

#include <limits>

#include <QVBoxLayout>
#include <QDebug>
#include <QFileInfo>
#include <QColor>
#include <QMetaType>
#include <QSet>
#include <QVariant>
#include <QtMath>

#include <qgsmapcanvas.h>
#include <qgsmaptoolpan.h>
#include <qgsmaptoolzoom.h>
#include <qgscoordinatetransform.h>
#include <qgsvectorlayer.h>
#include <qgsrasterlayer.h>
#include <qgsrasterdataprovider.h>
#include <qgsrasterbandstats.h>
#include <qgsproject.h>
#include <qgscoordinatereferencesystem.h>
#include <qgsexception.h>
#include <qgspointxy.h>
#include <qgsrectangle.h>
#include <qgslayertree.h>
#include <qgsfield.h>
#include <qgsfields.h>
#include <qgsfeature.h>
#include <qgsfeatureiterator.h>
#include <qgsfillsymbol.h>
#include <qgslinesymbol.h>
#include <qgsmarkersymbol.h>
#include <qgssinglesymbolrenderer.h>
#include <qgscategorizedsymbolrenderer.h>
#include <qgsgraduatedsymbolrenderer.h>
#include <qgsrendererrange.h>
#include <qgssinglebandgrayrenderer.h>
#include <qgssinglebandpseudocolorrenderer.h>
#include <qgscontrastenhancement.h>
#include <qgsrastershader.h>
#include <qgscolorrampshader.h>
#include <qgssymbol.h>

namespace
{

bool isNumericVariantType(QMetaType::Type _eType)
{
    return _eType == QMetaType::Int
        || _eType == QMetaType::UInt
        || _eType == QMetaType::LongLong
        || _eType == QMetaType::ULongLong
        || _eType == QMetaType::Double;
}

QColor categoryColor(int _nCategoryIdx)
{
    static const QList<QColor> S_V_COLORS = {
        QColor("#1677FF"),
        QColor("#52C41A"),
        QColor("#FAAD14"),
        QColor("#EB2F96"),
        QColor("#13C2C2"),
        QColor("#722ED1"),
        QColor("#FA541C"),
        QColor("#2F54EB")
    };
    return S_V_COLORS.at(_nCategoryIdx % S_V_COLORS.size());
}

QColor rampColor(double _dRatio)
{
    const double _dClamped = qBound(0.0, _dRatio, 1.0);
    const QColor _colorStart("#E6F4FF");
    const QColor _colorEnd("#0958D9");
    const int _nRed = static_cast<int>(
        _colorStart.red() + (_colorEnd.red() - _colorStart.red()) * _dClamped);
    const int _nGreen = static_cast<int>(
        _colorStart.green() + (_colorEnd.green() - _colorStart.green()) * _dClamped);
    const int _nBlue = static_cast<int>(
        _colorStart.blue() + (_colorEnd.blue() - _colorStart.blue()) * _dClamped);
    return QColor(_nRed, _nGreen, _nBlue, 220);
}

} // namespace


// ════════════════════════════════════════════════════════
//  构造 / 析构
// ════════════════════════════════════════════════════════
MapCanvasWidget::MapCanvasWidget(QWidget* _pParent)
    : QWidget(_pParent)
    , mpCanvas(nullptr)
    , mpToolPan(nullptr)
    , mpToolZoomIn(nullptr)
    , mpToolZoomOut(nullptr)
{
    initCanvas();
}

MapCanvasWidget::~MapCanvasWidget()
{
    // 地图工具的父对象是 mpCanvas，会随 mpCanvas 析构自动释放
    // mpCanvas 的父对象是 this，也会自动释放
    // mpvLayers 中的图层由 QgsProject 管理，不在此处 delete
}

// ════════════════════════════════════════════════════════
//  initCanvas
//  创建 QgsMapCanvas，设置默认 CRS（WGS84），初始化工具
// ════════════════════════════════════════════════════════
void MapCanvasWidget::initCanvas()
{
    // ── 创建画布 ──────────────────────────────────────
    mpCanvas = new QgsMapCanvas(this);
    mpCanvas->setCanvasColor(Qt::white);
    mpCanvas->enableAntiAliasing(true);

    // ── 设置默认 CRS：WGS84 (EPSG:4326) ──────────────
    QgsCoordinateReferenceSystem _defaultCrs;
    _defaultCrs.createFromSrid(4326);
    mpCanvas->setDestinationCrs(_defaultCrs);

    // 开启动态投影（on-the-fly CRS transformation）
    mpCanvas->setCachingEnabled(true);
    mpCanvas->setParallelRenderingEnabled(true);
    mpCanvas->setPreviewJobsEnabled(true);
    mpCanvas->setMapUpdateInterval(250);

    // ── 布局：让画布填满整个 Widget ──────────────────
    QVBoxLayout* _pLayout = new QVBoxLayout(this);
    _pLayout->setContentsMargins(0, 0, 0, 0);
    _pLayout->setSpacing(0);
    _pLayout->addWidget(mpCanvas);
    setLayout(_pLayout);

    // ── 初始化地图工具 ────────────────────────────────
    mpToolPan     = new QgsMapToolPan(mpCanvas);
    mpToolZoomIn  = new QgsMapToolZoom(mpCanvas, false); // false = zoom in
    mpToolZoomOut = new QgsMapToolZoom(mpCanvas, true);  // true  = zoom out

    // 默认激活平移工具
    mpCanvas->setMapTool(mpToolPan);

    // ── 连接画布内部信号 ──────────────────────────────
    connect(mpCanvas, &QgsMapCanvas::xyCoordinates,
        this, &MapCanvasWidget::onXYCoordinates);
    connect(mpCanvas, &QgsMapCanvas::scaleChanged,
        this, &MapCanvasWidget::onScaleChanged);
}

// ════════════════════════════════════════════════════════
//  mapCanvas
// ════════════════════════════════════════════════════════
QgsMapCanvas* MapCanvasWidget::mapCanvas() const
{
    return mpCanvas;
}

// ════════════════════════════════════════════════════════
//  loadFromPath
//  自动识别文件类型，创建 QGIS 图层并加入画布
// ════════════════════════════════════════════════════════
void MapCanvasWidget::loadFromPath(const QString& _strFilePath)
{
    LayerInfo _layerInfo;
    _layerInfo.strFilePath = _strFilePath;
    _layerInfo.strSourceUri = _strFilePath;
    _layerInfo.strName = QFileInfo(_strFilePath).baseName();
    _layerInfo.strType = _strFilePath.toLower().endsWith(".tif")
        || _strFilePath.toLower().endsWith(".tiff")
        || _strFilePath.toLower().endsWith(".img")
        ? QStringLiteral("raster")
        : QStringLiteral("vector");
    _layerInfo.strProviderKey = (_layerInfo.strType == QStringLiteral("raster"))
        ? QStringLiteral("gdal")
        : QStringLiteral("ogr");
    _layerInfo.bVisible = true;
    loadLayer(_layerInfo);
}

void MapCanvasWidget::loadLayer(const LayerInfo& _layerInfo)
{
    const QString _strSourceUri = _layerInfo.strSourceUri.trimmed().isEmpty()
        ? _layerInfo.strFilePath
        : _layerInfo.strSourceUri;
    const QString _strLayerName = _layerInfo.strName.trimmed().isEmpty()
        ? QFileInfo(_layerInfo.strFilePath).baseName()
        : _layerInfo.strName;
    const QString _strProviderKey = _layerInfo.strProviderKey.trimmed().isEmpty()
        ? QStringLiteral("ogr")
        : _layerInfo.strProviderKey;

    if (_strSourceUri.isEmpty())
    {
        qWarning() << "[MapCanvasWidget] loadFromPath: empty path.";
        return;
    }

    QgsMapLayer* _pLayer = nullptr;
    const QString _strLower = _strSourceUri.toLower();

    // ── 根据扩展名选择图层类型 ────────────────────────
    if (_layerInfo.strType == QStringLiteral("vector")
        || _strLower.endsWith(".shp")
        || _strLower.endsWith(".geojson")
        || _strLower.contains(QStringLiteral("|layername=")))
    {
        QgsVectorLayer* _pVecLayer = new QgsVectorLayer(
            _strSourceUri,
            _strLayerName,
            _strProviderKey);

        if (!_pVecLayer->isValid())
        {
            qWarning() << "[MapCanvasWidget] Failed to load vector layer:"
                       << _strSourceUri;
            delete _pVecLayer;
            return;
        }
        if (_layerInfo.bUseHighlightStyle)
        {
            applyHighlightStyle(_pVecLayer);
        }
        _pLayer = _pVecLayer;
    }
    else if (_strLower.endsWith(".tif")  ||
             _strLower.endsWith(".tiff") ||
             _strLower.endsWith(".img"))
    {
        QgsRasterLayer* _pRasLayer = new QgsRasterLayer(
            _strSourceUri,
            _strLayerName);

        if (!_pRasLayer->isValid())
        {
            qWarning() << "[MapCanvasWidget] Failed to load raster layer:"
                       << _strSourceUri;
            delete _pRasLayer;
            return;
        }
        _pRasLayer->setDefaultContrastEnhancement();
        _pLayer = _pRasLayer;
    }
    else
    {
        qWarning() << "[MapCanvasWidget] Unsupported file type:" << _strSourceUri;
        return;
    }

    // ── 注册到 QgsProject，并显式加入图层树根节点────────
    QgsProject::instance()->addMapLayer(_pLayer, false);
    QgsProject::instance()->layerTreeRoot()->insertLayer(0, _pLayer);

    // ── 加入本画布图层列表（新图层放在最上层）────────
    mpvLayers.prepend(_pLayer);
    refreshCanvasLayers();

    // 首次加载时缩放至该图层范围
    if (mpvLayers.size() == 1)
    {
        mpCanvas->setExtent(layerExtentInCanvasCrs(_pLayer));
    }
    mpCanvas->refresh();

    qDebug() << "[MapCanvasWidget] Layer loaded:" << _pLayer->name()
             << "CRS:" << _pLayer->crs().authid();
}

// ════════════════════════════════════════════════════════
//  removeLayer
// ════════════════════════════════════════════════════════
void MapCanvasWidget::removeLayer(const QString& _strLayerId)
{
    for (int _nFilesIdx = 0; _nFilesIdx < mpvLayers.size(); ++_nFilesIdx)
    {
        if (mpvLayers[_nFilesIdx]->id() == _strLayerId)
        {
            mpvLayers.removeAt(_nFilesIdx);
            QgsProject::instance()->removeMapLayer(_strLayerId);
            refreshCanvasLayers();
            mpCanvas->refresh();
            return;
        }
    }
    qWarning() << "[MapCanvasWidget] removeLayer: layer not found:" << _strLayerId;
}

// ════════════════════════════════════════════════════════
//  setLayerVisible
// ════════════════════════════════════════════════════════
void MapCanvasWidget::setLayerVisible(const QString& _strLayerId, bool _bVisible)
{
    for (QgsMapLayer* _pLayer : mpvLayers)
    {
        if (_pLayer->id() == _strLayerId)
        {
            // 通过 QgsProject 图层树控制可见性
            QgsLayerTree* _pRoot = QgsProject::instance()->layerTreeRoot();
            QgsLayerTreeLayer* _pTreeLayer = _pRoot->findLayer(_strLayerId);
            if (_pTreeLayer != nullptr)
            {
                _pTreeLayer->setItemVisibilityChecked(_bVisible);
            }
            mpCanvas->refresh();
            return;
        }
    }
    qWarning() << "[MapCanvasWidget] setLayerVisible: layer not found:" << _strLayerId;
}

// ════════════════════════════════════════════════════════
//  setMapTool
// ════════════════════════════════════════════════════════
void MapCanvasWidget::setMapTool(MapToolType _eToolType)
{
    switch (_eToolType)
    {
    case MapToolType::Pan:
        mpCanvas->setMapTool(mpToolPan);
        break;
    case MapToolType::ZoomIn:
        mpCanvas->setMapTool(mpToolZoomIn);
        break;
    case MapToolType::ZoomOut:
        mpCanvas->setMapTool(mpToolZoomOut);
        break;
    default:
        qWarning() << "[MapCanvasWidget] setMapTool: unknown tool type.";
        break;
    }
}

// ════════════════════════════════════════════════════════
//  zoomToFullExtent
// ════════════════════════════════════════════════════════
void MapCanvasWidget::zoomToFullExtent()
{
    if (mpvLayers.isEmpty())
    {
        return;
    }
    mpCanvas->zoomToFullExtent();
    mpCanvas->refresh();
}

// ════════════════════════════════════════════════════════
//  zoomToLayerExtent
// ════════════════════════════════════════════════════════
void MapCanvasWidget::zoomToLayerExtent(const QString& _strLayerId)
{
    for (QgsMapLayer* _pLayer : mpvLayers)
    {
        if (_pLayer != nullptr && _pLayer->id() == _strLayerId)
        {
            if (_pLayer->extent().isEmpty())
            {
                qWarning() << "[MapCanvasWidget] zoomToLayerExtent: empty extent:"
                           << _strLayerId;
                return;
            }

            mpCanvas->setExtent(layerExtentInCanvasCrs(_pLayer));
            mpCanvas->refresh();
            return;
        }
    }

    qWarning() << "[MapCanvasWidget] zoomToLayerExtent: layer not found:"
               << _strLayerId;
}

// ════════════════════════════════════════════════════════
//  setLayerOrder
//  按给定 ID 列表重新排列 mpvLayers，首项在最上层渲染
// ════════════════════════════════════════════════════════
void MapCanvasWidget::setLayerOrder(const QStringList& _vstrLayerIds)
{
    QList<QgsMapLayer*> _vReordered;
    for (const QString& _strId : _vstrLayerIds)
    {
        for (QgsMapLayer* _pLayer : mpvLayers)
        {
            if (_pLayer->id() == _strId)
            {
                _vReordered.append(_pLayer);
                break;
            }
        }
    }

    if (_vReordered.size() == mpvLayers.size())
    {
        mpvLayers = _vReordered;
        refreshCanvasLayers();
        mpCanvas->refresh();
    }
    else
    {
        qWarning() << "[MapCanvasWidget] setLayerOrder: ID list mismatch.";
    }
}

void MapCanvasWidget::applyVectorSimpleStyle(const QString& _strLayerId,
    const QColor& _colorSymbol,
    double _dSizeValue,
    double _dOpacity)
{
    QgsVectorLayer* _pVectorLayer =
        qobject_cast<QgsVectorLayer*>(findLayerById(_strLayerId));
    if (_pVectorLayer == nullptr) {
        qWarning() << "[MapCanvasWidget] applyVectorSimpleStyle: vector layer not found:"
                   << _strLayerId;
        return;
    }

    QgsSymbol* _pSymbol = QgsSymbol::defaultSymbol(_pVectorLayer->geometryType());
    if (_pSymbol == nullptr) {
        return;
    }

    applyCommonSymbolStyle(_pSymbol, _colorSymbol, _dSizeValue, _dOpacity);
    _pVectorLayer->setRenderer(new QgsSingleSymbolRenderer(_pSymbol));
    _pVectorLayer->triggerRepaint();
    mpCanvas->refresh();
}

void MapCanvasWidget::applyVectorFieldRenderer(const QString& _strLayerId,
    const QString& _strFieldName,
    int _nClassCount)
{
    QgsVectorLayer* _pVectorLayer =
        qobject_cast<QgsVectorLayer*>(findLayerById(_strLayerId));
    if (_pVectorLayer == nullptr) {
        qWarning() << "[MapCanvasWidget] applyVectorFieldRenderer: vector layer not found:"
                   << _strLayerId;
        return;
    }

    const int _nFieldIdx = _pVectorLayer->fields().indexOf(_strFieldName);
    if (_nFieldIdx < 0) {
        qWarning() << "[MapCanvasWidget] applyVectorFieldRenderer: field not found:"
                   << _strFieldName;
        return;
    }

    if (!isNumericField(_pVectorLayer, _strFieldName)) {
        QgsCategoryList _vCategories;
        QSet<QString> _setValues;
        QgsFeature _featureCurrent;
        QgsFeatureIterator _itFeature = _pVectorLayer->getFeatures();
        while (_itFeature.nextFeature(_featureCurrent)) {
            const QString _strValue = _featureCurrent.attribute(_nFieldIdx).toString();
            if (_setValues.contains(_strValue)) {
                continue;
            }
            _setValues.insert(_strValue);

            QgsSymbol* _pSymbol = QgsSymbol::defaultSymbol(_pVectorLayer->geometryType());
            if (_pSymbol == nullptr) {
                continue;
            }
            applyCommonSymbolStyle(
                _pSymbol,
                categoryColor(_vCategories.size()),
                1.0,
                0.85);
            _vCategories.append(QgsRendererCategory(
                _strValue,
                _pSymbol,
                _strValue.trimmed().isEmpty() ? tr("<空值>") : _strValue));

            if (_vCategories.size() >= 24) {
                break;
            }
        }

        if (_vCategories.isEmpty()) {
            return;
        }

        _pVectorLayer->setRenderer(
            new QgsCategorizedSymbolRenderer(_strFieldName, _vCategories));
        _pVectorLayer->triggerRepaint();
        mpCanvas->refresh();
        return;
    }

    double _dMinimumValue = std::numeric_limits<double>::max();
    double _dMaximumValue = -std::numeric_limits<double>::max();
    QgsFeature _featureCurrent;
    QgsFeatureIterator _itFeature = _pVectorLayer->getFeatures();
    while (_itFeature.nextFeature(_featureCurrent)) {
        bool _bOk = false;
        const double _dValue = _featureCurrent.attribute(_nFieldIdx).toDouble(&_bOk);
        if (!_bOk) {
            continue;
        }
        _dMinimumValue = qMin(_dMinimumValue, _dValue);
        _dMaximumValue = qMax(_dMaximumValue, _dValue);
    }

    if (_dMinimumValue > _dMaximumValue) {
        return;
    }

    const int _nResolvedClassCount = qBound(2, _nClassCount, 9);
    if (qFuzzyCompare(_dMinimumValue, _dMaximumValue)) {
        QgsSymbol* _pSymbol = QgsSymbol::defaultSymbol(_pVectorLayer->geometryType());
        if (_pSymbol == nullptr) {
            return;
        }
        applyCommonSymbolStyle(_pSymbol, QColor("#1677FF"), 1.0, 0.85);
        QgsRangeList _vRanges;
        _vRanges.append(QgsRendererRange(
            _dMinimumValue,
            _dMaximumValue,
            _pSymbol,
            QString::number(_dMinimumValue, 'f', 2)));
        _pVectorLayer->setRenderer(
            new QgsGraduatedSymbolRenderer(_strFieldName, _vRanges));
        _pVectorLayer->triggerRepaint();
        mpCanvas->refresh();
        return;
    }

    const double _dStep =
        (_dMaximumValue - _dMinimumValue) / static_cast<double>(_nResolvedClassCount);
    QgsRangeList _vRanges;
    for (int _nClassIdx = 0; _nClassIdx < _nResolvedClassCount; ++_nClassIdx) {
        const double _dLowerValue = _dMinimumValue + _dStep * _nClassIdx;
        const double _dUpperValue = (_nClassIdx == _nResolvedClassCount - 1)
            ? _dMaximumValue
            : _dMinimumValue + _dStep * (_nClassIdx + 1);

        QgsSymbol* _pSymbol = QgsSymbol::defaultSymbol(_pVectorLayer->geometryType());
        if (_pSymbol == nullptr) {
            continue;
        }
        applyCommonSymbolStyle(
            _pSymbol,
            rampColor(static_cast<double>(_nClassIdx)
                / qMax(1, _nResolvedClassCount - 1)),
            1.0,
            0.88);
        _vRanges.append(QgsRendererRange(
            _dLowerValue,
            _dUpperValue,
            _pSymbol,
            tr("%1 - %2")
                .arg(_dLowerValue, 0, 'f', 2)
                .arg(_dUpperValue, 0, 'f', 2)));
    }

    if (_vRanges.isEmpty()) {
        return;
    }

    _pVectorLayer->setRenderer(
        new QgsGraduatedSymbolRenderer(_strFieldName, _vRanges));
    _pVectorLayer->triggerRepaint();
    mpCanvas->refresh();
}

void MapCanvasWidget::applyRasterGrayRenderer(const QString& _strLayerId)
{
    QgsRasterLayer* _pRasterLayer =
        qobject_cast<QgsRasterLayer*>(findLayerById(_strLayerId));
    if (_pRasterLayer == nullptr || _pRasterLayer->dataProvider() == nullptr) {
        qWarning() << "[MapCanvasWidget] applyRasterGrayRenderer: raster layer not found:"
                   << _strLayerId;
        return;
    }

    const int _nBand = 1;
    const QgsRasterBandStats _statsBand =
        _pRasterLayer->dataProvider()->bandStatistics(_nBand);
    QgsSingleBandGrayRenderer* _pRenderer =
        new QgsSingleBandGrayRenderer(_pRasterLayer->dataProvider(), _nBand);
    QgsContrastEnhancement* _pContrast = new QgsContrastEnhancement(
        _pRasterLayer->dataProvider()->dataType(_nBand));
    _pContrast->setMinimumValue(_statsBand.minimumValue);
    _pContrast->setMaximumValue(_statsBand.maximumValue);
    _pContrast->setContrastEnhancementAlgorithm(
        QgsContrastEnhancement::StretchToMinimumMaximum);
    _pRenderer->setContrastEnhancement(_pContrast);

    _pRasterLayer->setRenderer(_pRenderer);
    _pRasterLayer->triggerRepaint();
    mpCanvas->refresh();
}

void MapCanvasWidget::applyRasterPseudoColorRenderer(const QString& _strLayerId)
{
    QgsRasterLayer* _pRasterLayer =
        qobject_cast<QgsRasterLayer*>(findLayerById(_strLayerId));
    if (_pRasterLayer == nullptr || _pRasterLayer->dataProvider() == nullptr) {
        qWarning() << "[MapCanvasWidget] applyRasterPseudoColorRenderer: raster layer not found:"
                   << _strLayerId;
        return;
    }

    const int _nBand = 1;
    const QgsRasterBandStats _statsBand =
        _pRasterLayer->dataProvider()->bandStatistics(_nBand);
    const double _dMinimumValue = _statsBand.minimumValue;
    const double _dMaximumValue = _statsBand.maximumValue;
    const double _dMiddleValue = (_dMinimumValue + _dMaximumValue) * 0.5;

    QList<QgsColorRampShader::ColorRampItem> _vItems;
    _vItems << QgsColorRampShader::ColorRampItem(
        _dMinimumValue, QColor("#2B6CB0"), tr("低值"));
    _vItems << QgsColorRampShader::ColorRampItem(
        _dMiddleValue, QColor("#F6E05E"), tr("中值"));
    _vItems << QgsColorRampShader::ColorRampItem(
        _dMaximumValue, QColor("#C53030"), tr("高值"));

    QgsColorRampShader* _pColorRamp = new QgsColorRampShader(
        _dMinimumValue,
        _dMaximumValue,
        nullptr,
        Qgis::ShaderInterpolationMethod::Linear);
    _pColorRamp->setColorRampItemList(_vItems);

    QgsRasterShader* _pShader = new QgsRasterShader(
        _dMinimumValue,
        _dMaximumValue);
    _pShader->setRasterShaderFunction(_pColorRamp);

    QgsSingleBandPseudoColorRenderer* _pRenderer =
        new QgsSingleBandPseudoColorRenderer(
            _pRasterLayer->dataProvider(),
            _nBand,
            _pShader);
    _pRenderer->setClassificationMin(_dMinimumValue);
    _pRenderer->setClassificationMax(_dMaximumValue);

    _pRasterLayer->setRenderer(_pRenderer);
    _pRasterLayer->triggerRepaint();
    mpCanvas->refresh();
}

void MapCanvasWidget::applyHighlightStyle(QgsVectorLayer* _pLayerInput)
{
    if (_pLayerInput == nullptr) {
        return;
    }

    QgsSymbol* _pSymbol = QgsSymbol::defaultSymbol(_pLayerInput->geometryType());
    if (_pSymbol == nullptr) {
        return;
    }

    _pSymbol->setColor(QColor(255, 77, 79, 190));
    _pSymbol->setOpacity(0.85);

    QgsLineSymbol* _pLineSymbol = dynamic_cast<QgsLineSymbol*>(_pSymbol);
    if (_pLineSymbol != nullptr) {
        _pLineSymbol->setWidth(1.2);
    }

    QgsMarkerSymbol* _pMarkerSymbol = dynamic_cast<QgsMarkerSymbol*>(_pSymbol);
    if (_pMarkerSymbol != nullptr) {
        _pMarkerSymbol->setSize(4.5);
    }

    _pLayerInput->setRenderer(new QgsSingleSymbolRenderer(_pSymbol));
}

QgsMapLayer* MapCanvasWidget::findLayerById(const QString& _strLayerId) const
{
    for (QgsMapLayer* _pLayer : mpvLayers) {
        if (_pLayer != nullptr && _pLayer->id() == _strLayerId) {
            return _pLayer;
        }
    }
    return QgsProject::instance()->mapLayer(_strLayerId);
}

void MapCanvasWidget::applyCommonSymbolStyle(QgsSymbol* _pSymbol,
    const QColor& _colorSymbol,
    double _dSizeValue,
    double _dOpacity) const
{
    if (_pSymbol == nullptr) {
        return;
    }

    _pSymbol->setColor(_colorSymbol);
    _pSymbol->setOpacity(qBound(0.0, _dOpacity, 1.0));

    QgsLineSymbol* _pLineSymbol = dynamic_cast<QgsLineSymbol*>(_pSymbol);
    if (_pLineSymbol != nullptr) {
        _pLineSymbol->setWidth(qMax(0.1, _dSizeValue));
    }

    QgsMarkerSymbol* _pMarkerSymbol = dynamic_cast<QgsMarkerSymbol*>(_pSymbol);
    if (_pMarkerSymbol != nullptr) {
        _pMarkerSymbol->setSize(qMax(0.5, _dSizeValue));
    }

}

bool MapCanvasWidget::isNumericField(QgsVectorLayer* _pLayerInput,
    const QString& _strFieldName) const
{
    if (_pLayerInput == nullptr) {
        return false;
    }

    const int _nFieldIdx = _pLayerInput->fields().indexOf(_strFieldName);
    if (_nFieldIdx < 0) {
        return false;
    }

    return isNumericVariantType(_pLayerInput->fields().at(_nFieldIdx).type());
}

QgsRectangle MapCanvasWidget::layerExtentInCanvasCrs(QgsMapLayer* _pLayerInput) const
{
    if (_pLayerInput == nullptr || mpCanvas == nullptr) {
        return QgsRectangle();
    }

    const QgsRectangle _rectLayerExtent = _pLayerInput->extent();
    if (_rectLayerExtent.isEmpty()) {
        return _rectLayerExtent;
    }

    const QgsCoordinateReferenceSystem _layerCrs = _pLayerInput->crs();
    const QgsCoordinateReferenceSystem _canvasCrs =
        mpCanvas->mapSettings().destinationCrs();
    if (!_layerCrs.isValid()
        || !_canvasCrs.isValid()
        || _layerCrs == _canvasCrs) {
        return _rectLayerExtent;
    }

    try {
        QgsCoordinateTransform _transform(_layerCrs, _canvasCrs,
            QgsProject::instance());
        return _transform.transformBoundingBox(_rectLayerExtent);
    } catch (const QgsCsException& _exception) {
        qWarning() << "[MapCanvasWidget] Failed to transform layer extent:"
                   << _pLayerInput->name()
                   << _exception.what();
        return _rectLayerExtent;
    }
}

// ════════════════════════════════════════════════════════
//  refreshCanvasLayers
//  将 mpvLayers 转换为 QgsMapCanvas 所需的 QList<QgsMapLayer*> 并刷新
// ════════════════════════════════════════════════════════
void MapCanvasWidget::refreshCanvasLayers()
{
    mpCanvas->setLayers(mpvLayers);
}

// ════════════════════════════════════════════════════════
//  onXYCoordinates
//  QGIS 画布坐标信号 → 转换为 WGS84 后发出 coordChanged
// ════════════════════════════════════════════════════════
void MapCanvasWidget::onXYCoordinates(const QgsPointXY& _point)
{
    // 若画布 CRS 非 WGS84，进行坐标转换
    const QgsCoordinateReferenceSystem _canvasCrs = mpCanvas->mapSettings().destinationCrs();
    const QgsCoordinateReferenceSystem _wgs84Crs("EPSG:4326");

    double _dLon = _point.x();
    double _dLat = _point.y();

    if (_canvasCrs != _wgs84Crs)
    {
        QgsCoordinateTransform _transform(_canvasCrs, _wgs84Crs,
            QgsProject::instance());
        const QgsPointXY _wgs84Point = _transform.transform(_point);
        _dLon = _wgs84Point.x();
        _dLat = _wgs84Point.y();
    }

    emit coordChanged(_dLon, _dLat);
}

// ════════════════════════════════════════════════════════
//  onScaleChanged
// ════════════════════════════════════════════════════════
void MapCanvasWidget::onScaleChanged(double _dScale)
{
    emit scaleChanged(_dScale);
}
