#ifndef _STATEFS_QT_UTIL_HPP_
#define _STATEFS_QT_UTIL_HPP_

#include <QString>
#include <QStringList>
#include <QDebug>
#include <QMutex>
#include <QWaitCondition>

#include <array>
#include <memory>

namespace statefs { namespace qt {

bool splitPropertyName(const QString &, QStringList &);
QString getPath(const QString &);

QVariant valueDecode(QString const&);
QString valueEncode(QVariant const&);
QVariant valueDefault(QVariant const&);

}}

#endif // _STATEFS_QT_UTIL_HPP_
