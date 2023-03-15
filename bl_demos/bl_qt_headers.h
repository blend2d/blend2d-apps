#ifndef BL_QT_HEADERS_H_INCLUDED
#define BL_QT_HEADERS_H_INCLUDED

#include <QtGui>
#include <QtWidgets>

#ifdef _WIN32
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
#endif

#endif // BL_QT_HEADERS_H_INCLUDED
