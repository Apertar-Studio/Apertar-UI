/****************************************************************************
** Meta object code from reading C++ file 'GyroConfigBridge.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/GyroConfigBridge.hpp"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'GyroConfigBridge.hpp' doesn't include <QObject>."
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
struct qt_meta_tag_ZN16GyroConfigBridgeE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN16GyroConfigBridgeE = QtMocHelpers::stringData(
    "GyroConfigBridge",
    "configChanged",
    "",
    "orientationChanged",
    "syncOffsetMsChanged",
    "livePollingEnabledChanged",
    "livePollingIntervalMsChanged",
    "liveAccelOnlyChanged",
    "liveSampleChanged",
    "levelReferenceChanged",
    "configSaveFinished",
    "ok",
    "message",
    "refresh",
    "setDriverName",
    "driver",
    "setLivePollingEnabled",
    "enabled",
    "setLivePollingIntervalMs",
    "intervalMs",
    "setLiveAccelOnly",
    "setLivePollingRequest",
    "requestId",
    "accelOnly",
    "setLevelReference",
    "clearLevelReference",
    "setOrientation",
    "orientation",
    "setSyncOffsetMs",
    "value",
    "adjustSyncOffsetMs",
    "delta",
    "resetSyncOffset",
    "syncMode",
    "syncOffsetMs",
    "syncOffsetText",
    "sampleRateHz",
    "configPath",
    "livePollingEnabled",
    "livePollingIntervalMs",
    "liveAccelOnly",
    "liveAvailable",
    "liveStatusText",
    "accelXG",
    "accelYG",
    "accelZG",
    "gyroXDps",
    "gyroYDps",
    "gyroZDps",
    "accelRangeG",
    "gyroRangeDps",
    "levelReferenceSet",
    "levelReferenceText",
    "levelAccelXG",
    "levelAccelYG",
    "levelAccelZG"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN16GyroConfigBridgeE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      21,   14, // methods
      25,  187, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       9,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  140,    2, 0x06,   26 /* Public */,
       3,    0,  141,    2, 0x06,   27 /* Public */,
       4,    0,  142,    2, 0x06,   28 /* Public */,
       5,    0,  143,    2, 0x06,   29 /* Public */,
       6,    0,  144,    2, 0x06,   30 /* Public */,
       7,    0,  145,    2, 0x06,   31 /* Public */,
       8,    0,  146,    2, 0x06,   32 /* Public */,
       9,    0,  147,    2, 0x06,   33 /* Public */,
      10,    2,  148,    2, 0x06,   34 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      13,    0,  153,    2, 0x02,   37 /* Public */,
      14,    1,  154,    2, 0x02,   38 /* Public */,
      16,    1,  157,    2, 0x02,   40 /* Public */,
      18,    1,  160,    2, 0x02,   42 /* Public */,
      20,    1,  163,    2, 0x02,   44 /* Public */,
      21,    4,  166,    2, 0x02,   46 /* Public */,
      24,    0,  175,    2, 0x02,   51 /* Public */,
      25,    0,  176,    2, 0x02,   52 /* Public */,
      26,    1,  177,    2, 0x02,   53 /* Public */,
      28,    1,  180,    2, 0x02,   55 /* Public */,
      30,    1,  183,    2, 0x02,   57 /* Public */,
      32,    0,  186,    2, 0x02,   59 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   11,   12,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Bool, QMetaType::QString,   15,
    QMetaType::Void, QMetaType::Bool,   17,
    QMetaType::Void, QMetaType::Int,   19,
    QMetaType::Void, QMetaType::Bool,   17,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool, QMetaType::Bool, QMetaType::Int,   22,   17,   23,   19,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Bool, QMetaType::QString,   27,
    QMetaType::Bool, QMetaType::Double,   29,
    QMetaType::Bool, QMetaType::Double,   31,
    QMetaType::Bool,

 // properties: name, type, flags, notifyId, revision
      15, QMetaType::QString, 0x00015001, uint(0), 0,
      33, QMetaType::QString, 0x00015001, uint(0), 0,
      27, QMetaType::QString, 0x00015001, uint(1), 0,
      34, QMetaType::Double, 0x00015001, uint(2), 0,
      35, QMetaType::QString, 0x00015001, uint(2), 0,
      36, QMetaType::Int, 0x00015001, uint(0), 0,
      37, QMetaType::QString, 0x00015001, uint(0), 0,
      38, QMetaType::Bool, 0x00015103, uint(3), 0,
      39, QMetaType::Int, 0x00015103, uint(4), 0,
      40, QMetaType::Bool, 0x00015103, uint(5), 0,
      41, QMetaType::Bool, 0x00015001, uint(6), 0,
      42, QMetaType::QString, 0x00015001, uint(6), 0,
      43, QMetaType::Double, 0x00015001, uint(6), 0,
      44, QMetaType::Double, 0x00015001, uint(6), 0,
      45, QMetaType::Double, 0x00015001, uint(6), 0,
      46, QMetaType::Double, 0x00015001, uint(6), 0,
      47, QMetaType::Double, 0x00015001, uint(6), 0,
      48, QMetaType::Double, 0x00015001, uint(6), 0,
      49, QMetaType::Int, 0x00015001, uint(0), 0,
      50, QMetaType::Int, 0x00015001, uint(0), 0,
      51, QMetaType::Bool, 0x00015001, uint(7), 0,
      52, QMetaType::QString, 0x00015001, uint(7), 0,
      53, QMetaType::Double, 0x00015001, uint(7), 0,
      54, QMetaType::Double, 0x00015001, uint(7), 0,
      55, QMetaType::Double, 0x00015001, uint(7), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject GyroConfigBridge::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN16GyroConfigBridgeE.offsetsAndSizes,
    qt_meta_data_ZN16GyroConfigBridgeE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN16GyroConfigBridgeE_t,
        // property 'driver'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'syncMode'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'orientation'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'syncOffsetMs'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'syncOffsetText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'sampleRateHz'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'configPath'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'livePollingEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'livePollingIntervalMs'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'liveAccelOnly'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'liveAvailable'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'liveStatusText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'accelXG'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'accelYG'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'accelZG'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'gyroXDps'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'gyroYDps'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'gyroZDps'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'accelRangeG'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'gyroRangeDps'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'levelReferenceSet'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'levelReferenceText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'levelAccelXG'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'levelAccelYG'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'levelAccelZG'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<GyroConfigBridge, std::true_type>,
        // method 'configChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'orientationChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'syncOffsetMsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'livePollingEnabledChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'livePollingIntervalMsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'liveAccelOnlyChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'liveSampleChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'levelReferenceChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'configSaveFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'refresh'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setDriverName'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setLivePollingEnabled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'setLivePollingIntervalMs'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'setLiveAccelOnly'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'setLivePollingRequest'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'setLevelReference'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'clearLevelReference'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'setOrientation'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setSyncOffsetMs'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'adjustSyncOffsetMs'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'resetSyncOffset'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void GyroConfigBridge::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<GyroConfigBridge *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->configChanged(); break;
        case 1: _t->orientationChanged(); break;
        case 2: _t->syncOffsetMsChanged(); break;
        case 3: _t->livePollingEnabledChanged(); break;
        case 4: _t->livePollingIntervalMsChanged(); break;
        case 5: _t->liveAccelOnlyChanged(); break;
        case 6: _t->liveSampleChanged(); break;
        case 7: _t->levelReferenceChanged(); break;
        case 8: _t->configSaveFinished((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 9: _t->refresh(); break;
        case 10: { bool _r = _t->setDriverName((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 11: _t->setLivePollingEnabled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 12: _t->setLivePollingIntervalMs((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 13: _t->setLiveAccelOnly((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 14: _t->setLivePollingRequest((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4]))); break;
        case 15: { bool _r = _t->setLevelReference();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 16: { bool _r = _t->clearLevelReference();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 17: { bool _r = _t->setOrientation((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 18: { bool _r = _t->setSyncOffsetMs((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 19: { bool _r = _t->adjustSyncOffsetMs((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 20: { bool _r = _t->resetSyncOffset();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (GyroConfigBridge::*)();
            if (_q_method_type _q_method = &GyroConfigBridge::configChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (GyroConfigBridge::*)();
            if (_q_method_type _q_method = &GyroConfigBridge::orientationChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (GyroConfigBridge::*)();
            if (_q_method_type _q_method = &GyroConfigBridge::syncOffsetMsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (GyroConfigBridge::*)();
            if (_q_method_type _q_method = &GyroConfigBridge::livePollingEnabledChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (GyroConfigBridge::*)();
            if (_q_method_type _q_method = &GyroConfigBridge::livePollingIntervalMsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (GyroConfigBridge::*)();
            if (_q_method_type _q_method = &GyroConfigBridge::liveAccelOnlyChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (GyroConfigBridge::*)();
            if (_q_method_type _q_method = &GyroConfigBridge::liveSampleChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (GyroConfigBridge::*)();
            if (_q_method_type _q_method = &GyroConfigBridge::levelReferenceChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (GyroConfigBridge::*)(bool , const QString & );
            if (_q_method_type _q_method = &GyroConfigBridge::configSaveFinished; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->driver(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->syncMode(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->orientation(); break;
        case 3: *reinterpret_cast< double*>(_v) = _t->syncOffsetMs(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->syncOffsetText(); break;
        case 5: *reinterpret_cast< int*>(_v) = _t->sampleRateHz(); break;
        case 6: *reinterpret_cast< QString*>(_v) = _t->configPath(); break;
        case 7: *reinterpret_cast< bool*>(_v) = _t->livePollingEnabled(); break;
        case 8: *reinterpret_cast< int*>(_v) = _t->livePollingIntervalMs(); break;
        case 9: *reinterpret_cast< bool*>(_v) = _t->liveAccelOnly(); break;
        case 10: *reinterpret_cast< bool*>(_v) = _t->liveAvailable(); break;
        case 11: *reinterpret_cast< QString*>(_v) = _t->liveStatusText(); break;
        case 12: *reinterpret_cast< double*>(_v) = _t->accelXG(); break;
        case 13: *reinterpret_cast< double*>(_v) = _t->accelYG(); break;
        case 14: *reinterpret_cast< double*>(_v) = _t->accelZG(); break;
        case 15: *reinterpret_cast< double*>(_v) = _t->gyroXDps(); break;
        case 16: *reinterpret_cast< double*>(_v) = _t->gyroYDps(); break;
        case 17: *reinterpret_cast< double*>(_v) = _t->gyroZDps(); break;
        case 18: *reinterpret_cast< int*>(_v) = _t->accelRangeG(); break;
        case 19: *reinterpret_cast< int*>(_v) = _t->gyroRangeDps(); break;
        case 20: *reinterpret_cast< bool*>(_v) = _t->levelReferenceSet(); break;
        case 21: *reinterpret_cast< QString*>(_v) = _t->levelReferenceText(); break;
        case 22: *reinterpret_cast< double*>(_v) = _t->levelAccelXG(); break;
        case 23: *reinterpret_cast< double*>(_v) = _t->levelAccelYG(); break;
        case 24: *reinterpret_cast< double*>(_v) = _t->levelAccelZG(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 7: _t->setLivePollingEnabled(*reinterpret_cast< bool*>(_v)); break;
        case 8: _t->setLivePollingIntervalMs(*reinterpret_cast< int*>(_v)); break;
        case 9: _t->setLiveAccelOnly(*reinterpret_cast< bool*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *GyroConfigBridge::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GyroConfigBridge::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN16GyroConfigBridgeE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int GyroConfigBridge::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 21)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 21;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 25;
    }
    return _id;
}

// SIGNAL 0
void GyroConfigBridge::configChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void GyroConfigBridge::orientationChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void GyroConfigBridge::syncOffsetMsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void GyroConfigBridge::livePollingEnabledChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void GyroConfigBridge::livePollingIntervalMsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void GyroConfigBridge::liveAccelOnlyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void GyroConfigBridge::liveSampleChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void GyroConfigBridge::levelReferenceChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void GyroConfigBridge::configSaveFinished(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}
QT_WARNING_POP
