/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt for Python.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef SBK_QTMACEXTRAS_PYTHON_H
#define SBK_QTMACEXTRAS_PYTHON_H

//workaround to access protected functions
#define protected public

#include <sbkpython.h>
#include <sbkconverter.h>
// Module Includes
#include <pyside2_qtcore_python.h>
#include <pyside2_qtgui_python.h>

// Bound library includes
#include <QtMacExtras/qmacpasteboardmime.h>
#include <QtMacExtras/qmactoolbaritem.h>
#include <QtMacExtras/qmactoolbar.h>
// Conversion Includes - Primitive Types
#include <qabstractitemmodel.h>
#include <QString>
#include <QStringList>
#include <signalmanager.h>

// Conversion Includes - Container Types
#include <pysideqflags.h>
#include <QLinkedList>
#include <QList>
#include <QMap>
#include <QMultiMap>
#include <QPair>
#include <QQueue>
#include <QSet>
#include <QStack>
#include <QVector>

// Type indices
enum : int {
    SBK_QMACPASTEBOARDMIME_QMACPASTEBOARDMIMETYPE_IDX        = 1,
    SBK_QMACPASTEBOARDMIME_IDX                               = 0,
    SBK_QMACTOOLBAR_IDX                                      = 2,
    SBK_QMACTOOLBARITEM_STANDARDITEM_IDX                     = 4,
    SBK_QMACTOOLBARITEM_IDX                                  = 3,
    SBK_QtMacExtras_IDX_COUNT                                = 5
};
// This variable stores all Python types exported by this module.
extern PyTypeObject **SbkPySide2_QtMacExtrasTypes;

// This variable stores the Python module object exported by this module.
extern PyObject *SbkPySide2_QtMacExtrasModuleObject;

// This variable stores all type converters exported by this module.
extern SbkConverter **SbkPySide2_QtMacExtrasTypeConverters;

// Converter indices
enum : int {
    SBK_QTMACEXTRAS_QLIST_QBYTEARRAY_IDX                     = 0, // QList<QByteArray >
    SBK_QTMACEXTRAS_QLIST_QMACTOOLBARITEMPTR_IDX             = 1, // QList<QMacToolBarItem* >
    SBK_QTMACEXTRAS_QLIST_QOBJECTPTR_IDX                     = 2, // const QList<QObject* > &
    SBK_QTMACEXTRAS_QLIST_QVARIANT_IDX                       = 3, // QList<QVariant >
    SBK_QTMACEXTRAS_QLIST_QSTRING_IDX                        = 4, // QList<QString >
    SBK_QTMACEXTRAS_QMAP_QSTRING_QVARIANT_IDX                = 5, // QMap<QString,QVariant >
    SBK_QtMacExtras_CONVERTERS_IDX_COUNT                     = 6
};
// Macros for type check

namespace Shiboken
{

// PyType functions, to get the PyObjectType for a type T
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
template<> inline PyTypeObject *SbkType< ::QMacPasteboardMime::QMacPasteboardMimeType >() { return SbkPySide2_QtMacExtrasTypes[SBK_QMACPASTEBOARDMIME_QMACPASTEBOARDMIMETYPE_IDX]; }
template<> inline PyTypeObject *SbkType< ::QMacPasteboardMime >() { return reinterpret_cast<PyTypeObject *>(SbkPySide2_QtMacExtrasTypes[SBK_QMACPASTEBOARDMIME_IDX]); }
template<> inline PyTypeObject *SbkType< ::QMacToolBar >() { return reinterpret_cast<PyTypeObject *>(SbkPySide2_QtMacExtrasTypes[SBK_QMACTOOLBAR_IDX]); }
template<> inline PyTypeObject *SbkType< ::QMacToolBarItem::StandardItem >() { return SbkPySide2_QtMacExtrasTypes[SBK_QMACTOOLBARITEM_STANDARDITEM_IDX]; }
template<> inline PyTypeObject *SbkType< ::QMacToolBarItem >() { return reinterpret_cast<PyTypeObject *>(SbkPySide2_QtMacExtrasTypes[SBK_QMACTOOLBARITEM_IDX]); }
QT_WARNING_POP

} // namespace Shiboken

#endif // SBK_QTMACEXTRAS_PYTHON_H

