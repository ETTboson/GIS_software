#include "mapcanvaswidget.h"

#include <QVBoxLayout>
#include <QDebug>

#include <qgsmapcanvas.h>
#include <qgsmaptoolpan.h>
#include <qgsmaptoolzoom.h>
#include <qgsvectorlayer.h>
#include <qgsrasterlayer.h>
#include <qgsproject.h>
#include <qgscoordinatereferencesystem.h>
#include <qgspointxy.h>
#include <qgslayertree.h>


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
    if (_strFilePath.isEmpty())
    {
        qWarning() << "[MapCanvasWidget] loadFromPath: empty path.";
        return;
    }

    QgsMapLayer* _pLayer = nullptr;
    const QString _strLower = _strFilePath.toLower();

    // ── 根据扩展名选择图层类型 ────────────────────────
    if (_strLower.endsWith(".shp") || _strLower.endsWith(".geojson"))
    {
        QgsVectorLayer* _pVecLayer = new QgsVectorLayer(
            _strFilePath,
            QFileInfo(_strFilePath).baseName(),
            QStringLiteral("ogr"));

        if (!_pVecLayer->isValid())
        {
            qWarning() << "[MapCanvasWidget] Failed to load vector layer:"
                       << _strFilePath;
            delete _pVecLayer;
            return;
        }
        _pLayer = _pVecLayer;
    }
    else if (_strLower.endsWith(".tif")  ||
             _strLower.endsWith(".tiff") ||
             _strLower.endsWith(".img"))
    {
        QgsRasterLayer* _pRasLayer = new QgsRasterLayer(
            _strFilePath,
            QFileInfo(_strFilePath).baseName());

        if (!_pRasLayer->isValid())
        {
            qWarning() << "[MapCanvasWidget] Failed to load raster layer:"
                       << _strFilePath;
            delete _pRasLayer;
            return;
        }
        _pLayer = _pRasLayer;
    }
    else
    {
        qWarning() << "[MapCanvasWidget] Unsupported file type:" << _strFilePath;
        return;
    }

    // ── 注册到 QgsProject（统一管理生命周期）─────────
    QgsProject::instance()->addMapLayer(_pLayer, false);

    // ── 加入本画布图层列表（新图层放在最上层）────────
    mpvLayers.prepend(_pLayer);
    refreshCanvasLayers();

    // 首次加载时缩放至该图层范围
    if (mpvLayers.size() == 1)
    {
        mpCanvas->setExtent(_pLayer->extent());
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
