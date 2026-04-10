#ifndef MAPCANVASMANAGER_H_A1B2C3D4E5F6
#define MAPCANVASMANAGER_H_A1B2C3D4E5F6

#include <QObject>
#include <QList>

class MapCanvasWidget;

// ════════════════════════════════════════════════════════
//  MapCanvasManager
//  职责：统一管理多个 MapCanvasWidget 实例的生命周期与激活状态。
//        MainWindow 所有画布操作（切换工具、加载图层等）均通过
//        activeCanvas() 获取当前激活画布后转发，不直接持有画布。
//  位于 ui/map/ 层，继承 QObject 以支持信号槽机制。
// ════════════════════════════════════════════════════════
class MapCanvasManager : public QObject
{
    Q_OBJECT

public:
    /*
     * @brief 构造函数，初始化空的画布列表，激活索引置 -1
     * @param_1 _pParent: 父对象指针
     */
    explicit MapCanvasManager(QObject* _pParent = nullptr);

    /*
     * @brief 析构函数，不负责 delete 各画布（画布由各自父控件管理）
     */
    ~MapCanvasManager() override;

    /*
     * @brief 返回当前激活的画布指针，若无画布则返回 nullptr
     */
    MapCanvasWidget* activeCanvas() const;

    /*
     * @brief 创建一个新的 MapCanvasWidget，注册到管理器，
     *        若当前无激活画布则自动将其设为激活
     * @param_1 _pParent: 新画布的父控件（通常为 MainWindow 的中央区域）
     */
    MapCanvasWidget* createCanvas(QWidget* _pParent);

    /*
     * @brief 从管理器中注销并销毁指定索引的画布
     * @param_1 _nIdx: 要移除的画布索引（0-based）
     */
    void removeCanvas(int _nIdx);

    /*
     * @brief 将指定索引的画布设为激活画布，并发出 activeCanvasChanged 信号
     * @param_1 _nIdx: 目标画布索引（0-based）
     */
    void setActiveCanvas(int _nIdx);

    /*
     * @brief 返回当前激活画布的索引，若无画布则返回 -1
     */
    int activeIndex() const;

    /*
     * @brief 返回当前管理器持有的画布总数
     */
    int canvasCount() const;

    /*
     * @brief 返回指定索引的画布指针，越界则返回 nullptr
     * @param_1 _nIdx: 目标画布索引（0-based）
     */
    MapCanvasWidget* canvasAt(int _nIdx) const;

signals:
    /*
     * @brief 激活画布切换后发出
     * @param _pCanvas: 新的激活画布指针
     */
    void activeCanvasChanged(MapCanvasWidget* _pCanvas);

    /*
     * @brief 新画布被添加后发出
     * @param _pCanvas: 新建的画布指针
     */
    void canvasAdded(MapCanvasWidget* _pCanvas);

    /*
     * @brief 画布被移除后发出
     * @param _nIdx: 被移除画布的原索引
     */
    void canvasRemoved(int _nIdx);

private:
    QList<MapCanvasWidget*> mpvCanvases; // 所有已注册的画布列表
    int                     mnActiveIdx; // 当前激活画布的索引，-1 表示无画布
};

#endif // MAPCANVASMANAGER_H_A1B2C3D4E5F6
