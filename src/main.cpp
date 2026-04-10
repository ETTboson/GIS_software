#include <qgsapplication.h>
#include "core/integration/qgsappinitializer.h"
#include "ui/mainwindow.h"

int main(int argc, char* argv[])
{
    // 使用 QgsApplication 替代 QApplication
    // 第三个参数 true 表示启用 GUI 模式
    QgsApplication app(argc, argv, true);

    // RAII：构造时初始化 QGIS，析构时自动 exitQgis()
    QgsAppInitializer _qgsInitializer;

    MainWindow w;
    w.show();

    return app.exec();
}
