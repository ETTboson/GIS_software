#include "ollamaclient.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QUrl>
#include <QDebug>

const QString OllamaClient::S_STR_API_URL = "http://localhost:11434/api/chat";

OllamaClient::OllamaClient(QObject* _pParent)
    : QObject(_pParent)
    , mpNetManager(new QNetworkAccessManager(this))
    , mpCurrentReply(nullptr)
    , mbFinalChunkDelivered(false)
    , mbHadNetworkError(false)
{
}

OllamaClient::~OllamaClient()
{
    abort();
}

void OllamaClient::postRequest(const QJsonObject& _body)
{
    if (mpCurrentReply != nullptr) {
        abort();
    }

    mbaPendingBuffer.clear();
    mbFinalChunkDelivered = false;
    mbHadNetworkError = false;

    QUrl _url(S_STR_API_URL);
    QNetworkRequest _request;
    _request.setUrl(_url);
    _request.setHeader(QNetworkRequest::ContentTypeHeader,
        QString("application/json"));

    const QByteArray _baBody = QJsonDocument(_body).toJson(QJsonDocument::Compact);
    mpCurrentReply = mpNetManager->post(_request, _baBody);

    connect(mpCurrentReply, &QNetworkReply::readyRead,
        this, &OllamaClient::onReadyRead);
    connect(mpCurrentReply, &QNetworkReply::finished,
        this, &OllamaClient::onReplyFinished);
    connect(mpCurrentReply,
        QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
        this, &OllamaClient::onNetworkError);
}

void OllamaClient::abort()
{
    if (mpCurrentReply == nullptr) {
        return;
    }

    disconnect(mpCurrentReply, nullptr, this, nullptr);
    mpCurrentReply->abort();
    mpCurrentReply->deleteLater();
    mpCurrentReply = nullptr;

    mbaPendingBuffer.clear();
    mbFinalChunkDelivered = true;
    mbHadNetworkError = false;

    QJsonObject _jsonFinal;
    _jsonFinal["done"] = true;
    _jsonFinal["aborted"] = true;
    emit replyFinished(_jsonFinal);
}

void OllamaClient::onReadyRead()
{
    if (mpCurrentReply == nullptr) {
        return;
    }

    mbaPendingBuffer += mpCurrentReply->readAll();
    processPendingBuffer(false);
}

void OllamaClient::onReplyFinished()
{
    if (mpCurrentReply == nullptr) {
        return;
    }

    mbaPendingBuffer += mpCurrentReply->readAll();
    processPendingBuffer(true);

    if (!mbFinalChunkDelivered && !mbHadNetworkError) {
        QJsonObject _jsonFinal;
        _jsonFinal["done"] = true;
        emit replyFinished(_jsonFinal);
    }

    mpCurrentReply->deleteLater();
    mpCurrentReply = nullptr;
    mbaPendingBuffer.clear();
    mbFinalChunkDelivered = false;
    mbHadNetworkError = false;
}

void OllamaClient::onNetworkError(QNetworkReply::NetworkError _eCode)
{
    Q_UNUSED(_eCode)
    mbHadNetworkError = true;
    if (mpCurrentReply != nullptr) {
        emit errorOccurred(tr("网络错误: %1").arg(mpCurrentReply->errorString()));
    }
}

void OllamaClient::processPendingBuffer(bool _bFlushRemaining)
{
    while (true) {
        const int _nNewlineIdx = mbaPendingBuffer.indexOf('\n');
        if (_nNewlineIdx < 0) {
            break;
        }

        const QByteArray _baLine = mbaPendingBuffer.left(_nNewlineIdx).trimmed();
        mbaPendingBuffer.remove(0, _nNewlineIdx + 1);
        if (_baLine.isEmpty()) {
            continue;
        }

        const QJsonObject _jsonObj = parseLine(_baLine);
        if (_jsonObj.isEmpty()) {
            continue;
        }

        if (_jsonObj["done"].toBool()) {
            if (!mbFinalChunkDelivered) {
                mbFinalChunkDelivered = true;
                emit replyFinished(_jsonObj);
            }
        } else {
            emit chunkReceived(_jsonObj);
        }
    }

    if (!_bFlushRemaining) {
        return;
    }

    const QByteArray _baTail = mbaPendingBuffer.trimmed();
    mbaPendingBuffer.clear();
    if (_baTail.isEmpty()) {
        return;
    }

    const QJsonObject _jsonObj = parseLine(_baTail);
    if (_jsonObj.isEmpty()) {
        return;
    }

    if (_jsonObj["done"].toBool()) {
        if (!mbFinalChunkDelivered) {
            mbFinalChunkDelivered = true;
            emit replyFinished(_jsonObj);
        }
    } else {
        emit chunkReceived(_jsonObj);
    }
}

QJsonObject OllamaClient::parseLine(const QByteArray& _baLine) const
{
    QJsonParseError _parseErr;
    const QJsonDocument _doc = QJsonDocument::fromJson(_baLine, &_parseErr);

    if (_parseErr.error != QJsonParseError::NoError) {
        qWarning() << "[OllamaClient] JSON 解析失败:"
            << _parseErr.errorString()
            << "| 原始数据:" << _baLine;
        return QJsonObject();
    }

    if (!_doc.isObject()) {
        qWarning() << "[OllamaClient] 非预期的 JSON 类型（非 Object）:" << _baLine;
        return QJsonObject();
    }

    return _doc.object();
}
