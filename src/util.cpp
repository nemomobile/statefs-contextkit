#include <statefs/qt/util.hpp>
#include <cor/error.hpp>

#include <QRegExp>
#include <QDebug>
#include <QDate>
#include <QDateTime>
#include <QDir>

namespace statefs { namespace qt {

bool splitPropertyName(const QString &name, QStringList &parts)
{
    QRegExp re("[./]");
    re.setPatternSyntax(QRegExp::RegExp);
    parts = name.split(re);
    if (!parts.size()) {
        qWarning() << "Can't parse property name:" << name;
        return false;
    }

    if (parts.size() > 2) {
        auto fname = parts.last();
        parts.pop_back();
        if (!parts[0].size()) // for names like "/..."
            parts.pop_front();
        auto ns = parts.join("_");
        parts.clear();
        parts.push_back(ns);
        parts.push_back(fname);
    }
    // should be 2 parts: namespace and property_name
    return (parts.size() == 2);
}

QString getPath(const QString &name)
{
    QStringList parts;
    if (!splitPropertyName(name, parts))
        return "";

    parts.push_front("namespaces");
    parts.push_front("state");
    parts.push_front(::getenv("XDG_RUNTIME_DIR")); // TODO hardcoded path!

    return parts.join(QDir::separator());
}

static QRegExp re(QString const &cs)
{
	auto in_spaces = [](QString const& s) {
		static const QString aspaces = "\\s*";
		return aspaces + s + aspaces;
	};
	return QRegExp(in_spaces(cs), Qt::CaseSensitive, QRegExp::RegExp2);
};

static const QString date_re("[0-9]{4}-[0-9]{2}-[0-9]{2}");
static const QString hhmm_re("[0-9]{2}:[0-9]{2}");
static const QString time_re(QString("%1(:[0-9]{2})?").arg(hhmm_re));
static const QString tz_re("(Z|[+-][0-9]{2}(:[0-9]{2})?)");
static const QString datetime_re(QString("%1T%2%3")
								 .arg(date_re, time_re, tz_re));


static const std::initializer_list<std::pair<QRegExp, QVariant::Type> > re_types
= {{re("[+-][0-9]+"), QVariant::Int}
   , {re("[0-9]+"), QVariant::UInt}
   , {re("[+-]?([0-9]+\\.[0-9]*|[0-9]*\\.[0-9]+)"), QVariant::Double}
   , {re(date_re), QVariant::Date}
   , {re(time_re), QVariant::Time}
   , {re(datetime_re), QVariant::DateTime}
};

QVariant valueDecode(QString const& s)
{
	if (!s.size())
		return QVariant(s);

	QVariant v(s);
	for (auto const& re_type : re_types) {
		if (re_type.first.exactMatch(s)) {
			v.convert(re_type.second);
			break;
		}
	}
	return v;
}

QString valueEncode(QVariant const& v)
{
    switch(v.type()) {
    case QVariant::Bool:
        return v.toBool() ? "1" : "0";
    default:
        return v.toString();
    }
}

QVariant valueDefault(QVariant const& v)
{
    switch (v.type()) {
    case QVariant::String:
        return "";
    case QVariant::Int:
        return 0;
    case QVariant::UInt:
        return QVariant((unsigned)0);
    case QVariant::Double:
        return QVariant((double)0);
    case QVariant::Date:
        return QDate();
    case QVariant::DateTime:
        return QDateTime();
    default:
        return QVariant(v.type());
    }
}

}}
