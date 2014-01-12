#include "captcha_manager.h"

#pragma warning(push, 1)
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QBuffer>
#include <QHttpMultiPart>
#include <QImage>
#include <boost/log/trivial.hpp>
#pragma warning(pop)

captcha_manager::captcha_manager()
    : state_(UPLOAD_IMAGE)
{
    manager_ = new QNetworkAccessManager(this);

    connect(manager_, &QNetworkAccessManager::finished, this, &captcha_manager::finished);
}

bool captcha_manager::upload_image(const QImage& image)
{
    if (state_ != UPLOAD_IMAGE)
        return false;

    if (reply_ && reply_->isRunning())
        return false;

    BOOST_LOG_TRIVIAL(info) << "Uploading CAPTCHA image";

    QByteArray array;
    QBuffer buffer(&array);
    image.save(&buffer, "PNG");

    auto multi_part = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentTypeHeader, "image/png");
    part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"file\"; filename=\"image.png\"");
    part.setBody(array);
    multi_part->append(part);

    reply_.reset(manager_->post(QNetworkRequest(QUrl(upload_url_.c_str())), multi_part));
    multi_part->setParent(reply_.get());
            
    state_ = QUERY_SOLUTION;

    return true;
}

void captcha_manager::query_solution()
{
    if (state_ != QUERY_SOLUTION)
        return;

    if (reply_ && reply_->isRunning())
        return;

    BOOST_LOG_TRIVIAL(info) << "Querying CAPTCHA solution";

    reply_.reset(manager_->get(QNetworkRequest(QUrl(read_url_.c_str()))));

    state_ = GET_SOLUTION;
    solution_.clear();
}

void captcha_manager::reset()
{
    state_ = UPLOAD_IMAGE;
    solution_.clear();
}

void captcha_manager::set_read_url(const std::string& url)
{
    read_url_ = url;
}

void captcha_manager::set_upload_url(const std::string& url)
{
    upload_url_ = url;
}

void captcha_manager::finished(QNetworkReply* reply)
{
    switch (state_)
    {
    case GET_SOLUTION:
        {
            solution_ = QString(reply->readAll()).toStdString();

            if (!solution_.empty())
            {
                BOOST_LOG_TRIVIAL(info) << "Captcha solution: " << solution_;
                state_ = FINISHED;
            }
            else
                state_ = QUERY_SOLUTION;
        }
        break;
    }
}

std::string captcha_manager::get_solution() const
{
    return solution_;
}
