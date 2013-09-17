#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/frontend_requirements.hpp>

class QPlainTextEdit;

class qt_log_sink : public boost::log::sinks::basic_formatted_sink_backend<
        char,
        boost::log::sinks::synchronized_feeding
    >
{
public:
    explicit qt_log_sink(QPlainTextEdit* out);
    void consume(const boost::log::record_view&, const string_type& str);

private:
    QPlainTextEdit* out_;
};
