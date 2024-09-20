#include <QCoreApplication>
#include <QPair>
#include <QString>
#include <QMap>
#include <QSet>

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QScopedPointer>
#include <QSettings>
#include <QUuid>
#include <QDebug>
#include <QDir>
#include <QPair>

#include <QtConcurrent/QtConcurrent>
#include <QFutureInterface>
#include <QFuture>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QtHttpServer>
#include <QHostAddress>

#include <QCryptographicHash>
#include <optional>
#include <algorithm>

std::optional<QPair<qreal, qreal>> calculateLinearRegression(const QVector<QPair<qreal, qreal> > &xyVector)
{
    if (xyVector.isEmpty())
        return std::nullopt;

    const QPair<qreal, qreal> sumXandSumY = [](const QVector<QPair<qreal, qreal> > &xyVector) -> QPair<qreal, qreal>
    {
        QPair<qreal, qreal> sumXandSumY;

        for (const QPair<qreal, qreal> &point : xyVector)
        {
            sumXandSumY.first  += point.first;
            sumXandSumY.second += point.second;
        }

        return sumXandSumY;

    }(xyVector);

    const QPair<qreal, qreal> meanXandMeanY {sumXandSumY.first  / xyVector.size(), sumXandSumY.second / xyVector.size()};

    const QPair<qreal, qreal> numeratorAndDenominator = [](const QVector<QPair<qreal, qreal> > &xyVector, const QPair<qreal, qreal> &meanXandMeanY) -> QPair<qreal, qreal>
    {
        QPair<qreal, qreal> numeratorAndDenominator;

        for (const QPair<qreal, qreal> &point : xyVector)
        {
            numeratorAndDenominator.first += (point.first - meanXandMeanY.first) * (point.second - meanXandMeanY.second);
            numeratorAndDenominator.second += (point.first - meanXandMeanY.first) * (point.first - meanXandMeanY.first);
        }

        return numeratorAndDenominator;

    }(xyVector, meanXandMeanY);

    if (numeratorAndDenominator.second == 0)
        return std::nullopt;

    const qreal constTerm {meanXandMeanY.second - ((numeratorAndDenominator.first / numeratorAndDenominator.second) * meanXandMeanY.first)};

    return QPair<qreal, qreal> {numeratorAndDenominator.first / numeratorAndDenominator.second, constTerm};
}

std::optional<qreal> calculateConstantTerm(const qreal &count, const qreal &sumX, const qreal &sumY, const qreal &sumXSquare, const qreal &sumXY)
{
    const qreal numerator   {sumY * sumXSquare - (sumX * sumXY)};
    const qreal denominator {count * sumXSquare - (sumX * sumX)};

    if (denominator == 0)
        return std::nullopt;

    return numerator / denominator;
}

qreal predict(const qreal &x, const qreal &coefficient, const qreal &constTerm)
{
    return coefficient * x + constTerm;
}

qreal errorSquare(const QVector<qreal> &x, const QVector<qreal> &y, const qreal &coefficient, const qreal &constTerm)
{
    qreal ans {0};

    for (qreal i = 0;i < x.size(); i++)
        ans += ((predict(x[i] - y[i], coefficient, constTerm)) * (predict(x[i] - y[i], coefficient, constTerm)));

    return ans;
}

qreal errorIn(const QVector<qreal> &x, const QVector<qreal> &y, qreal num,  const qreal &coefficient, const qreal &constTerm)
{
    for (qreal i = 0; i < x.size(); i++)
        if (num == x[i])
            return (y[i] - predict(x[i], coefficient, constTerm));

    return 0;
}

const QByteArray rapidApiKey {"cab828ab7936b9f16203634eae84cc6560813e659ab585b73668944ac9bee0561403944d5dd6c88704dd67df4785a395f4f24307f2b8733dd644081c112cf0c2"};


int main(int argc, char *argv[])
{
    QCoreApplication app {argc, argv};

    const quint16 PORT {50000};
    const QScopedPointer<QHttpServer> httpServer {new QHttpServer {&app}};

    httpServer->route("/ping", QHttpServerRequest::Method::Get,
    [](const QHttpServerRequest &request) -> QFuture<QHttpServerResponse>
    {
        qDebug() << "Ping verarbeitet";

#ifdef QT_DEBUG
        Q_UNUSED(request)
#else
        const bool requestIsFromRapidAPI = [](const QHttpServerRequest &request) -> bool
        {
            for (const QPair<QByteArray, QByteArray> &header : request.headers())
                if (header.first == "X-RapidAPI-Proxy-Secret" && QCryptographicHash::hash(header.second, QCryptographicHash::Sha512).toHex() == rapidApiKey)
                    return true;

            return false;

        }(request);

        if (!requestIsFromRapidAPI)
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "HTTP-Requests allowed only via RapidAPI-Gateway."}
                    }
                };
            });
#endif
        return QtConcurrent::run([]()
        {
            return QHttpServerResponse
            {
                QJsonObject
                {
                    {"Message", "pong"}
                }
            };
        });
    });

    httpServer->route("/linearregression", QHttpServerRequest::Method::Get    |
                                           QHttpServerRequest::Method::Put     |
                                           QHttpServerRequest::Method::Head    |
                                           QHttpServerRequest::Method::Trace   |
                                           QHttpServerRequest::Method::Patch   |
                                           QHttpServerRequest::Method::Delete  |
                                           QHttpServerRequest::Method::Options |
                                           QHttpServerRequest::Method::Connect |
                                           QHttpServerRequest::Method::Unknown,
    [](const QHttpServerRequest &request) -> QFuture<QHttpServerResponse>
    {
#ifdef QT_DEBUG
        Q_UNUSED(request)
#else
        const bool requestIsFromRapidAPI = [](const QHttpServerRequest &request) -> bool
        {
            for (const QPair<QByteArray, QByteArray> &header : request.headers())
            {
                if (header.first == "X-RapidAPI-Proxy-Secret" && QCryptographicHash::hash(header.second, QCryptographicHash::Sha512).toHex() == rapidApiKey)
                    return true;
            }

            return false;

        }(request);

        if (!requestIsFromRapidAPI)
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "HTTP-Requests allowed only via RapidAPI-Gateway."}
                    }
                };
            });
#endif
        return QtConcurrent::run([]()
        {
            return QHttpServerResponse
            {
                QJsonObject
                {
                    {"Message", "The used HTTP-Method is not implemented."}
                }
            };
        });
    });

    httpServer->route("/linearregression", QHttpServerRequest::Method::Post,
    [](const QHttpServerRequest &request) -> QFuture<QHttpServerResponse>
    {
        qDebug() << "Anfrage von IP: " << request.remoteAddress().toString();

#ifdef QT_DEBUG
        Q_UNUSED(request)
#else
        const bool requestIsFromRapidAPI = [](const QHttpServerRequest &request) -> bool
        {
            for (const QPair<QByteArray, QByteArray> &header : request.headers())
            {
                if (header.first == "X-RapidAPI-Proxy-Secret" && QCryptographicHash::hash(header.second, QCryptographicHash::Sha512).toHex() == rapidApiKey)
                    return true;
            }

            return false;

        }(request);

        if (!requestIsFromRapidAPI)
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "HTTP-Requests allowed only via RapidAPI-Gateway."}
                    }
                };
            });
#endif
        if (request.body().isEmpty())
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "HTTP-Request body is empty."}
                    }
                };
            });

        const QJsonDocument jsonDocument {QJsonDocument::fromJson(request.body())};

        if (jsonDocument.isNull())
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "Invalid data sent. Please send a valid JSON-Object."}
                    }
                };
            });

        const QJsonObject jsonObject {jsonDocument.object()};

        if (jsonObject.isEmpty())
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "Invalid data sent. Please send a valid JSON-Object."}
                    }
                };
           });

        if (!jsonObject.contains("X"))
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "Invalid data sent. Array of X-Points is missing."}
                    }
                };
            });

        if (!jsonObject.contains("Y"))
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "Invalid data sent. Array of Y-Points is missing."}
                    }
                };
            });

        const QJsonArray xArray {jsonObject.value("X").toArray()};
        const QJsonArray yArray {jsonObject.value("Y").toArray()};

        if (xArray.isEmpty())
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "Invalid data sent. Array of X-Points is missing."}
                    }
                };
            });

        if (xArray[0].toArray().isEmpty())
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "Invalid data sent. Array of X-Points is missing."}
                    }
                };
            });

        if (yArray.isEmpty())
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                      {"Message", "Invalid data sent. Array of Y-Points is missing."}
                    }
                };
            });

        if (yArray[0].toArray().isEmpty())
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "Invalid data sent. Array of Y-Points is missing."}
                    }
                };
            });

        if (xArray[0].toArray().size() != yArray[0].toArray().size())
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "Invalid data sent. Arrays X and Y are not of the same size."}
                    }
                };
            });

        const QVector<QPair<qreal, qreal> > xyVector = [](const QJsonArray &xArray, const QJsonArray &yArray) -> QVector<QPair<qreal, qreal> >
        {
            QVector<QPair<qreal, qreal> > xyVector;

            for (int i {0}; i < xArray[0].toArray().size(); ++i)
                xyVector << QPair<qreal, qreal> {xArray[0][i].toVariant().toReal(), yArray[0][i].toVariant().toReal()};

            return xyVector;

        }(xArray, yArray);

        const std::optional<QPair<qreal, qreal> > linearRegression {calculateLinearRegression(xyVector)};

        if (linearRegression == std::nullopt)
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "Invalid data sent. Coefficient cannot be calculated."}
                    },

                    QHttpServerResponse::StatusCode::InternalServerError
                };
            });

        return QtConcurrent::run([linearRegression]()
        {
            return QHttpServerResponse
            {
                QJsonObject
                {
                    {"Coefficient", QString::number(linearRegression->first, 'f')},
                    {"Constant",    QString::number(linearRegression->second, 'f')},
                    {"Function",    "y = " + QString::number(linearRegression->first, 'f') + "x " + (linearRegression->second > 0 ? "+" : "") + QString::number(linearRegression->second, 'f')}
                }
            };
        });
    });

    if (httpServer->listen(QHostAddress::Any, static_cast<quint16>(PORT)) == 0)
        return -1;

    return app.exec();
}
