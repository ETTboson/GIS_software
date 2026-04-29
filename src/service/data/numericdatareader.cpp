// src/service/numericdatareader.cpp
#include "numericdatareader.h"

#include <gdal_priv.h>

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

bool NumericDataReader::readCsvFile(const QString& _strFilePath,
    NumericDataset& _outDataSet, QString& _strError)
{
    QFile _file(_strFilePath);
    if (!_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        _strError = QString::fromUtf8("无法打开 CSV 文件：%1").arg(_strFilePath);
        return false;
    }

    QTextStream _stream(&_file);
    _stream.setCodec("UTF-8");

    QVector<QVector<double>> _vvRows;
    bool _bHeaderSkipped = false;
    int  _nExpectedCols = -1;

    while (!_stream.atEnd()) {
        const QString _strLine = _stream.readLine().trimmed();
        if (_strLine.isEmpty()) {
            continue;
        }

        const QStringList _vTokens = splitCsvLine(_strLine);
        if (_vTokens.isEmpty()) {
            continue;
        }

        QVector<double> _vdRow;
        bool _bAllNumeric = true;

        for (const QString& _strRawToken : _vTokens) {
            double _dValue = 0.0;
            if (!tryParseDouble(_strRawToken.trimmed(), _dValue)) {
                _bAllNumeric = false;
                break;
            }
            _vdRow.append(_dValue);
        }

        if (!_bAllNumeric) {
            if (!_bHeaderSkipped && _vvRows.isEmpty()) {
                _bHeaderSkipped = true;
                continue;
            }

            _strError = QString::fromUtf8("CSV 中存在非数值内容：%1").arg(_strLine);
            return false;
        }

        if (_nExpectedCols < 0) {
            _nExpectedCols = _vdRow.size();
        } else if (_vdRow.size() != _nExpectedCols) {
            _strError = QString::fromUtf8("CSV 列数不一致，无法构成规则矩阵");
            return false;
        }

        _vvRows.append(_vdRow);
    }

    if (_vvRows.isEmpty()) {
        _strError = QString::fromUtf8("CSV 中未找到可分析的数值数据");
        return false;
    }

    fillDataSet(_strFilePath, "csv", _vvRows, _outDataSet);
    return true;
}

bool NumericDataReader::readSimpleRasterFile(const QString& _strFilePath,
    NumericDataset& _outDataSet, QString& _strError)
{
    QFile _file(_strFilePath);
    if (!_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        _strError = QString::fromUtf8("无法打开栅格文本文件：%1").arg(_strFilePath);
        return false;
    }

    QTextStream _stream(&_file);
    _stream.setCodec("UTF-8");

    QVector<QVector<double>> _vvRows;
    int _nExpectedCols = -1;

    while (!_stream.atEnd()) {
        QString _strLine = _stream.readLine().trimmed();
        if (_strLine.isEmpty()) {
            continue;
        }

        const QString _strLowerLine = _strLine.toLower();
        if (_strLowerLine.startsWith("ncols")
            || _strLowerLine.startsWith("nrows")
            || _strLowerLine.startsWith("xllcorner")
            || _strLowerLine.startsWith("yllcorner")
            || _strLowerLine.startsWith("xllcenter")
            || _strLowerLine.startsWith("yllcenter")
            || _strLowerLine.startsWith("cellsize")
            || _strLowerLine.startsWith("nodata_value")) {
            continue;
        }

        _strLine.replace(',', ' ');
        const QStringList _vTokens = _strLine.split(
            QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (_vTokens.isEmpty()) {
            continue;
        }

        QVector<double> _vdRow;
        for (const QString& _strToken : _vTokens) {
            double _dValue = 0.0;
            if (!tryParseDouble(_strToken, _dValue)) {
                _strError = QString::fromUtf8("栅格文本中存在非数值内容：%1").arg(_strToken);
                return false;
            }
            _vdRow.append(_dValue);
        }

        if (_nExpectedCols < 0) {
            _nExpectedCols = _vdRow.size();
        } else if (_vdRow.size() != _nExpectedCols) {
            _strError = QString::fromUtf8("栅格文本列数不一致，无法构成规则矩阵");
            return false;
        }

        _vvRows.append(_vdRow);
    }

    if (_vvRows.isEmpty()) {
        _strError = QString::fromUtf8("栅格文本中未找到可分析的数值数据");
        return false;
    }

    fillDataSet(_strFilePath, "simple_raster", _vvRows, _outDataSet);
    return true;
}

bool NumericDataReader::readRasterFile(const QString& _strFilePath,
    NumericDataset& _outDataSet,
    int* _pnBandCount,
    bool* _pbHasNoData,
    double* _pdNoDataValue,
    QString& _strError)
{
    GDALAllRegister();

    GDALDataset* _poDataset = static_cast<GDALDataset*>(
        GDALOpen(_strFilePath.toUtf8().constData(), GA_ReadOnly));
    if (_poDataset == nullptr) {
        _strError = QString::fromUtf8("无法打开栅格文件：%1").arg(_strFilePath);
        return false;
    }

    const int _nRasterXSize = _poDataset->GetRasterXSize();
    const int _nRasterYSize = _poDataset->GetRasterYSize();
    const int _nBandCount = _poDataset->GetRasterCount();
    if (_pnBandCount != nullptr) {
        *_pnBandCount = _nBandCount;
    }

    if (_nRasterXSize <= 0 || _nRasterYSize <= 0 || _nBandCount <= 0) {
        _strError = QString::fromUtf8("栅格文件缺少有效波段或尺寸信息：%1")
            .arg(_strFilePath);
        GDALClose(_poDataset);
        return false;
    }

    GDALRasterBand* _poBand = _poDataset->GetRasterBand(1);
    if (_poBand == nullptr) {
        _strError = QString::fromUtf8("无法读取第一波段：%1").arg(_strFilePath);
        GDALClose(_poDataset);
        return false;
    }

    int _nHasNoData = FALSE;
    const double _dNoDataValue = _poBand->GetNoDataValue(&_nHasNoData);
    if (_pbHasNoData != nullptr) {
        *_pbHasNoData = (_nHasNoData != FALSE);
    }
    if (_pdNoDataValue != nullptr) {
        *_pdNoDataValue = _dNoDataValue;
    }

    QVector<double> _vdValues;
    _vdValues.resize(_nRasterXSize * _nRasterYSize);
    CPLErr _eReadStatus = _poBand->RasterIO(
        GF_Read,
        0,
        0,
        _nRasterXSize,
        _nRasterYSize,
        _vdValues.data(),
        _nRasterXSize,
        _nRasterYSize,
        GDT_Float64,
        0,
        0);
    if (_eReadStatus != CE_None) {
        _strError = QString::fromUtf8("读取栅格像元失败：%1").arg(_strFilePath);
        GDALClose(_poDataset);
        return false;
    }

    _outDataSet = NumericDataset();
    _outDataSet.strSourcePath = _strFilePath;
    _outDataSet.strName = QFileInfo(_strFilePath).fileName();
    _outDataSet.strFormat = QFileInfo(_strFilePath).suffix().toLower();
    _outDataSet.nRows = _nRasterYSize;
    _outDataSet.nCols = _nRasterXSize;
    _outDataSet.vdValues = _vdValues;

    GDALClose(_poDataset);
    return true;
}

bool NumericDataReader::tryParseDouble(const QString& _strToken, double& _dValue)
{
    bool _bOk = false;
    _dValue = _strToken.toDouble(&_bOk);
    return _bOk;
}

QStringList NumericDataReader::splitCsvLine(const QString& _strLine)
{
    if (_strLine.contains(';') && !_strLine.contains(',')) {
        return _strLine.split(';', Qt::SkipEmptyParts);
    }

    return _strLine.split(',', Qt::KeepEmptyParts);
}

void NumericDataReader::fillDataSet(const QString& _strFilePath,
    const QString& _strFormat,
    const QVector<QVector<double>>& _vvRows,
    NumericDataset& _outDataSet)
{
    _outDataSet.strSourcePath = _strFilePath;
    _outDataSet.strName = QFileInfo(_strFilePath).fileName();
    _outDataSet.strFormat = _strFormat;
    _outDataSet.nRows = _vvRows.size();
    _outDataSet.nCols = _vvRows[0].size();
    _outDataSet.vdValues.clear();
    _outDataSet.vdValues.reserve(_outDataSet.nRows * _outDataSet.nCols);

    for (const QVector<double>& _vdRow : _vvRows) {
        for (double _dValue : _vdRow) {
            _outDataSet.vdValues.append(_dValue);
        }
    }
}
