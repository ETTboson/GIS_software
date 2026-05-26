#include "datarepository.h"

#include <QUuid>

DataRepository::DataRepository(QObject* _pParent)
    : QObject(_pParent)
{
}

AnalysisDataAsset DataRepository::upsertAsset(const AnalysisDataAsset& _assetInput)
{
    AnalysisDataAsset _assetStored = _assetInput;
    const QString _strTargetKey = BuildAssetKey(_assetStored);

    for (int _nAssetIdx = 0; _nAssetIdx < mvAssets.size(); ++_nAssetIdx) {
        if (BuildAssetKey(mvAssets[_nAssetIdx]) != _strTargetKey) {
            continue;
        }

        _assetStored.strAssetId = mvAssets[_nAssetIdx].strAssetId;
        mvAssets[_nAssetIdx] = _assetStored;
        mstrCurrentAssetId = _assetStored.strAssetId;
        emit assetListChanged();
        emit assetUpserted(_assetStored);
        emit currentAssetChanged(_assetStored);
        return _assetStored;
    }

    if (_assetStored.strAssetId.trimmed().isEmpty()) {
        _assetStored.strAssetId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    mvAssets.append(_assetStored);
    mstrCurrentAssetId = _assetStored.strAssetId;
    emit assetListChanged();
    emit assetUpserted(_assetStored);
    emit currentAssetChanged(_assetStored);
    return _assetStored;
}

void DataRepository::clearAssets()
{
    mvAssets.clear();
    mstrCurrentAssetId.clear();
    emit assetListChanged();
    emit currentAssetCleared();
}

bool DataRepository::removeAssetById(const QString& _strAssetId)
{
    const QString _strTargetAssetId = _strAssetId.trimmed();
    if (_strTargetAssetId.isEmpty()) {
        return false;
    }

    for (int _nAssetIdx = 0; _nAssetIdx < mvAssets.size(); ++_nAssetIdx) {
        if (mvAssets[_nAssetIdx].strAssetId != _strTargetAssetId) {
            continue;
        }

        const bool _bRemovingCurrent =
            (mstrCurrentAssetId == _strTargetAssetId);
        mvAssets.removeAt(_nAssetIdx);

        AnalysisDataAsset _assetNextCurrent;
        bool _bHasNextCurrent = false;
        if (_bRemovingCurrent) {
            if (mvAssets.isEmpty()) {
                mstrCurrentAssetId.clear();
            } else {
                int _nNextAssetIdx = _nAssetIdx;
                if (_nNextAssetIdx >= mvAssets.size()) {
                    _nNextAssetIdx = mvAssets.size() - 1;
                }
                _assetNextCurrent = mvAssets.at(_nNextAssetIdx);
                mstrCurrentAssetId = _assetNextCurrent.strAssetId;
                _bHasNextCurrent = true;
            }
        }

        emit assetListChanged();
        if (_bRemovingCurrent) {
            if (_bHasNextCurrent) {
                emit currentAssetChanged(_assetNextCurrent);
            } else {
                emit currentAssetCleared();
            }
        }
        return true;
    }

    return false;
}

QList<AnalysisDataAsset> DataRepository::getAssets() const
{
    return mvAssets;
}

AnalysisDataAsset DataRepository::findAssetById(const QString& _strAssetId) const
{
    for (const AnalysisDataAsset& _assetCurrent : mvAssets) {
        if (_assetCurrent.strAssetId == _strAssetId) {
            return _assetCurrent;
        }
    }
    return AnalysisDataAsset();
}

bool DataRepository::hasAssets() const
{
    return !mvAssets.isEmpty();
}

AnalysisDataAsset DataRepository::currentAsset() const
{
    if (mstrCurrentAssetId.trimmed().isEmpty()) {
        return AnalysisDataAsset();
    }
    return findAssetById(mstrCurrentAssetId);
}

bool DataRepository::hasCurrentAsset() const
{
    return !mstrCurrentAssetId.trimmed().isEmpty();
}

bool DataRepository::selectAssetById(const QString& _strAssetId)
{
    if (_strAssetId.trimmed().isEmpty()) {
        mstrCurrentAssetId.clear();
        emit currentAssetCleared();
        return true;
    }

    for (const AnalysisDataAsset& _assetCurrent : mvAssets) {
        if (_assetCurrent.strAssetId != _strAssetId) {
            continue;
        }

        mstrCurrentAssetId = _strAssetId;
        emit currentAssetChanged(_assetCurrent);
        return true;
    }

    return false;
}

QString DataRepository::BuildAssetKey(const AnalysisDataAsset& _assetInput)
{
    return QString("%1|%2")
        .arg(_assetInput.strSourcePath.trimmed().toLower())
        .arg(static_cast<int>(_assetInput.eAssetType));
}
