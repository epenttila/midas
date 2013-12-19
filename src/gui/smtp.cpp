#include "smtp.h"

smtp::smtp(const QString& host, std::uint16_t port)
    : host_(host)
    , port_(port)
{    
    socket_ = new QTcpSocket(this);
    connect(socket_, &QTcpSocket::readyRead, this, &smtp::ready_read);
}

void smtp::send(const QString &from, const QString &to, const QString &subject, const QString &body)
{
    state_ = INIT;
    from_ = from;
    rcpt_ = to;

    message_ = "To: " + to + "\n";
    message_.append("From: " + from + "\n");
    message_.append("Subject: " + subject + "\n");
    message_.append(body);
    message_.replace("\n", "\r\n");

    socket_->connectToHost(host_, port_);
}

void smtp::ready_read()
{
    const QString full_response = socket_->readLine();
    auto response = full_response;
    response.truncate(3);

    QTextStream out(socket_);

    if (state_ == INIT && response == "220")
    {
        out << "HELO localhost\r\n";
        state_ = MAIL;
    }
    else if (state_ == MAIL && response == "250")
    {
        out << "MAIL FROM:<" << from_ << ">\r\n";
        state_ = RCPT;
    }
    else if (state_ == RCPT && response == "250")
    {
        out << "RCPT TO:<" << rcpt_ << ">\r\n";
        state_ = DATA;
    }
    else if (state_ == DATA && response == "250")
    {
        out << "DATA\r\n";
        state_ = BODY;
    }
    else if (state_ == BODY && response == "354")
    {
        out << message_ << "\r\n.\r\n";
        state_ = QUIT;
    }
    else if (state_ == QUIT && response == "250")
    {
        out << "QUIT\r\n";
        state_ = CLOSE;
    }
    else if (state_ == CLOSE && response == "221")
    {
        emit status("Message sent");
    }
    else
    {
        emit status(QString("Failed to send message (%1)").arg(full_response));
    }
}
