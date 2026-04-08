#ifndef MAPTOOLTYPE_H_A1B2C3D4E5F6
#define MAPTOOLTYPE_H_A1B2C3D4E5F6

// ════════════════════════════════════════════════════════
//  MapToolType
//  地图交互工具类型枚举，供 MapCanvasWidget 与 MainWindow 共享
//  位于 model/ 层，不含任何业务逻辑与 UI 依赖
// ════════════════════════════════════════════════════════
enum class MapToolType
{
    Pan,      // 平移工具
    ZoomIn,   // 放大工具
    ZoomOut   // 缩小工具
};

#endif // MAPTOOLTYPE_H_A1B2C3D4E5F6
