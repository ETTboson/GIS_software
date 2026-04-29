#ifndef ANALYSISDATAASSET_H_8394A5B6C7D8
#define ANALYSISDATAASSET_H_8394A5B6C7D8

#include <QString>
#include <QMetaType>

#include "model/dto/metadatadocument.h"
#include "model/dto/numericdataset.h"
#include "model/dto/spatialrasterdata.h"
#include "model/dto/spatialvectordata.h"
#include "model/dto/tabulardata.h"
#include "model/enums/analysiscapability.h"
#include "model/enums/dataassettype.h"

// ════════════════════════════════════════════════════════
//  AnalysisDataAsset
//  统一分析数据资产 DTO
//  由数据解析层产出，经由 DataRepository 在工作区、宿主桥接层与 AI 模块之间共享，
//  统一描述数据类型、能力集合、数值输入与特定载荷。
// ════════════════════════════════════════════════════════
struct AnalysisDataAsset
{
    QString              strAssetId;         // 资产唯一标识，由仓库层生成并复用
    QString              strName;            // 资产显示名称
    QString              strSourcePath;      // 原始数据路径
    QString              strSourceFormat;    // 源格式标识，如 csv / xml / tif
    DataAssetType        eAssetType = DataAssetType::None; // 当前资产类型
    AnalysisCapabilities flagsCapabilities;  // 当前资产支持的分析能力集合
    QString              strSummary;         // 面向用户的数据摘要文本
    bool                 bCanAddToMap = false; // 当前资产是否可作为图层加入地图
    bool                 bHasNumericDataset = false; // 是否已准备好统一数值视图
    bool                 bNeedsUserChoice = false; // 当前导入是否需要 UI 二次确认
    QString              strChoicePrompt;    // 二次确认提示文本
    NumericDataset       dataNumeric;        // 跨能力共享的数值数据视图
    SpatialRasterData    dataRaster;         // 栅格资产载荷
    SpatialVectorData    dataVector;         // 矢量资产载荷
    TabularData          dataTable;          // 表格资产载荷
    MetadataDocument     dataMetadata;       // 元数据文档载荷
};

Q_DECLARE_METATYPE(AnalysisDataAsset)

#endif // ANALYSISDATAASSET_H_8394A5B6C7D8
