/****************************************************************************
** Meta object code from reading C++ file 'DeviceInfoBridge.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/DeviceInfoBridge.hpp"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DeviceInfoBridge.hpp' doesn't include <QObject>."
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
struct qt_meta_tag_ZN16DeviceInfoBridgeE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN16DeviceInfoBridgeE = QtMocHelpers::stringData(
    "DeviceInfoBridge",
    "infoChanged",
    "",
    "refreshDynamicInfo",
    "setDynamicPollingEnabled",
    "enabled",
    "sensorName",
    "backendName",
    "piModel",
    "serialNumber",
    "osVersion",
    "kernelVersion",
    "uptimeText",
    "ipAddress",
    "hostname",
    "ramTotalText",
    "ramAvailableText",
    "ramUsedText",
    "systemStorageTotalText",
    "systemStorageFreeText",
    "systemStorageMountPoint"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN16DeviceInfoBridgeE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
      15,   37, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   32,    2, 0x06,   16 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       3,    0,   33,    2, 0x08,   17 /* Private */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       4,    1,   34,    2, 0x02,   18 /* Public */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::Bool,    5,

 // properties: name, type, flags, notifyId, revision
       6, QMetaType::QString, 0x00015001, uint(0), 0,
       7, QMetaType::QString, 0x00015401, uint(-1), 0,
       8, QMetaType::QString, 0x00015001, uint(0), 0,
       9, QMetaType::QString, 0x00015001, uint(0), 0,
      10, QMetaType::QString, 0x00015001, uint(0), 0,
      11, QMetaType::QString, 0x00015001, uint(0), 0,
      12, QMetaType::QString, 0x00015001, uint(0), 0,
      13, QMetaType::QString, 0x00015001, uint(0), 0,
      14, QMetaType::QString, 0x00015001, uint(0), 0,
      15, QMetaType::QString, 0x00015001, uint(0), 0,
      16, QMetaType::QString, 0x00015001, uint(0), 0,
      17, QMetaType::QString, 0x00015001, uint(0), 0,
      18, QMetaType::QString, 0x00015001, uint(0), 0,
      19, QMetaType::QString, 0x00015001, uint(0), 0,
      20, QMetaType::QString, 0x00015001, uint(0), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject DeviceInfoBridge::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN16DeviceInfoBridgeE.offsetsAndSizes,
    qt_meta_data_ZN16DeviceInfoBridgeE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN16DeviceInfoBridgeE_t,
        // property 'sensorName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'backendName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'piModel'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'serialNumber'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'osVersion'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'kernelVersion'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'uptimeText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'ipAddress'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'hostname'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'ramTotalText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'ramAvailableText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'ramUsedText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'systemStorageTotalText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'systemStorageFreeText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'systemStorageMountPoint'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<DeviceInfoBridge, std::true_type>,
        // method 'infoChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshDynamicInfo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setDynamicPollingEnabled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void DeviceInfoBridge::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DeviceInfoBridge *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->infoChanged(); break;
        case 1: _t->refreshDynamicInfo(); break;
        case 2: _t->setDynamicPollingEnabled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (DeviceInfoBridge::*)();
            if (_q_method_type _q_method = &DeviceInfoBridge::infoChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->sensorName(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->backendName(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->piModel(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->serialNumber(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->osVersion(); break;
        case 5: *reinterpret_cast< QString*>(_v) = _t->kernelVersion(); break;
        case 6: *reinterpret_cast< QString*>(_v) = _t->uptimeText(); break;
        case 7: *reinterpret_cast< QString*>(_v) = _t->ipAddress(); break;
        case 8: *reinterpret_cast< QString*>(_v) = _t->hostname(); break;
        case 9: *reinterpret_cast< QString*>(_v) = _t->ramTotalText(); break;
        case 10: *reinterpret_cast< QString*>(_v) = _t->ramAvailableText(); break;
        case 11: *reinterpret_cast< QString*>(_v) = _t->ramUsedText(); break;
        case 12: *reinterpret_cast< QString*>(_v) = _t->systemStorageTotalText(); break;
        case 13: *reinterpret_cast< QString*>(_v) = _t->systemStorageFreeText(); break;
        case 14: *reinterpret_cast< QString*>(_v) = _t->systemStorageMountPoint(); break;
        default: break;
        }
    }
}

const QMetaObject *DeviceInfoBridge::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DeviceInfoBridge::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN16DeviceInfoBridgeE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int DeviceInfoBridge::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void DeviceInfoBridge::infoChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
