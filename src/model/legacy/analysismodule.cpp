#include "analysismodule.h"
#include <QDebug>

AnalysisModule::AnalysisModule(QObject *_pParent)
    : QObject(_pParent)
    , mstrReadyDataPath(QString())
{}

AnalysisModule::~AnalysisModule() {}

void AnalysisModule::onDataReady(const QString &_strFilePath)
{
    mstrReadyDataPath = _strFilePath;
    qDebug() << "[AnalysisModule] 数据就绪:" << _strFilePath;
}

void AnalysisModule::runBufferAnalysis(double _dRadiusMeters)
{
    emit analysisProgress(0);
    // TODO: 接入 GEOS/OGR
    qDebug() << "[AnalysisModule] runBufferAnalysis radius=" << _dRadiusMeters;
    emit analysisProgress(100);
    emit analysisFinished(tr("缓冲区分析完成（半径 %1 米）").arg(_dRadiusMeters));
}

void AnalysisModule::runOverlayAnalysis(const QString &_strOverlayType)
{
    emit analysisProgress(0);
    qDebug() << "[AnalysisModule] runOverlayAnalysis type=" << _strOverlayType;
    emit analysisProgress(100);
    emit analysisFinished(tr("叠加分析完成（%1）").arg(_strOverlayType));
}

void AnalysisModule::runSpatialQuery(const QString &_strExpression)
{
    emit analysisProgress(0);
    qDebug() << "[AnalysisModule] runSpatialQuery expr=" << _strExpression;
    emit analysisProgress(100);
    emit analysisFinished(tr("空间查询完成：%1").arg(_strExpression));
}

void AnalysisModule::runRasterCalc(const QString &_strExpression)
{
    emit analysisProgress(0);
    qDebug() << "[AnalysisModule] runRasterCalc expr=" << _strExpression;
    emit analysisProgress(100);
    emit analysisFinished(tr("栅格计算完成：%1").arg(_strExpression));
}
