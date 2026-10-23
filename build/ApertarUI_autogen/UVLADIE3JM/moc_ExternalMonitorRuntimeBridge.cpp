/****************************************************************************
** Meta object code from reading C++ file 'ExternalMonitorRuntimeBridge.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ExternalMonitorRuntimeBridge.hpp"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ExternalMonitorRuntimeBridge.hpp' doesn't include <QObject>."
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
struct qt_meta_tag_ZN28ExternalMonitorRuntimeBridgeE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN28ExternalMonitorRuntimeBridgeE = QtMocHelpers::stringData(
    "ExternalMonitorRuntimeBridge",
    "cameraPageActiveChanged",
    "",
    "shutdownStateChanged",
    "mediaStateChanged",
    "captureStateChanged",
    "acceptConnections",
    "captureMainWindowState",
    "captureMediaState",
    "connectSubscriber",
    "readSubscriberMessages",
    "cameraPageActive",
    "shutdownPopupVisible",
    "shutdownPopupTriggered",
    "shutdownHoldActive",
    "shutdownHoldProgress",
    "shutdownErrorText",
    "mediaMounted",
    "remainingMinutesText",
    "remainingStillsText",
    "captureStateAvailable",
    "recordingActive",
    "timelapseActive",
    "timecode"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN28ExternalMonitorRuntimeBridgeE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
      13,   77, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   68,    2, 0x06,   14 /* Public */,
       3,    0,   69,    2, 0x06,   15 /* Public */,
       4,    0,   70,    2, 0x06,   16 /* Public */,
       5,    0,   71,    2, 0x06,   17 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       6,    0,   72,    2, 0x08,   18 /* Private */,
       7,    0,   73,    2, 0x08,   19 /* Private */,
       8,    0,   74,    2, 0x08,   20 /* Private */,
       9,    0,   75,    2, 0x08,   21 /* Private */,
      10,    0,   76,    2, 0x08,   22 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags, notifyId, revision
      11, QMetaType::Bool, 0x00015001, uint(0), 0,
      12, QMetaType::Bool, 0x00015001, uint(1), 0,
      13, QMetaType::Bool, 0x00015001, uint(1), 0,
      14, QMetaType::Bool, 0x00015001, uint(1), 0,
      15, QMetaType::Double, 0x00015001, uint(1), 0,
      16, QMetaType::QString, 0x00015001, uint(1), 0,
      17, QMetaType::Bool, 0x00015001, uint(2), 0,
      18, QMetaType::QString, 0x00015001, uint(2), 0,
      19, QMetaType::QString, 0x00015001, uint(2), 0,
      20, QMetaType::Bool, 0x00015001, uint(3), 0,
      21, QMetaType::Bool, 0x00015001, uint(3), 0,
      22, QMetaType::Bool, 0x00015001, uint(3), 0,
      23, QMetaType::QString, 0x00015001, uint(3), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject ExternalMonitorRuntimeBridge::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN28ExternalMonitorRuntimeBridgeE.offsetsAndSizes,
    qt_meta_data_ZN28ExternalMonitorRuntimeBridgeE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN28ExternalMonitorRuntimeBridgeE_t,
        // property 'cameraPageActive'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'shutdownPopupVisible'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'shutdownPopupTriggered'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'shutdownHoldActive'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'shutdownHoldProgress'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'shutdownErrorText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'mediaMounted'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'remainingMinutesText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'remainingStillsText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'captureStateAvailable'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'recordingActive'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'timelapseActive'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'timecode'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ExternalMonitorRuntimeBridge, std::true_type>,
        // method 'cameraPageActiveChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'shutdownStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'mediaStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'captureStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'acceptConnections'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'captureMainWindowState'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'captureMediaState'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'connectSubscriber'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'readSubscriberMessages'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void ExternalMonitorRuntimeBridge::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ExternalMonitorRuntimeBridge *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->cameraPageActiveChanged(); break;
        case 1: _t->shutdownStateChanged(); break;
        case 2: _t->mediaStateChanged(); break;
        case 3: _t->captureStateChanged(); break;
        case 4: _t->acceptConnections(); break;
        case 5: _t->captureMainWindowState(); break;
        case 6: _t->captureMediaState(); break;
        case 7: _t->connectSubscriber(); break;
        case 8: _t->readSubscriberMessages(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (ExternalMonitorRuntimeBridge::*)();
            if (_q_method_type _q_method = &ExternalMonitorRuntimeBridge::cameraPageActiveChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (ExternalMonitorRuntimeBridge::*)();
            if (_q_method_type _q_method = &ExternalMonitorRuntimeBridge::shutdownStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (ExternalMonitorRuntimeBridge::*)();
            if (_q_method_type _q_method = &ExternalMonitorRuntimeBridge::mediaStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (ExternalMonitorRuntimeBridge::*)();
            if (_q_method_type _q_method = &ExternalMonitorRuntimeBridge::captureStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->cameraPageActive(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->shutdownPopupVisible(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->shutdownPopupTriggered(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->shutdownHoldActive(); break;
        case 4: *reinterpret_cast< double*>(_v) = _t->shutdownHoldProgress(); break;
        case 5: *reinterpret_cast< QString*>(_v) = _t->shutdownErrorText(); break;
        case 6: *reinterpret_cast< bool*>(_v) = _t->mediaMounted(); break;
        case 7: *reinterpret_cast< QString*>(_v) = _t->remainingMinutesText(); break;
        case 8: *reinterpret_cast< QString*>(_v) = _t->remainingStillsText(); break;
        case 9: *reinterpret_cast< bool*>(_v) = _t->captureStateAvailable(); break;
        case 10: *reinterpret_cast< bool*>(_v) = _t->recordingActive(); break;
        case 11: *reinterpret_cast< bool*>(_v) = _t->timelapseActive(); break;
        case 12: *reinterpret_cast< QString*>(_v) = _t->timecode(); break;
        default: break;
        }
    }
}

const QMetaObject *ExternalMonitorRuntimeBridge::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ExternalMonitorRuntimeBridge::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN28ExternalMonitorRuntimeBridgeE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ExternalMonitorRuntimeBridge::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
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
void ExternalMonitorRuntimeBridge::cameraPageActiveChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ExternalMonitorRuntimeBridge::shutdownStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ExternalMonitorRuntimeBridge::mediaStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ExternalMonitorRuntimeBridge::captureStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
