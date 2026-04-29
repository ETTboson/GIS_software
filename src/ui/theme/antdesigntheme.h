#ifndef ANTDESIGNTHEME_H_B7C8D9E0F1A2
#define ANTDESIGNTHEME_H_B7C8D9E0F1A2

class QApplication;
class QFont;
class QPalette;
class QString;

// ============================================================================
//  AntDesignTheme
//  职责：安装应用级浅色 Ant Design 风格主题。
//        仅负责 UI 字体、调色板与全局 QSS，不承载业务逻辑。
// ============================================================================
class AntDesignTheme
{
public:
    /*
     * @brief 安装应用级字体、调色板与浅色主题样式
     * @param_1 _pApp: Qt 应用对象指针
     */
    static void Install(QApplication* _pApp);

private:
    /*
     * @brief 构建应用字体
     */
    static QFont BuildFont();

    /*
     * @brief 构建浅色主题调色板
     */
    static QPalette BuildPalette();

    /*
     * @brief 构建应用级 QSS
     */
    static QString BuildAppStyleSheet();
};

#endif // ANTDESIGNTHEME_H_B7C8D9E0F1A2
