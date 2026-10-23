/****************************************************************************
** Meta object code from reading C++ file 'Ina219Bridge.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/Ina219Bridge.hpp"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Ina219Bridge.hpp' doesn't include <QObject>."
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
struct qt_meta_tag_ZN12Ina219BridgeE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN12Ina219BridgeE = QtMocHelpers::stringData(
    "Ina219Bridge",
    "sensorAvailableChanged",
    "",
    "busVoltageVChanged",
    "currentAChanged",
    "powerWChanged",
    "batteryPercentChanged",
    "updateReadings",
    "sensorAvailable",
    "busVoltageV",
    "currentA",
    "powerW",
    "batteryPercent",
    "voltageText",
    "currentText",
    "powerText",
    "batteryPercentText"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN12Ina219BridgeE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       9,   56, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   50,    2, 0x06,   10 /* Public */,
       3,    0,   51,    2, 0x06,   11 /* Public */,
       4,    0,   52,    2, 0x06,   12 /* Public */,
       5,    0,   53,    2, 0x06,   13 /* Public */,
       6,    0,   54,    2, 0x06,   14 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       7,    0,   55,    2, 0x08,   15 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,

 // properties: name, type, flags, notifyId, revision
       8, QMetaType::Bool, 0x00015001, uint(0), 0,
       9, QMetaType::Double, 0x00015001, uint(1), 0,
      10, QMetaType::Double, 0x00015001, uint(2), 0,
      11, QMetaType::Double, 0x00015001, uint(3), 0,
      12, QMetaType::Int, 0x00015001, uint(4), 0,
      13, QMetaType::QString, 0x00015001, uint(1), 0,
      14, QMetaType::QString, 0x00015001, uint(2), 0,
      15, QMetaType::QString, 0x00015001, uint(3), 0,
      16, QMetaType::QString, 0x00015001, uint(4), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject Ina219Bridge::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN12Ina219BridgeE.offsetsAndSizes,
    qt_meta_data_ZN12Ina219BridgeE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN12Ina219BridgeE_t,
        // property 'sensorAvailable'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'busVoltageV'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'currentA'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'powerW'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'batteryPercent'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'voltageText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'powerText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'batteryPercentText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<Ina219Bridge, std::true_type>,
        // method 'sensorAvailableChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'busVoltageVChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'currentAChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'powerWChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'batteryPercentChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateReadings'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void Ina219Bridge::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Ina219Bridge *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->sensorAvailableChanged(); break;
        case 1: _t->busVoltageVChanged(); break;
        case 2: _t->currentAChanged(); break;
        case 3: _t->powerWChanged(); break;
        case 4: _t->batteryPercentChanged(); break;
        case 5: _t->updateReadings(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (Ina219Bridge::*)();
            if (_q_method_type _q_method = &Ina219Bridge::sensorAvailableChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (Ina219Bridge::*)();
            if (_q_method_type _q_method = &Ina219Bridge::busVoltageVChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (Ina219Bridge::*)();
            if (_q_method_type _q_method = &Ina219Bridge::currentAChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (Ina219Bridge::*)();
            if (_q_method_type _q_method = &Ina219Bridge::powerWChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (Ina219Bridge::*)();
            if (_q_method_type _q_method = &Ina219Bridge::batteryPercentChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->sensorAvailable(); break;
        case 1: *reinterpret_cast< double*>(_v) = _t->busVoltageV(); break;
        case 2: *reinterpret_cast< double*>(_v) = _t->currentA(); break;
        case 3: *reinterpret_cast< double*>(_v) = _t->powerW(); break;
        case 4: *reinterpret_cast< int*>(_v) = _t->batteryPercent(); break;
        case 5: *reinterpret_cast< QString*>(_v) = _t->voltageText(); break;
        case 6: *reinterpret_cast< QString*>(_v) = _t->currentText(); break;
        case 7: *reinterpret_cast< QString*>(_v) = _t->powerText(); break;
        case 8: *reinterpret_cast< QString*>(_v) = _t->batteryPercentText(); break;
        default: break;
        }
    }
}

const QMetaObject *Ina219Bridge::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Ina219Bridge::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN12Ina219BridgeE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Ina219Bridge::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void Ina219Bridge::sensorAvailableChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void Ina219Bridge::busVoltageVChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void Ina219Bridge::currentAChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void Ina219Bridge::powerWChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void Ina219Bridge::batteryPercentChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
