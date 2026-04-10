// src/model/legacy/analysismodule.h
#ifndef ANALYSISMODULE_H_C3D4E5F6A1B2
#define ANALYSISMODULE_H_C3D4E5F6A1B2

#include <QObject>
#include <QString>

// ════════════════════════════════════════════════════════
//  AnalysisModule
//  职责：空间分析算法调度
//  接口参数由 AI 从对话中提取后传入
// ════════════════════════════════════════════════════════
class AnalysisModule : public QObject
{
    Q_OBJECT

public:
    explicit AnalysisModule(QObject *_pParent = nullptr);
    ~AnalysisModule();

    // 分析接口（参数由 AI 提取传入）
    void runBufferAnalysis(double _dRadiusMeters);
    void runOverlayAnalysis(const QString &_strOverlayType);
    void runSpatialQuery(const QString &_strExpression);
    void runRasterCalc(const QString &_strExpression);

signals:
    void analysisProgress(int _nPercent);
    void analysisFinished(const QString &_strResultDesc);
    void analysisFailed(const QString &_strErrorMsg);

public slots:
    void onDataReady(const QString &_strFilePath);

private:
    QString mstrReadyDataPath;
};

#endif // ANALYSISMODULE_H_C3D4E5F6A1B2
