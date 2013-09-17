#include "qt_log_sink.h"
#pragma warning(push, 1)
#include <QPlainTextEdit>
#pragma warning(pop)

qt_log_sink::qt_log_sink(QPlainTextEdit* out)
    : out_(out)
{
}

void qt_log_sink::consume(const boost::log::record_view&, const string_type& str)
{
    out_->appendPlainText(str.c_str());
}
