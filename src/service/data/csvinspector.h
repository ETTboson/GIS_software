#ifndef CSVINSPECTOR_H_A8B9C0D1E2F3
#define CSVINSPECTOR_H_A8B9C0D1E2F3

#include <QString>
#include <QStringList>

#include "model/dto/analysisdataasset.h"

// ════════════════════════════════════════════════════════
//  CsvInspector
//  职责：识别 CSV 文件的表格结构、数值列与坐标列，
//        统一产出 TabularData 资产，并在需要时支持转换为点矢量资产。
//  位于 service/data/ 层。
// ════════════════════════════════════════════════════════
class CsvInspector
{
public:
    /*
     * @brief 检查 CSV 文件并生成表格资产
     * @param_1 _strFilePath: CSV 文件绝对路径
     * @param_2 _outAsset: 成功时写出的分析资产 DTO
     * @param_3 _strError: 失败时写出的错误信息
     */
    static bool inspect(const QString& _strFilePath,
        AnalysisDataAsset& _outAsset,
        QString& _strError);

    /*
     * @brief 将带坐标列的表格资产转换为点矢量资产
     * @param_1 _assetTable: 已识别完成的表格资产
     * @param_2 _outAsset: 成功时写出的点矢量资产 DTO
     * @param_3 _strError: 失败时写出的错误信息
     */
    static bool buildPointAsset(const AnalysisDataAsset& _assetTable,
        AnalysisDataAsset& _outAsset,
        QString& _strError);

private:
    /*
     * @brief 按项目当前约定切分 CSV 行，兼容逗号与分号分隔
     * @param_1 _strLine: 单行原始文本
     */
    static QStringList splitCsvLine(const QString& _strLine);

    /*
     * @brief 判断一组 token 是否全部可解析为 double
     * @param_1 _vTokens: 当前行 token 列表
     */
    static bool isNumericRow(const QStringList& _vTokens);

    /*
     * @brief 规范化表头名称，便于识别 x/y、lon/lat 等坐标列
     * @param_1 _strHeader: 原始表头文本
     */
    static QString normalizeHeader(const QString& _strHeader);
};

#endif // CSVINSPECTOR_H_A8B9C0D1E2F3
