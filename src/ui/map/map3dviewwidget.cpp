#include "map3dviewwidget.h"

#include <QColor>
#include <QDebug>
#include <QFileInfo>
#include <QList>
#include <QVBoxLayout>
#include <QtGlobal>

#include <qgs3dmapcanvas.h>
#include <qgs3dmapsettings.h>
#include <qgscoordinatetransform.h>
#include <qgsdemterraingenerator.h>
#include <qgsexception.h>
#include <qgsmaplayer.h>
#include <qgsproject.h>
#include <qgsrasterlayer.h>

Map3DViewWidget::Map3DViewWidget(QWidget* _pParent)
    : QWidget(_pParent)
    , mpCanvas3D(nullptr)
    , mpctrlContainer(nullptr)
    , mbSceneInitialized(false)
{
    QVBoxLayout* _pLayout = new QVBoxLayout(this);
    _pLayout->setContentsMargins(0, 0, 0, 0);
    _pLayout->setSpacing(0);
    setLayout(_pLayout);
}

Map3DViewWidget::~Map3DViewWidget()
{
    delete mpctrlContainer;
    mpctrlContainer = nullptr;
    mpCanvas3D = nullptr;
    removeRegisteredLayer();
}

bool Map3DViewWidget::initializeFromDem(const QString& _strDemPath)
{
    if (mbSceneInitialized) {
        qWarning() << "[Map3DViewWidget] Scene has already been initialized.";
        return false;
    }

    const QFileInfo _fiDem(_strDemPath);
    if (!_fiDem.exists() || !_fiDem.isFile()) {
        qWarning() << "[Map3DViewWidget] DEM file does not exist:" << _strDemPath;
        return false;
    }

    QgsRasterLayer* _pDemLayer =
        createRasterLayer(_fiDem.absoluteFilePath(), _fiDem.completeBaseName());
    if (_pDemLayer == nullptr) {
        return false;
    }

    const QgsCoordinateReferenceSystem _crsScene =
        sceneCrsForLayer(_pDemLayer);
    mrectSceneExtent = normalizedSceneExtent(
        transformedLayerExtent(_pDemLayer, _crsScene));
    if (!_crsScene.isValid() || mrectSceneExtent.isEmpty()) {
        qWarning() << "[Map3DViewWidget] DEM has invalid 3D scene CRS or extent:"
                   << _strDemPath
                   << _crsScene.authid()
                   << mrectSceneExtent.toString();
        delete _pDemLayer;
        return false;
    }

    removeRegisteredLayer();
    registerProjectLayer(_pDemLayer);

    if (!createCanvasContainer()) {
        removeRegisteredLayer();
        return false;
    }

    QList<QgsMapLayer*> _vSceneLayers;
    _vSceneLayers << _pDemLayer;

    Qgs3DMapSettings* _pSettings = new Qgs3DMapSettings();
    _pSettings->setCrs(_crsScene);
    _pSettings->setTransformContext(QgsProject::instance()->transformContext());
    _pSettings->setPathResolver(QgsProject::instance()->pathResolver());
    _pSettings->setMapThemeCollection(QgsProject::instance()->mapThemeCollection());
    _pSettings->setBackgroundColor(QColor("#EAF2F8"));
    _pSettings->setSelectionColor(QColor("#FF4D4F"));
    _pSettings->setLayers(_vSceneLayers);
    _pSettings->setMapTileResolution(256);
    _pSettings->setMaxTerrainScreenError(3.0f);
    _pSettings->setTerrainVerticalScale(1.5f);
    _pSettings->setTerrainRenderingEnabled(true);
    _pSettings->setExtent(mrectSceneExtent);

    QgsDemTerrainGenerator* _pTerrainGenerator = new QgsDemTerrainGenerator();
    _pTerrainGenerator->setLayer(_pDemLayer);
    _pTerrainGenerator->setCrs(
        _crsScene,
        QgsProject::instance()->transformContext());
    _pTerrainGenerator->setResolution(32);
    _pSettings->setTerrainGenerator(_pTerrainGenerator);

    mpCanvas3D->resize(1100, 760);
    mpCanvas3D->setMapSettings(_pSettings);
    mbSceneInitialized = true;
    resetView();
    return true;
}

void Map3DViewWidget::resetView()
{
    if (mpCanvas3D == nullptr) {
        return;
    }

    if (mbSceneInitialized && !mrectSceneExtent.isEmpty()) {
        mpCanvas3D->setViewFrom2DExtent(mrectSceneExtent);
        return;
    }

    mpCanvas3D->resetView();
}

QgsRasterLayer* Map3DViewWidget::createRasterLayer(
    const QString& _strFilePath,
    const QString& _strLayerName) const
{
    QgsRasterLayer* _pLayer = new QgsRasterLayer(_strFilePath, _strLayerName);
    if (!_pLayer->isValid()) {
        qWarning() << "[Map3DViewWidget] Invalid DEM/raster layer:" << _strFilePath;
        delete _pLayer;
        return nullptr;
    }

    _pLayer->setDefaultContrastEnhancement();
    return _pLayer;
}

bool Map3DViewWidget::createCanvasContainer()
{
    if (mpCanvas3D != nullptr && mpctrlContainer != nullptr) {
        return true;
    }

    mpCanvas3D = new Qgs3DMapCanvas();
    mpCanvas3D->resize(1100, 760);

    mpctrlContainer = QWidget::createWindowContainer(mpCanvas3D, this);
    if (mpctrlContainer == nullptr) {
        // Qgs3DMapCanvas assumes setMapSettings() has succeeded before destruction.
        mpCanvas3D = nullptr;
        return false;
    }
    mpctrlContainer->setFocusPolicy(Qt::StrongFocus);

    if (layout() != nullptr) {
        layout()->addWidget(mpctrlContainer);
    }
    return true;
}

QgsCoordinateReferenceSystem Map3DViewWidget::sceneCrsForLayer(
    QgsMapLayer* _pLayerInput) const
{
    if (_pLayerInput == nullptr || !_pLayerInput->crs().isValid()) {
        return QgsCoordinateReferenceSystem(QStringLiteral("EPSG:3857"));
    }
    if (_pLayerInput->crs().isGeographic()) {
        return QgsCoordinateReferenceSystem(QStringLiteral("EPSG:3857"));
    }
    return _pLayerInput->crs();
}

QgsRectangle Map3DViewWidget::normalizedSceneExtent(
    const QgsRectangle& _rectInput) const
{
    if (_rectInput.isEmpty()) {
        return _rectInput;
    }

    QgsRectangle _rectNormalized = _rectInput;
    const double _dLargestSide =
        qMax(_rectNormalized.width(), _rectNormalized.height());
    const double _dGrowDistance = qMax(_dLargestSide * 0.05, 1.0);
    _rectNormalized.grow(_dGrowDistance);
    return _rectNormalized;
}

void Map3DViewWidget::registerProjectLayer(QgsMapLayer* _pLayerInput)
{
    if (_pLayerInput == nullptr) {
        return;
    }

    QgsProject::instance()->addMapLayer(_pLayerInput, false);
    mstrDemLayerId = _pLayerInput->id();
}

void Map3DViewWidget::removeRegisteredLayer()
{
    if (mstrDemLayerId.isEmpty()) {
        return;
    }

    QgsProject::instance()->removeMapLayer(mstrDemLayerId);
    mstrDemLayerId.clear();
    mbSceneInitialized = false;
    mrectSceneExtent = QgsRectangle();
}

QgsRectangle Map3DViewWidget::transformedLayerExtent(
    QgsMapLayer* _pLayerInput,
    const QgsCoordinateReferenceSystem& _crsTarget) const
{
    if (_pLayerInput == nullptr) {
        return QgsRectangle();
    }

    const QgsRectangle _rectLayerExtent = _pLayerInput->extent();
    if (_rectLayerExtent.isEmpty()) {
        return _rectLayerExtent;
    }

    if (!_pLayerInput->crs().isValid()
        || !_crsTarget.isValid()
        || _pLayerInput->crs() == _crsTarget) {
        return _rectLayerExtent;
    }

    try {
        QgsCoordinateTransform _transform(
            _pLayerInput->crs(),
            _crsTarget,
            QgsProject::instance());
        return _transform.transformBoundingBox(_rectLayerExtent);
    } catch (const QgsCsException& _exception) {
        qWarning() << "[Map3DViewWidget] Failed to transform extent:"
                   << _pLayerInput->name()
                   << _exception.what();
        return _rectLayerExtent;
    }
}
