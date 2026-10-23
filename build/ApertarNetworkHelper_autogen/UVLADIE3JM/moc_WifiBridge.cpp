/****************************************************************************
** Meta object code from reading C++ file 'WifiBridge.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/WifiBridge.hpp"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WifiBridge.hpp' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10WifiBridgeE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN10WifiBridgeE = QtMocHelpers::stringData(
    "WifiBridge",
    "networksChanged",
    "",
    "scanningChanged",
    "connectingChanged",
    "statusChanged",
    "hotspotChanged",
    "lastErrorChanged",
    "handleProcessFinished",
    "exitCode",
    "QProcess::ExitStatus",
    "exitStatus",
    "handleProcessError",
    "QProcess::ProcessError",
    "error",
    "handleAutoHotspotTick",
    "refresh",
    "setWifiEnabled",
    "enabled",
    "connectToNetwork",
    "ssid",
    "password",
    "disconnectCurrent",
    "startHotspot",
    "stopHotspot",
    "networks",
    "QVariantList",
    "scanning",
    "connecting",
    "wifiAvailable",
    "wifiEnabled",
    "statusText",
    "currentSsid",
    "wifiDevice",
    "hotspotActive",
    "hotspotSsid",
    "hotspotPassword",
    "hotspotStatusText",
    "lastError"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN10WifiBridgeE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      18,   14, // methods
      13,  160, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  122,    2, 0x06,   14 /* Public */,
       3,    0,  123,    2, 0x06,   15 /* Public */,
       4,    0,  124,    2, 0x06,   16 /* Public */,
       5,    0,  125,    2, 0x06,   17 /* Public */,
       6,    0,  126,    2, 0x06,   18 /* Public */,
       7,    0,  127,    2, 0x06,   19 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       8,    2,  128,    2, 0x08,   20 /* Private */,
      12,    1,  133,    2, 0x08,   23 /* Private */,
      15,    0,  136,    2, 0x08,   25 /* Private */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      16,    0,  137,    2, 0x02,   26 /* Public */,
      17,    1,  138,    2, 0x02,   27 /* Public */,
      19,    2,  141,    2, 0x02,   29 /* Public */,
      19,    1,  146,    2, 0x22,   32 /* Public | MethodCloned */,
      22,    0,  149,    2, 0x02,   34 /* Public */,
      23,    2,  150,    2, 0x02,   35 /* Public */,
      23,    1,  155,    2, 0x22,   38 /* Public | MethodCloned */,
      23,    0,  158,    2, 0x22,   40 /* Public | MethodCloned */,
      24,    0,  159,    2, 0x02,   41 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Int, 0x80000000 | 10,    9,   11,
    QMetaType::Void, 0x80000000 | 13,   14,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Bool,
    QMetaType::Bool, QMetaType::Bool,   18,
    QMetaType::Bool, QMetaType::QString, QMetaType::QString,   20,   21,
    QMetaType::Bool, QMetaType::QString,   20,
    QMetaType::Bool,
    QMetaType::Bool, QMetaType::QString, QMetaType::QString,   20,   21,
    QMetaType::Bool, QMetaType::QString,   20,
    QMetaType::Bool,
    QMetaType::Bool,

 // properties: name, type, flags, notifyId, revision
      25, 0x80000000 | 26, 0x00015009, uint(0), 0,
      27, QMetaType::Bool, 0x00015001, uint(1), 0,
      28, QMetaType::Bool, 0x00015001, uint(2), 0,
      29, QMetaType::Bool, 0x00015001, uint(3), 0,
      30, QMetaType::Bool, 0x00015001, uint(3), 0,
      31, QMetaType::QString, 0x00015001, uint(3), 0,
      32, QMetaType::QString, 0x00015001, uint(3), 0,
      33, QMetaType::QString, 0x00015001, uint(3), 0,
      34, QMetaType::Bool, 0x00015001, uint(4), 0,
      35, QMetaType::QString, 0x00015103, uint(4), 0,
      36, QMetaType::QString, 0x00015103, uint(4), 0,
      37, QMetaType::QString, 0x00015001, uint(4), 0,
      38, QMetaType::QString, 0x00015001, uint(5), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject WifiBridge::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN10WifiBridgeE.offsetsAndSizes,
    qt_meta_data_ZN10WifiBridgeE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN10WifiBridgeE_t,
        // property 'networks'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'scanning'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'connecting'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'wifiAvailable'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'wifiEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'statusText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentSsid'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'wifiDevice'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'hotspotActive'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'hotspotSsid'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'hotspotPassword'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'hotspotStatusText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'lastError'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<WifiBridge, std::true_type>,
        // method 'networksChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'scanningChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'connectingChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'statusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'hotspotChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'lastErrorChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'handleProcessFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QProcess::ExitStatus, std::false_type>,
        // method 'handleProcessError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QProcess::ProcessError, std::false_type>,
        // method 'handleAutoHotspotTick'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refresh'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'setWifiEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'connectToNetwork'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'connectToNetwork'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'disconnectCurrent'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'startHotspot'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'startHotspot'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'startHotspot'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'stopHotspot'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void WifiBridge::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<WifiBridge *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->networksChanged(); break;
        case 1: _t->scanningChanged(); break;
        case 2: _t->connectingChanged(); break;
        case 3: _t->statusChanged(); break;
        case 4: _t->hotspotChanged(); break;
        case 5: _t->lastErrorChanged(); break;
        case 6: _t->handleProcessFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 7: _t->handleProcessError((*reinterpret_cast< std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        case 8: _t->handleAutoHotspotTick(); break;
        case 9: { bool _r = _t->refresh();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 10: { bool _r = _t->setWifiEnabled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 11: { bool _r = _t->connectToNetwork((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 12: { bool _r = _t->connectToNetwork((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 13: { bool _r = _t->disconnectCurrent();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 14: { bool _r = _t->startHotspot((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 15: { bool _r = _t->startHotspot((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 16: { bool _r = _t->startHotspot();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 17: { bool _r = _t->stopHotspot();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (WifiBridge::*)();
            if (_q_method_type _q_method = &WifiBridge::networksChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (WifiBridge::*)();
            if (_q_method_type _q_method = &WifiBridge::scanningChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (WifiBridge::*)();
            if (_q_method_type _q_method = &WifiBridge::connectingChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (WifiBridge::*)();
            if (_q_method_type _q_method = &WifiBridge::statusChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (WifiBridge::*)();
            if (_q_method_type _q_method = &WifiBridge::hotspotChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (WifiBridge::*)();
            if (_q_method_type _q_method = &WifiBridge::lastErrorChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QVariantList*>(_v) = _t->networks(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->scanning(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->connecting(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->wifiAvailable(); break;
        case 4: *reinterpret_cast< bool*>(_v) = _t->wifiEnabled(); break;
        case 5: *reinterpret_cast< QString*>(_v) = _t->statusText(); break;
        case 6: *reinterpret_cast< QString*>(_v) = _t->currentSsid(); break;
        case 7: *reinterpret_cast< QString*>(_v) = _t->wifiDevice(); break;
        case 8: *reinterpret_cast< bool*>(_v) = _t->hotspotActive(); break;
        case 9: *reinterpret_cast< QString*>(_v) = _t->hotspotSsid(); break;
        case 10: *reinterpret_cast< QString*>(_v) = _t->hotspotPassword(); break;
        case 11: *reinterpret_cast< QString*>(_v) = _t->hotspotStatusText(); break;
        case 12: *reinterpret_cast< QString*>(_v) = _t->lastError(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 9: _t->setHotspotSsid(*reinterpret_cast< QString*>(_v)); break;
        case 10: _t->setHotspotPassword(*reinterpret_cast< QString*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *WifiBridge::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *WifiBridge::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN10WifiBridgeE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int WifiBridge::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 18;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void WifiBridge::networksChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void WifiBridge::scanningChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void WifiBridge::connectingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void WifiBridge::statusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void WifiBridge::hotspotChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void WifiBridge::lastErrorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP
