#ifndef QGSAPPINITIALIZER_H_A1B2C3D4E5F6
#define QGSAPPINITIALIZER_H_A1B2C3D4E5F6

// ════════════════════════════════════════════════════════
//  QgsAppInitializer
//  职责：以 RAII 方式管理 QGIS 应用程序的生命周期
//        构造时完成 QgsApplication 的初始化序列，
//        析构时安全退出 QGIS，确保资源释放顺序正确。
//  位于 core/integration/ 层，与业务逻辑无关。
//  使用方式：在 main() 中 QgsApplication 构造之后立即创建，
//            并在 app.exec() 之前检查 isValid()。
// ════════════════════════════════════════════════════════
class QgsAppInitializer
{
public:
    /*
     * @brief 初始化 QGIS 运行时环境
     *        依次执行：setPrefixPath → init → initQgis
     *        若任一步骤失败，mbValid 置为 false
     */
    QgsAppInitializer();

    /*
     * @brief 析构函数，调用 QgsApplication::exitQgis() 释放所有 QGIS 资源
     */
    ~QgsAppInitializer();

    /*
     * @brief 返回 QGIS 是否初始化成功
     */
    bool isValid() const;

    // 禁止拷贝与移动，保证全局唯一
    QgsAppInitializer(const QgsAppInitializer&)            = delete;
    QgsAppInitializer& operator=(const QgsAppInitializer&) = delete;
    QgsAppInitializer(QgsAppInitializer&&)                 = delete;
    QgsAppInitializer& operator=(QgsAppInitializer&&)      = delete;

private:
    bool mbValid; // QGIS 初始化是否成功
};

#endif // QGSAPPINITIALIZER_H_A1B2C3D4E5F6
