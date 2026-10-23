/****************************************************************************
** Meta object code from reading C++ file 'cliplistmodel.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/cliplistmodel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'cliplistmodel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN13ClipListModelE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN13ClipListModelE = QtMocHelpers::stringData(
    "ClipListModel",
    "countChanged",
    "",
    "rootPathChanged",
    "stillModeChanged",
    "sortModeChanged",
    "browserOpenChanged",
    "loadingChanged",
    "statusTextChanged",
    "refresh",
    "indexOfPath",
    "path",
    "pathAt",
    "index",
    "nameAt",
    "frameCountAt",
    "durationTextAt",
    "removeClip",
    "rootPath",
    "stillMode",
    "sortMode",
    "browserOpen",
    "count",
    "loading",
    "statusText",
    "ClipRoles",
    "NameRole",
    "PathRole",
    "FrameCountRole",
    "DurationTextRole",
    "ThumbnailSourceRole",
    "ShotDateRole",
    "FormatLabelRole"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN13ClipListModelE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       7,  124, // properties
       1,  159, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   98,    2, 0x06,    9 /* Public */,
       3,    0,   99,    2, 0x06,   10 /* Public */,
       4,    0,  100,    2, 0x06,   11 /* Public */,
       5,    0,  101,    2, 0x06,   12 /* Public */,
       6,    0,  102,    2, 0x06,   13 /* Public */,
       7,    0,  103,    2, 0x06,   14 /* Public */,
       8,    0,  104,    2, 0x06,   15 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       9,    0,  105,    2, 0x02,   16 /* Public */,
      10,    1,  106,    2, 0x102,   17 /* Public | MethodIsConst  */,
      12,    1,  109,    2, 0x102,   19 /* Public | MethodIsConst  */,
      14,    1,  112,    2, 0x102,   21 /* Public | MethodIsConst  */,
      15,    1,  115,    2, 0x102,   23 /* Public | MethodIsConst  */,
      16,    1,  118,    2, 0x102,   25 /* Public | MethodIsConst  */,
      17,    1,  121,    2, 0x02,   27 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Int, QMetaType::QString,   11,
    QMetaType::QString, QMetaType::Int,   13,
    QMetaType::QString, QMetaType::Int,   13,
    QMetaType::Int, QMetaType::Int,   13,
    QMetaType::QString, QMetaType::Int,   13,
    QMetaType::Bool, QMetaType::QString,   11,

 // properties: name, type, flags, notifyId, revision
      18, QMetaType::QString, 0x00015103, uint(1), 0,
      19, QMetaType::Bool, 0x00015103, uint(2), 0,
      20, QMetaType::QString, 0x00015103, uint(3), 0,
      21, QMetaType::Bool, 0x00015103, uint(4), 0,
      22, QMetaType::Int, 0x00015001, uint(0), 0,
      23, QMetaType::Bool, 0x00015001, uint(5), 0,
      24, QMetaType::QString, 0x00015001, uint(6), 0,

 // enums: name, alias, flags, count, data
      25,   25, 0x0,    7,  164,

 // enum data: key, value
      26, uint(ClipListModel::NameRole),
      27, uint(ClipListModel::PathRole),
      28, uint(ClipListModel::FrameCountRole),
      29, uint(ClipListModel::DurationTextRole),
      30, uint(ClipListModel::ThumbnailSourceRole),
      31, uint(ClipListModel::ShotDateRole),
      32, uint(ClipListModel::FormatLabelRole),

       0        // eod
};

Q_CONSTINIT const QMetaObject ClipListModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractListModel::staticMetaObject>(),
    qt_meta_stringdata_ZN13ClipListModelE.offsetsAndSizes,
    qt_meta_data_ZN13ClipListModelE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN13ClipListModelE_t,
        // property 'rootPath'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'stillMode'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'sortMode'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'browserOpen'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'count'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'loading'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'statusText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // enum 'ClipRoles'
        QtPrivate::TypeAndForceComplete<ClipListModel::ClipRoles, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ClipListModel, std::true_type>,
        // method 'countChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'rootPathChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stillModeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sortModeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'browserOpenChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadingChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'statusTextChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refresh'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'indexOfPath'
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'pathAt'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'nameAt'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'frameCountAt'
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'durationTextAt'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'removeClip'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void ClipListModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ClipListModel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->countChanged(); break;
        case 1: _t->rootPathChanged(); break;
        case 2: _t->stillModeChanged(); break;
        case 3: _t->sortModeChanged(); break;
        case 4: _t->browserOpenChanged(); break;
        case 5: _t->loadingChanged(); break;
        case 6: _t->statusTextChanged(); break;
        case 7: _t->refresh(); break;
        case 8: { int _r = _t->indexOfPath((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 9: { QString _r = _t->pathAt((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 10: { QString _r = _t->nameAt((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 11: { int _r = _t->frameCountAt((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 12: { QString _r = _t->durationTextAt((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 13: { bool _r = _t->removeClip((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (ClipListModel::*)();
            if (_q_method_type _q_method = &ClipListModel::countChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (ClipListModel::*)();
            if (_q_method_type _q_method = &ClipListModel::rootPathChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (ClipListModel::*)();
            if (_q_method_type _q_method = &ClipListModel::stillModeChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (ClipListModel::*)();
            if (_q_method_type _q_method = &ClipListModel::sortModeChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (ClipListModel::*)();
            if (_q_method_type _q_method = &ClipListModel::browserOpenChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (ClipListModel::*)();
            if (_q_method_type _q_method = &ClipListModel::loadingChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (ClipListModel::*)();
            if (_q_method_type _q_method = &ClipListModel::statusTextChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->rootPath(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->stillMode(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->sortMode(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->browserOpen(); break;
        case 4: *reinterpret_cast< int*>(_v) = _t->count(); break;
        case 5: *reinterpret_cast< bool*>(_v) = _t->loading(); break;
        case 6: *reinterpret_cast< QString*>(_v) = _t->statusText(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setRootPath(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->setStillMode(*reinterpret_cast< bool*>(_v)); break;
        case 2: _t->setSortMode(*reinterpret_cast< QString*>(_v)); break;
        case 3: _t->setBrowserOpen(*reinterpret_cast< bool*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *ClipListModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ClipListModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN13ClipListModelE.stringdata0))
        return static_cast<void*>(this);
    return QAbstractListModel::qt_metacast(_clname);
}

int ClipListModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractListModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void ClipListModel::countChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ClipListModel::rootPathChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ClipListModel::stillModeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ClipListModel::sortModeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void ClipListModel::browserOpenChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void ClipListModel::loadingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void ClipListModel::statusTextChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
QT_WARNING_POP
