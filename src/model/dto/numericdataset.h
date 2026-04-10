// src/model/dto/numericdataset.h
#ifndef NUMERICDATASET_H_D4E5F6A1B2C3
#define NUMERICDATASET_H_D4E5F6A1B2C3

#include <QString>
#include <QVector>
#include <QMetaType>

// ════════════════════════════════════════════════════════
//  NumericDataset
//  数值型数据集 DTO
//  纯数据结构，不含业务逻辑
//  在 DataService、AnalysisService、MainWindow 之间传递
// ════════════════════════════════════════════════════════
struct NumericDataset
{
    QString        strSourcePath;   // 原始文件路径
    QString        strName;         // 数据集显示名称
    QString        strFormat;       // 数据格式，如 "csv" / "simple_raster"
    int            nRows;           // 行数
    int            nCols;           // 列数
    QVector<double> vdValues;       // 按行优先展开的数值列表
};

Q_DECLARE_METATYPE(NumericDataset)

#endif // NUMERICDATASET_H_D4E5F6A1B2C3
