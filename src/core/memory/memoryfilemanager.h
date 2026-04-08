// src/core/memory/memoryfilemanager.h
#ifndef MEMORYFILEMANAGER_H_B1C2D3E4F5A6
#define MEMORYFILEMANAGER_H_B1C2D3E4F5A6

#include <QString>
#include <QStringList>
#include <QList>

struct MemorySearchResult
{
    QString strFilePath;    // 来源文件完整路径
    QString strFileName;    // 来源文件名
    int     nLineNumber;    // 行号，从 1 开始
    QString strLineText;    // 命中行文本
    QString strContext;     // 简单上下文
};

class MemoryFileManager
{
public:
    /*
     * @brief 构造函数，指定记忆文件工作目录
     * @param_1 _strWorkDir: CLAUDE.md 与 memory/ 的扫描根目录
     */
    explicit MemoryFileManager(const QString& _strWorkDir = QString());

    /*
     * @brief 设置工作目录
     * @param_1 _strWorkDir: 新的扫描根目录
     */
    void setWorkDir(const QString& _strWorkDir);

    /*
     * @brief 加载温数据记忆文本
     */
    QString loadWarm();

    /*
     * @brief 是否存在任意记忆文件
     */
    bool hasMemoryFiles() const;

    /*
     * @brief 返回所有记忆文件路径
     */
    QStringList memoryFilePaths() const;

    /*
     * @brief 搜索冷数据记忆
     * @param_1 _strKeyword: 搜索关键词，支持简单通配符
     * @param_2 _bCaseSensitive: 是否大小写敏感
     * @param_3 _nMaxResults: 最大返回数
     */
    QList<MemorySearchResult> searchCold(const QString& _strKeyword,
        bool _bCaseSensitive = false,
        int _nMaxResults = 20) const;

    /*
     * @brief 格式化搜索结果为可回传模型的文本
     */
    static QString formatSearchResults(const QList<MemorySearchResult>& _vResults,
        const QString& _strKeyword);

    /*
     * @brief 写入或追加记忆到 CLAUDE.md
     */
    bool saveMemory(const QString& _strContent,
        const QString& _strSection = QString(),
        bool _bOverwrite = false);

private:
    /*
     * @brief 扫描记忆文件列表
     */
    QStringList discoverMemoryFiles() const;

    /*
     * @brief 获取主记忆文件完整路径
     */
    QString primaryMemoryPath() const;

    /*
     * @brief 获取 memory 目录完整路径
     */
    QString memoryDirPath() const;

    /*
     * @brief 读取文本文件内容
     */
    static QString readFile(const QString& _strPath);

    QString mstrWorkDir;
};

#endif // MEMORYFILEMANAGER_H_B1C2D3E4F5A6
