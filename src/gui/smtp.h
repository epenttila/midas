#pragma once

#pragma warning(push, 1)
#include <memory>
#include <cstdint>
#include <QObject>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QTcpSocket>
#pragma warning(pop)

class smtp : public QObject
{
    Q_OBJECT

public:
    smtp(const QString& host, std::uint16_t port = 25);
    void send(const QString& from, const QString& to, const QString& subject, const QString& body);

signals:
    void status(const QString&);

private:
    enum states { INIT, MAIL, RCPT, DATA, BODY, QUIT, CLOSE } state_;

    void ready_read();

    QString message_;
    QTcpSocket* socket_;
    QString from_;
    QString rcpt_;
    QString host_;
    std::uint16_t port_;
};
