/****************************************************************************
** Meta object code from reading C++ file 'AudioDeviceBridge.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/AudioDeviceBridge.hpp"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AudioDeviceBridge.hpp' doesn't include <QObject>."
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
struct qt_meta_tag_ZN17AudioDeviceBridgeE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN17AudioDeviceBridgeE = QtMocHelpers::stringData(
    "AudioDeviceBridge",
    "inputDeviceOptionsChanged",
    "",
    "outputDeviceOptionsChanged",
    "captureOptionsChanged",
    "capabilityProbeActiveChanged",
    "active",
    "setPollingEnabled",
    "enabled",
    "refresh",
    "refreshCaptureCapabilities",
    "selectSampleFormat",
    "label",
    "selectSampleRate",
    "selectChannels",
    "resolvedInputDeviceLabel",
    "selection",
    "resolvedOutputDeviceLabel",
    "normalizeInputSelection",
    "normalizeOutputSelection",
    "inputDeviceIdForLabel",
    "outputDeviceIdForLabel",
    "inputDeviceOptions",
    "outputDeviceOptions",
    "hasInputDevices",
    "hasOutputDevices",
    "sampleFormatOptions",
    "sampleRateOptions",
    "channelOptions",
    "sampleFormatLabel",
    "sampleRateLabel",
    "channelsLabel",
    "captureDescription",
    "capabilitiesBusy"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN17AudioDeviceBridgeE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
      12,  148, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  110,    2, 0x06,   13 /* Public */,
       3,    0,  111,    2, 0x06,   14 /* Public */,
       4,    0,  112,    2, 0x06,   15 /* Public */,
       5,    1,  113,    2, 0x06,   16 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       7,    1,  116,    2, 0x02,   18 /* Public */,
       9,    0,  119,    2, 0x02,   20 /* Public */,
      10,    0,  120,    2, 0x02,   21 /* Public */,
      11,    1,  121,    2, 0x02,   22 /* Public */,
      13,    1,  124,    2, 0x02,   24 /* Public */,
      14,    1,  127,    2, 0x02,   26 /* Public */,
      15,    1,  130,    2, 0x102,   28 /* Public | MethodIsConst  */,
      17,    1,  133,    2, 0x102,   30 /* Public | MethodIsConst  */,
      18,    1,  136,    2, 0x102,   32 /* Public | MethodIsConst  */,
      19,    1,  139,    2, 0x102,   34 /* Public | MethodIsConst  */,
      20,    1,  142,    2, 0x102,   36 /* Public | MethodIsConst  */,
      21,    1,  145,    2, 0x102,   38 /* Public | MethodIsConst  */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    6,

 // methods: parameters
    QMetaType::Void, QMetaType::Bool,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::QString, QMetaType::QString,   16,
    QMetaType::QString, QMetaType::QString,   16,
    QMetaType::QString, QMetaType::QString,   16,
    QMetaType::QString, QMetaType::QString,   16,
    QMetaType::QString, QMetaType::QString,   12,
    QMetaType::QString, QMetaType::QString,   12,

 // properties: name, type, flags, notifyId, revision
      22, QMetaType::QStringList, 0x00015001, uint(0), 0,
      23, QMetaType::QStringList, 0x00015001, uint(1), 0,
      24, QMetaType::Bool, 0x00015001, uint(0), 0,
      25, QMetaType::Bool, 0x00015001, uint(1), 0,
      26, QMetaType::QStringList, 0x00015001, uint(2), 0,
      27, QMetaType::QStringList, 0x00015001, uint(2), 0,
      28, QMetaType::QStringList, 0x00015001, uint(2), 0,
      29, QMetaType::QString, 0x00015001, uint(2), 0,
      30, QMetaType::QString, 0x00015001, uint(2), 0,
      31, QMetaType::QString, 0x00015001, uint(2), 0,
      32, QMetaType::QString, 0x00015001, uint(2), 0,
      33, QMetaType::Bool, 0x00015001, uint(2), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject AudioDeviceBridge::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN17AudioDeviceBridgeE.offsetsAndSizes,
    qt_meta_data_ZN17AudioDeviceBridgeE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN17AudioDeviceBridgeE_t,
        // property 'inputDeviceOptions'
        QtPrivate::TypeAndForceComplete<QStringList, std::true_type>,
        // property 'outputDeviceOptions'
        QtPrivate::TypeAndForceComplete<QStringList, std::true_type>,
        // property 'hasInputDevices'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'hasOutputDevices'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'sampleFormatOptions'
        QtPrivate::TypeAndForceComplete<QStringList, std::true_type>,
        // property 'sampleRateOptions'
        QtPrivate::TypeAndForceComplete<QStringList, std::true_type>,
        // property 'channelOptions'
        QtPrivate::TypeAndForceComplete<QStringList, std::true_type>,
        // property 'sampleFormatLabel'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'sampleRateLabel'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'channelsLabel'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'captureDescription'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'capabilitiesBusy'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AudioDeviceBridge, std::true_type>,
        // method 'inputDeviceOptionsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'outputDeviceOptionsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'captureOptionsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'capabilityProbeActiveChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'setPollingEnabled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'refresh'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshCaptureCapabilities'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'selectSampleFormat'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'selectSampleRate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'selectChannels'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'resolvedInputDeviceLabel'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'resolvedOutputDeviceLabel'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'normalizeInputSelection'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'normalizeOutputSelection'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'inputDeviceIdForLabel'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'outputDeviceIdForLabel'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void AudioDeviceBridge::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AudioDeviceBridge *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->inputDeviceOptionsChanged(); break;
        case 1: _t->outputDeviceOptionsChanged(); break;
        case 2: _t->captureOptionsChanged(); break;
        case 3: _t->capabilityProbeActiveChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 4: _t->setPollingEnabled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 5: _t->refresh(); break;
        case 6: _t->refreshCaptureCapabilities(); break;
        case 7: _t->selectSampleFormat((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->selectSampleRate((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->selectChannels((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: { QString _r = _t->resolvedInputDeviceLabel((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 11: { QString _r = _t->resolvedOutputDeviceLabel((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 12: { QString _r = _t->normalizeInputSelection((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 13: { QString _r = _t->normalizeOutputSelection((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 14: { QString _r = _t->inputDeviceIdForLabel((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 15: { QString _r = _t->outputDeviceIdForLabel((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (AudioDeviceBridge::*)();
            if (_q_method_type _q_method = &AudioDeviceBridge::inputDeviceOptionsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (AudioDeviceBridge::*)();
            if (_q_method_type _q_method = &AudioDeviceBridge::outputDeviceOptionsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (AudioDeviceBridge::*)();
            if (_q_method_type _q_method = &AudioDeviceBridge::captureOptionsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (AudioDeviceBridge::*)(bool );
            if (_q_method_type _q_method = &AudioDeviceBridge::capabilityProbeActiveChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QStringList*>(_v) = _t->inputDeviceOptions(); break;
        case 1: *reinterpret_cast< QStringList*>(_v) = _t->outputDeviceOptions(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->hasInputDevices(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->hasOutputDevices(); break;
        case 4: *reinterpret_cast< QStringList*>(_v) = _t->sampleFormatOptions(); break;
        case 5: *reinterpret_cast< QStringList*>(_v) = _t->sampleRateOptions(); break;
        case 6: *reinterpret_cast< QStringList*>(_v) = _t->channelOptions(); break;
        case 7: *reinterpret_cast< QString*>(_v) = _t->sampleFormatLabel(); break;
        case 8: *reinterpret_cast< QString*>(_v) = _t->sampleRateLabel(); break;
        case 9: *reinterpret_cast< QString*>(_v) = _t->channelsLabel(); break;
        case 10: *reinterpret_cast< QString*>(_v) = _t->captureDescription(); break;
        case 11: *reinterpret_cast< bool*>(_v) = _t->capabilitiesBusy(); break;
        default: break;
        }
    }
}

const QMetaObject *AudioDeviceBridge::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AudioDeviceBridge::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN17AudioDeviceBridgeE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AudioDeviceBridge::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 16;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void AudioDeviceBridge::inputDeviceOptionsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AudioDeviceBridge::outputDeviceOptionsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AudioDeviceBridge::captureOptionsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void AudioDeviceBridge::capabilityProbeActiveChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
