// src/service/numericdatareader.h
#ifndef NUMERICDATAREADER_H_A4B5C6D7E8F9
#define NUMERICDATAREADER_H_A4B5C6D7E8F9

#include <QString>

#include "model/numericdataset.h"

class NumericDataReader
{
public:
    /*
     * @brief 读取 CSV 数值表格
     * @param_1 _strFilePath: 数据文件路径
     * @param_2 _outDataSet: 成功时写出的数据集 DTO
     * @param_3 _strError: 失败时写出的错误信息
     */
    static bool readCsvFile(const QString& _strFilePath,
        NumericDataset& _outDataSet,
        QString& _strError);

    /*
     * @brief 读取简单栅格文本
     *        支持 ESRI ASCII Grid 头部或普通空白分隔矩阵
     * @param_1 _strFilePath: 数据文件路径
     * @param_2 _outDataSet: 成功时写出的数据集 DTO
     * @param_3 _strError: 失败时写出的错误信息
     */
    static bool readSimpleRasterFile(const QString& _strFilePath,
        NumericDataset& _outDataSet,
        QString& _strError);

private:
    /*
     * @brief 尝试把文本 token 解析为 double
     */
    static bool tryParseDouble(const QString& _strToken, double& _dValue);

    /*
     * @brief 解析一行 CSV，兼容逗号与分号分隔
     */
    static QStringList splitCsvLine(const QString& _strLine);

    /*
     * @brief 将二维数值矩阵写入 NumericDataset
     */
    static void fillDataSet(const QString& _strFilePath,
        const QString& _strFormat,
        const QVector<QVector<double>>& _vvRows,
        NumericDataset& _outDataSet);
};

#endif // NUMERICDATAREADER_H_A4B5C6D7E8F9
