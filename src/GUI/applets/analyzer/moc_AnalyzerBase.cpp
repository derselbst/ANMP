/****************************************************************************
** Meta object code from reading C++ file 'AnalyzerBase.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "AnalyzerBase.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AnalyzerBase.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_AnalyzerBase_t {
    QByteArrayData data[9];
    char stringdata[97];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AnalyzerBase_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AnalyzerBase_t qt_meta_stringdata_AnalyzerBase = {
    {
QT_MOC_LITERAL(0, 0, 12), // "AnalyzerBase"
QT_MOC_LITERAL(1, 13, 14), // "connectSignals"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 17), // "disconnectSignals"
QT_MOC_LITERAL(4, 47, 11), // "processData"
QT_MOC_LITERAL(5, 59, 11), // "const Song*"
QT_MOC_LITERAL(6, 71, 8), // "thescope"
QT_MOC_LITERAL(7, 80, 7), // "frame_t"
QT_MOC_LITERAL(8, 88, 8) // "playhead"

    },
    "AnalyzerBase\0connectSignals\0\0"
    "disconnectSignals\0processData\0const Song*\0"
    "thescope\0frame_t\0playhead"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AnalyzerBase[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   29,    2, 0x0a /* Public */,
       3,    0,   30,    2, 0x0a /* Public */,
       4,    2,   31,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 5, 0x80000000 | 7,    6,    8,

       0        // eod
};

void AnalyzerBase::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        AnalyzerBase *_t = static_cast<AnalyzerBase *>(_o);
        switch (_id) {
        case 0: _t->connectSignals(); break;
        case 1: _t->disconnectSignals(); break;
        case 2: _t->processData((*reinterpret_cast< const Song*(*)>(_a[1])),(*reinterpret_cast< frame_t(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObject AnalyzerBase::staticMetaObject = {
    { &QGLWidget::staticMetaObject, qt_meta_stringdata_AnalyzerBase.data,
      qt_meta_data_AnalyzerBase,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *AnalyzerBase::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AnalyzerBase::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_AnalyzerBase.stringdata))
        return static_cast<void*>(const_cast< AnalyzerBase*>(this));
    return QGLWidget::qt_metacast(_clname);
}

int AnalyzerBase::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGLWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
