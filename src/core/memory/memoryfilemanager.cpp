// src/core/memory/memoryfilemanager.cpp
#include "memoryfilemanager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

MemoryFileManager::MemoryFileManager(const QString& _strWorkDir)
    : mstrWorkDir(_strWorkDir.isEmpty() ? QDir::currentPath() : _strWorkDir)
{
}

void MemoryFileManager::setWorkDir(const QString& _strWorkDir)
{
    mstrWorkDir = _strWorkDir.isEmpty() ? QDir::currentPath() : _strWorkDir;
}

QString MemoryFileManager::primaryMemoryPath() const
{
    return QDir(mstrWorkDir).filePath("CLAUDE.md");
}

QString MemoryFileManager::memoryDirPath() const
{
    return QDir(mstrWorkDir).filePath("memory");
}

QStringList MemoryFileManager::discoverMemoryFiles() const
{
    QStringList _vResult;
    QDir _workDir(mstrWorkDir);

    const QString _strPrimaryPath = _workDir.filePath("CLAUDE.md");
    if (QFile::exists(_strPrimaryPath)) {
        _vResult << _strPrimaryPath;
    }

    QDir _memDir(memoryDirPath());
    if (_memDir.exists()) {
        const QStringList _vMdFiles = _memDir.entryList(QStringList() << "*.md",
            QDir::Files, QDir::Name);
        for (const QString& _strFileName : _vMdFiles) {
            _vResult << _memDir.filePath(_strFileName);
        }
    }

    QDir _parentDir = _workDir;
    for (int _nLevelIdx = 0; _nLevelIdx < 3; ++_nLevelIdx) {
        if (!_parentDir.cdUp()) {
            break;
        }

        const QString _strParentClaude = _parentDir.filePath("CLAUDE.md");
        if (QFile::exists(_strParentClaude) && !_vResult.contains(_strParentClaude)) {
            _vResult << _strParentClaude;
        }
    }

    return _vResult;
}

QString MemoryFileManager::loadWarm()
{
    const QStringList _vFiles = discoverMemoryFiles();
    if (_vFiles.isEmpty()) {
        return QString();
    }

    QStringList _vSections;
    for (const QString& _strFilePath : _vFiles) {
        const QString _strContent = readFile(_strFilePath).trimmed();
        if (_strContent.isEmpty()) {
            continue;
        }

        const QString _strRelPath = QDir(mstrWorkDir).relativeFilePath(_strFilePath);
        _vSections << QString("# %1\n%2").arg(_strRelPath, _strContent);
    }

    if (_vSections.isEmpty()) {
        return QString();
    }

    return QString("<memory>\n%1\n</memory>").arg(_vSections.join("\n\n---\n\n"));
}

bool MemoryFileManager::hasMemoryFiles() const
{
    return !discoverMemoryFiles().isEmpty();
}

QStringList MemoryFileManager::memoryFilePaths() const
{
    return discoverMemoryFiles();
}

QList<MemorySearchResult> MemoryFileManager::searchCold(const QString& _strKeyword,
    bool _bCaseSensitive, int _nMaxResults) const
{
    QList<MemorySearchResult> _vResults;
    if (_strKeyword.trimmed().isEmpty()) {
        return _vResults;
    }

    QString _strPattern = QRegularExpression::escape(_strKeyword);
    _strPattern.replace("\\*", ".*");
    _strPattern.replace("\\?", ".");
    const QRegularExpression _re(_strPattern,
        _bCaseSensitive ? QRegularExpression::NoPatternOption
                        : QRegularExpression::CaseInsensitiveOption);

    const QStringList _vFiles = discoverMemoryFiles();
    for (const QString& _strFilePath : _vFiles) {
        if (_vResults.size() >= _nMaxResults) {
            break;
        }

        QFile _file(_strFilePath);
        if (!_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QTextStream _stream(&_file);
        _stream.setCodec("UTF-8");
        QStringList _vAllLines;
        while (!_stream.atEnd()) {
            _vAllLines << _stream.readLine();
        }
        _file.close();

        for (int _nLineIdx = 0; _nLineIdx < _vAllLines.size(); ++_nLineIdx) {
            if (_vResults.size() >= _nMaxResults) {
                break;
            }

            const QString& _strLine = _vAllLines[_nLineIdx];
            if (!_re.match(_strLine).hasMatch()) {
                continue;
            }

            MemorySearchResult _result;
            _result.strFilePath = _strFilePath;
            _result.strFileName = QFileInfo(_strFilePath).fileName();
            _result.nLineNumber = _nLineIdx + 1;
            _result.strLineText = _strLine.trimmed();

            QStringList _vContext;
            if (_nLineIdx > 0) {
                _vContext << _vAllLines[_nLineIdx - 1].trimmed();
            }
            _vContext << QString(">>> %1").arg(_strLine.trimmed());
            if (_nLineIdx + 1 < _vAllLines.size()) {
                _vContext << _vAllLines[_nLineIdx + 1].trimmed();
            }
            _result.strContext = _vContext.join("\n");
            _vResults << _result;
        }
    }

    return _vResults;
}

QString MemoryFileManager::formatSearchResults(const QList<MemorySearchResult>& _vResults,
    const QString& _strKeyword)
{
    if (_vResults.isEmpty()) {
        return QString("在记忆文件中未找到与「%1」相关的内容。").arg(_strKeyword);
    }

    QStringList _vLines;
    _vLines << QString("在记忆文件中找到 %1 条与「%2」相关的内容：")
        .arg(_vResults.size()).arg(_strKeyword);

    QString _strLastFileName;
    for (const MemorySearchResult& _result : _vResults) {
        if (_result.strFileName != _strLastFileName) {
            _strLastFileName = _result.strFileName;
            _vLines << QString("\n[%1]").arg(_result.strFileName);
        }
        _vLines << QString("第 %1 行：%2").arg(_result.nLineNumber).arg(_result.strLineText);
    }

    return _vLines.join("\n");
}

bool MemoryFileManager::saveMemory(const QString& _strContent,
    const QString& _strSection, bool _bOverwrite)
{
    const QString _strPath = primaryMemoryPath();
    const QString _strExisting = readFile(_strPath);
    QString _strNewContent;

    if (_strSection.trimmed().isEmpty()) {
        _strNewContent = _strExisting;
        if (!_strNewContent.isEmpty() && !_strNewContent.endsWith('\n')) {
            _strNewContent += '\n';
        }
        _strNewContent += QString("\n<!-- 更新于 %1 -->\n%2")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"), _strContent);
    } else {
        const QString _strSectionHeader = QString("## %1").arg(_strSection);
        const int _nSectionStart = _strExisting.indexOf(_strSectionHeader);

        if (_nSectionStart < 0) {
            _strNewContent = _strExisting;
            if (!_strNewContent.isEmpty() && !_strNewContent.endsWith('\n')) {
                _strNewContent += '\n';
            }
            _strNewContent += QString("\n## %1\n\n<!-- 创建于 %2 -->\n%3")
                .arg(_strSection,
                    QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"),
                    _strContent);
        } else {
            const int _nNextSection = _strExisting.indexOf("\n## ",
                _nSectionStart + _strSectionHeader.length());
            const int _nSectionEnd = (_nNextSection < 0) ? _strExisting.length() : _nNextSection;
            const QString _strBefore = _strExisting.left(_nSectionStart);
            const QString _strAfter = _strExisting.mid(_nSectionEnd);

            if (_bOverwrite) {
                _strNewContent = _strBefore
                    + QString("%1\n<!-- 更新于 %2 -->\n%3")
                        .arg(_strSectionHeader,
                            QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"),
                            _strContent)
                    + _strAfter;
            } else {
                QString _strSectionBody = _strExisting.left(_nSectionEnd);
                if (!_strSectionBody.endsWith('\n')) {
                    _strSectionBody += '\n';
                }
                _strNewContent = _strSectionBody
                    + QString("\n<!-- 追加于 %1 -->\n%2")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"),
                            _strContent)
                    + _strAfter;
            }
        }
    }

    QFile _file(_strPath);
    if (!_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream _stream(&_file);
    _stream.setCodec("UTF-8");
    _stream << _strNewContent;
    _file.close();
    return true;
}

QString MemoryFileManager::readFile(const QString& _strPath)
{
    QFile _file(_strPath);
    if (!_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream _stream(&_file);
    _stream.setCodec("UTF-8");
    return _stream.readAll();
}
