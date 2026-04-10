#include "mapcanvasmanager.h"
#include "mapcanvaswidget.h"

#include <QDebug>

// ════════════════════════════════════════════════════════
//  构造 / 析构
// ════════════════════════════════════════════════════════
MapCanvasManager::MapCanvasManager(QObject* _pParent)
    : QObject(_pParent)
    , mnActiveIdx(-1)
{
}

MapCanvasManager::~MapCanvasManager()
{
    // 画布的内存由各自的父 QWidget 管理，此处仅清空列表
    mpvCanvases.clear();
}

// ════════════════════════════════════════════════════════
//  activeCanvas
// ════════════════════════════════════════════════════════
MapCanvasWidget* MapCanvasManager::activeCanvas() const
{
    if (mnActiveIdx < 0 || mnActiveIdx >= mpvCanvases.size())
    {
        return nullptr;
    }
    return mpvCanvases[mnActiveIdx];
}

// ════════════════════════════════════════════════════════
//  createCanvas
//  创建新画布，注册到列表，若是第一个则自动激活
// ════════════════════════════════════════════════════════
MapCanvasWidget* MapCanvasManager::createCanvas(QWidget* _pParent)
{
    MapCanvasWidget* _pCanvas = new MapCanvasWidget(_pParent);
    mpvCanvases.append(_pCanvas);

    const int _nNewIdx = mpvCanvases.size() - 1;
    emit canvasAdded(_pCanvas);

    // 第一个画布自动成为激活画布
    if (mnActiveIdx < 0)
    {
        mnActiveIdx = _nNewIdx;
        emit activeCanvasChanged(_pCanvas);
    }

    qDebug() << "[MapCanvasManager] Canvas created, total:" << mpvCanvases.size();
    return _pCanvas;
}

// ════════════════════════════════════════════════════════
//  removeCanvas
// ════════════════════════════════════════════════════════
void MapCanvasManager::removeCanvas(int _nIdx)
{
    if (_nIdx < 0 || _nIdx >= mpvCanvases.size())
    {
        qWarning() << "[MapCanvasManager] removeCanvas: index out of range:" << _nIdx;
        return;
    }

    MapCanvasWidget* _pCanvas = mpvCanvases[_nIdx];
    mpvCanvases.removeAt(_nIdx);

    // 调整激活索引
    if (mnActiveIdx == _nIdx)
    {
        // 被移除的是当前激活画布，尝试激活前一个
        mnActiveIdx = mpvCanvases.isEmpty() ? -1
            : qMax(0, _nIdx - 1);

        emit activeCanvasChanged(activeCanvas());
    }
    else if (mnActiveIdx > _nIdx)
    {
        // 移除的在激活画布前面，索引需要前移
        --mnActiveIdx;
    }

    emit canvasRemoved(_nIdx);

    // 画布由父控件负责释放，此处 delete 是为了在无父控件时也安全
    if (_pCanvas->parent() == nullptr)
    {
        delete _pCanvas;
    }

    qDebug() << "[MapCanvasManager] Canvas removed at" << _nIdx
             << ", total:" << mpvCanvases.size();
}

// ════════════════════════════════════════════════════════
//  setActiveCanvas
// ════════════════════════════════════════════════════════
void MapCanvasManager::setActiveCanvas(int _nIdx)
{
    if (_nIdx < 0 || _nIdx >= mpvCanvases.size())
    {
        qWarning() << "[MapCanvasManager] setActiveCanvas: index out of range:" << _nIdx;
        return;
    }

    if (_nIdx == mnActiveIdx)
    {
        return; // 已经是激活画布，无需操作
    }

    mnActiveIdx = _nIdx;
    emit activeCanvasChanged(mpvCanvases[mnActiveIdx]);

    qDebug() << "[MapCanvasManager] Active canvas set to index:" << mnActiveIdx;
}

// ════════════════════════════════════════════════════════
//  activeIndex / canvasCount / canvasAt
// ════════════════════════════════════════════════════════
int MapCanvasManager::activeIndex() const
{
    return mnActiveIdx;
}

int MapCanvasManager::canvasCount() const
{
    return mpvCanvases.size();
}

MapCanvasWidget* MapCanvasManager::canvasAt(int _nIdx) const
{
    if (_nIdx < 0 || _nIdx >= mpvCanvases.size())
    {
        qWarning() << "[MapCanvasManager] canvasAt: index out of range:" << _nIdx;
        return nullptr;
    }
    return mpvCanvases[_nIdx];
}
