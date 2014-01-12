#pragma once

#pragma warning(push, 1)
#include <string>
#include <QObject>
#include <memory>
#pragma warning(pop)

class QNetworkAccessManager;
class QImage;
class QNetworkReply;

class captcha_manager : public QObject
{
    Q_OBJECT

public:
    enum state_t { UPLOAD_IMAGE, QUERY_SOLUTION, GET_SOLUTION, FINISHED };

    captcha_manager();
    bool upload_image(const QImage& image);
    void query_solution();
    std::string get_solution() const;
    void reset();
    void set_read_url(const std::string& url);
    void set_upload_url(const std::string& url);

private:
    state_t state_;

    void finished(QNetworkReply* reply);

    QNetworkAccessManager* manager_;
    std::string solution_;
    std::unique_ptr<QNetworkReply> reply_;
    std::string upload_url_;
    std::string read_url_;
};
