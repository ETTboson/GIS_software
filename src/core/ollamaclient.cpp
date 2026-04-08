// src/core/ollamaclient.cpp
#include "ollamaclient.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QUrl>
#include <QDebug>

const QString OllamaClient::S_STR_API_URL = "http://localhost:11434/api/chat";

// ════════════════════════════════════════════════════════
//  构造 / 析构
// ════════════════════════════════════════════════════════
OllamaClient::OllamaClient(QObject* _pParent)
    : QObject(_pParent)
    , mpNetManager(new QNetworkAccessManager(this))
    , mpCurrentReply(nullptr)
{
}

OllamaClient::~OllamaClient()
{
    abort();
}

// ════════════════════════════════════════════════════════
//  postRequest — 发起流式 POST 请求
//  用默认构造 + setUrl 的写法，规避 MSVC 对
//  QNetworkRequest _request(QUrl(...)) 的函数指针歧义
// ════════════════════════════════════════════════════════
void OllamaClient::postRequest(const QJsonObject& _body)
{
    if (mpCurrentReply != nullptr) {
        abort();
    }

    QUrl            _url(S_STR_API_URL);
    QNetworkRequest _request;
    _request.setUrl(_url);
    _request.setHeader(QNetworkRequest::ContentTypeHeader,
        QString("application/json"));

    QByteArray _baBody = QJsonDocument(_body).toJson(QJsonDocument::Compact);
    mpCurrentReply = mpNetManager->post(_request, _baBody);

    connect(mpCurrentReply, &QNetworkReply::readyRead,
        this, &OllamaClient::onReadyRead);
    connect(mpCurrentReply, &QNetworkReply::finished,
        this, &OllamaClient::onReplyFinished);
    connect(mpCurrentReply,
        QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
        this, &OllamaClient::onNetworkError);
}

// ════════════════════════════════════════════════════════
//  abort — 中止当前请求
// ════════════════════════════════════════════════════════
void OllamaClient::abort()
{
    if (mpCurrentReply == nullptr) { return; }

    mpCurrentReply->disconnect(this);
    mpCurrentReply->abort();
    mpCurrentReply->deleteLater();
    mpCurrentReply = nullptr;
}

// ════════════════════════════════════════════════════════
//  onReadyRead — 逐行解析流式 NDJSON
//  每解析出一条有效 JSON 就向上 emit
//  done:true 的那条单独走 replyFinished 通道
// ════════════════════════════════════════════════════════
void OllamaClient::onReadyRead()
{
    if (mpCurrentReply == nullptr) { return; }

    while (mpCurrentReply->canReadLine()) {
        QByteArray _baLine = mpCurrentReply->readLine().trimmed();
        if (_baLine.isEmpty()) { continue; }

        QJsonObject _obj = parseLine(_baLine);
        if (_obj.isEmpty()) { continue; }

        bool _bDone = _obj["done"].toBool();

        if (_bDone) {
            emit replyFinished(_obj);
        }
        else {
            emit chunkReceived(_obj);
        }
    }
}

// ════════════════════════════════════════════════════════
//  onReplyFinished — 请求结束时的兜底处理
// ════════════════════════════════════════════════════════
void OllamaClient::onReplyFinished()
{
    if (mpCurrentReply == nullptr) { return; }

    QByteArray _baRemain = mpCurrentReply->readAll().trimmed();
    if (!_baRemain.isEmpty()) {
        QJsonObject _obj = parseLine(_baRemain);
        if (!_obj.isEmpty()) {
            bool _bDone = _obj["done"].toBool();
            if (_bDone) {
                emit replyFinished(_obj);
            }
            else {
                emit chunkReceived(_obj);
            }
        }
    }

    mpCurrentReply->deleteLater();
    mpCurrentReply = nullptr;
}

// ════════════════════════════════════════════════════════
//  onNetworkError — 网络错误处理
// ════════════════════════════════════════════════════════
void OllamaClient::onNetworkError(QNetworkReply::NetworkError _eCode)
{
    Q_UNUSED(_eCode)
        if (mpCurrentReply != nullptr) {
            emit errorOccurred(tr("网络错误: %1")
                .arg(mpCurrentReply->errorString()));
        }
}

// ════════════════════════════════════════════════════════
//  parseLine — 单行 NDJSON 解析
// ════════════════════════════════════════════════════════
QJsonObject OllamaClient::parseLine(const QByteArray& _baLine) const
{
    QJsonParseError _parseErr;
    QJsonDocument   _doc = QJsonDocument::fromJson(_baLine, &_parseErr);

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