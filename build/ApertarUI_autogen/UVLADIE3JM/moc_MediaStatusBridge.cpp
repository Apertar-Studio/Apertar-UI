/****************************************************************************
** Meta object code from reading C++ file 'MediaStatusBridge.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/MediaStatusBridge.hpp"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MediaStatusBridge.hpp' doesn't include <QObject>."
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
struct qt_meta_tag_ZN17MediaStatusBridgeE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN17MediaStatusBridgeE = QtMocHelpers::stringData(
    "MediaStatusBridge",
    "mediaMountedChanged",
    "",
    "statusTextChanged",
    "lastActionErrorChanged",
    "frameSizeMBChanged",
    "fpsChanged",
    "freeBytesChanged",
    "totalBytesChanged",
    "remainingSecondsChanged",
    "remainingMinutesTextChanged",
    "remainingStillsChanged",
    "writeSpeedStateChanged",
    "mediaInfoChanged",
    "mountableDrivesChanged",
    "updateStatus",
    "pollCoreWriteSpeedTest",
    "ejectMedia",
    "formatMedia",
    "filesystemType",
    "startWriteSpeedTest",
    "refreshMountableDrives",
    "setDriveDiscoveryActive",
    "active",
    "mountDrive",
    "devicePath",
    "mountDriveByDisplayName",
    "displayName",
    "mediaMounted",
    "mountPath",
    "statusText",
    "lastActionError",
    "frameSizeMB",
    "fps",
    "freeBytes",
    "totalBytes",
    "remainingSeconds",
    "remainingMinutesText",
    "remainingStills",
    "remainingStillsText",
    "writeSpeedTestRunning",
    "lastWriteSpeedMBps",
    "writeSpeedResultText",
    "writeSpeedDetailText",
    "writeSpeedError",
    "mediaTypeLabel",
    "mediaPromptLabel",
    "mountableDrives",
    "QVariantList",
    "mountableDriveOptions"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN17MediaStatusBridgeE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      22,   14, // methods
      21,  176, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      13,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  146,    2, 0x06,   22 /* Public */,
       3,    0,  147,    2, 0x06,   23 /* Public */,
       4,    0,  148,    2, 0x06,   24 /* Public */,
       5,    0,  149,    2, 0x06,   25 /* Public */,
       6,    0,  150,    2, 0x06,   26 /* Public */,
       7,    0,  151,    2, 0x06,   27 /* Public */,
       8,    0,  152,    2, 0x06,   28 /* Public */,
       9,    0,  153,    2, 0x06,   29 /* Public */,
      10,    0,  154,    2, 0x06,   30 /* Public */,
      11,    0,  155,    2, 0x06,   31 /* Public */,
      12,    0,  156,    2, 0x06,   32 /* Public */,
      13,    0,  157,    2, 0x06,   33 /* Public */,
      14,    0,  158,    2, 0x06,   34 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      15,    0,  159,    2, 0x08,   35 /* Private */,
      16,    0,  160,    2, 0x08,   36 /* Private */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      17,    0,  161,    2, 0x02,   37 /* Public */,
      18,    1,  162,    2, 0x02,   38 /* Public */,
      20,    0,  165,    2, 0x02,   40 /* Public */,
      21,    0,  166,    2, 0x02,   41 /* Public */,
      22,    1,  167,    2, 0x02,   42 /* Public */,
      24,    1,  170,    2, 0x02,   44 /* Public */,
      26,    1,  173,    2, 0x02,   46 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Bool,
    QMetaType::Bool, QMetaType::QString,   19,
    QMetaType::Bool,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   23,
    QMetaType::Bool, QMetaType::QString,   25,
    QMetaType::Bool, QMetaType::QString,   27,

 // properties: name, type, flags, notifyId, revision
      28, QMetaType::Bool, 0x00015001, uint(0), 0,
      29, QMetaType::QString, 0x00015401, uint(-1), 0,
      30, QMetaType::QString, 0x00015001, uint(1), 0,
      31, QMetaType::QString, 0x00015001, uint(2), 0,
      32, QMetaType::Double, 0x00015103, uint(3), 0,
      33, QMetaType::Double, 0x00015103, uint(4), 0,
      34, QMetaType::ULongLong, 0x00015001, uint(5), 0,
      35, QMetaType::ULongLong, 0x00015001, uint(6), 0,
      36, QMetaType::LongLong, 0x00015001, uint(7), 0,
      37, QMetaType::QString, 0x00015001, uint(8), 0,
      38, QMetaType::ULongLong, 0x00015001, uint(9), 0,
      39, QMetaType::QString, 0x00015001, uint(9), 0,
      40, QMetaType::Bool, 0x00015001, uint(10), 0,
      41, QMetaType::Double, 0x00015001, uint(10), 0,
      42, QMetaType::QString, 0x00015001, uint(10), 0,
      43, QMetaType::QString, 0x00015001, uint(10), 0,
      44, QMetaType::QString, 0x00015001, uint(10), 0,
      45, QMetaType::QString, 0x00015001, uint(11), 0,
      46, QMetaType::QString, 0x00015001, uint(11), 0,
      47, 0x80000000 | 48, 0x00015009, uint(12), 0,
      49, QMetaType::QStringList, 0x00015001, uint(12), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject MediaStatusBridge::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN17MediaStatusBridgeE.offsetsAndSizes,
    qt_meta_data_ZN17MediaStatusBridgeE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN17MediaStatusBridgeE_t,
        // property 'mediaMounted'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'mountPath'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'statusText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'lastActionError'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'frameSizeMB'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'fps'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'freeBytes'
        QtPrivate::TypeAndForceComplete<qulonglong, std::true_type>,
        // property 'totalBytes'
        QtPrivate::TypeAndForceComplete<qulonglong, std::true_type>,
        // property 'remainingSeconds'
        QtPrivate::TypeAndForceComplete<qint64, std::true_type>,
        // property 'remainingMinutesText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'remainingStills'
        QtPrivate::TypeAndForceComplete<qulonglong, std::true_type>,
        // property 'remainingStillsText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'writeSpeedTestRunning'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'lastWriteSpeedMBps'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'writeSpeedResultText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'writeSpeedDetailText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'writeSpeedError'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'mediaTypeLabel'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'mediaPromptLabel'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'mountableDrives'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'mountableDriveOptions'
        QtPrivate::TypeAndForceComplete<QStringList, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MediaStatusBridge, std::true_type>,
        // method 'mediaMountedChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'statusTextChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'lastActionErrorChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'frameSizeMBChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'fpsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'freeBytesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'totalBytesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'remainingSecondsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'remainingMinutesTextChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'remainingStillsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'writeSpeedStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'mediaInfoChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'mountableDrivesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'pollCoreWriteSpeedTest'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'ejectMedia'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'formatMedia'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'startWriteSpeedTest'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'refreshMountableDrives'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setDriveDiscoveryActive'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'mountDrive'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'mountDriveByDisplayName'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void MediaStatusBridge::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MediaStatusBridge *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->mediaMountedChanged(); break;
        case 1: _t->statusTextChanged(); break;
        case 2: _t->lastActionErrorChanged(); break;
        case 3: _t->frameSizeMBChanged(); break;
        case 4: _t->fpsChanged(); break;
        case 5: _t->freeBytesChanged(); break;
        case 6: _t->totalBytesChanged(); break;
        case 7: _t->remainingSecondsChanged(); break;
        case 8: _t->remainingMinutesTextChanged(); break;
        case 9: _t->remainingStillsChanged(); break;
        case 10: _t->writeSpeedStateChanged(); break;
        case 11: _t->mediaInfoChanged(); break;
        case 12: _t->mountableDrivesChanged(); break;
        case 13: _t->updateStatus(); break;
        case 14: _t->pollCoreWriteSpeedTest(); break;
        case 15: { bool _r = _t->ejectMedia();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 16: { bool _r = _t->formatMedia((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 17: { bool _r = _t->startWriteSpeedTest();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 18: _t->refreshMountableDrives(); break;
        case 19: _t->setDriveDiscoveryActive((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 20: { bool _r = _t->mountDrive((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 21: { bool _r = _t->mountDriveByDisplayName((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (MediaStatusBridge::*)();
            if (_q_method_type _q_method = &MediaStatusBridge::mediaMountedChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (MediaStatusBridge::*)();
            if (_q_method_type _q_method = &MediaStatusBridge::statusTextChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (MediaStatusBridge::*)();
            if (_q_method_type _q_method = &MediaStatusBridge::lastActionErrorChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (MediaStatusBridge::*)();
            if (_q_method_type _q_method = &MediaStatusBridge::frameSizeMBChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (MediaStatusBridge::*)();
            if (_q_method_type _q_method = &MediaStatusBridge::fpsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (MediaStatusBridge::*)();
            if (_q_method_type _q_method = &MediaStatusBridge::freeBytesChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (MediaStatusBridge::*)();
            if (_q_method_type _q_method = &MediaStatusBridge::totalBytesChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (MediaStatusBridge::*)();
            if (_q_method_type _q_method = &MediaStatusBridge::remainingSecondsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (MediaStatusBridge::*)();
            if (_q_method_type _q_method = &MediaStatusBridge::remainingMinutesTextChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (MediaStatusBridge::*)();
            if (_q_method_type _q_method = &MediaStatusBridge::remainingStillsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (MediaStatusBridge::*)();
            if (_q_method_type _q_method = &MediaStatusBridge::writeSpeedStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _q_method_type = void (MediaStatusBridge::*)();
            if (_q_method_type _q_method = &MediaStatusBridge::mediaInfoChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _q_method_type = void (MediaStatusBridge::*)();
            if (_q_method_type _q_method = &MediaStatusBridge::mountableDrivesChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->mediaMounted(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->mountPath(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->statusText(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->lastActionError(); break;
        case 4: *reinterpret_cast< double*>(_v) = _t->frameSizeMB(); break;
        case 5: *reinterpret_cast< double*>(_v) = _t->fps(); break;
        case 6: *reinterpret_cast< qulonglong*>(_v) = _t->freeBytes(); break;
        case 7: *reinterpret_cast< qulonglong*>(_v) = _t->totalBytes(); break;
        case 8: *reinterpret_cast< qint64*>(_v) = _t->remainingSeconds(); break;
        case 9: *reinterpret_cast< QString*>(_v) = _t->remainingMinutesText(); break;
        case 10: *reinterpret_cast< qulonglong*>(_v) = _t->remainingStills(); break;
        case 11: *reinterpret_cast< QString*>(_v) = _t->remainingStillsText(); break;
        case 12: *reinterpret_cast< bool*>(_v) = _t->writeSpeedTestRunning(); break;
        case 13: *reinterpret_cast< double*>(_v) = _t->lastWriteSpeedMBps(); break;
        case 14: *reinterpret_cast< QString*>(_v) = _t->writeSpeedResultText(); break;
        case 15: *reinterpret_cast< QString*>(_v) = _t->writeSpeedDetailText(); break;
        case 16: *reinterpret_cast< QString*>(_v) = _t->writeSpeedError(); break;
        case 17: *reinterpret_cast< QString*>(_v) = _t->mediaTypeLabel(); break;
        case 18: *reinterpret_cast< QString*>(_v) = _t->mediaPromptLabel(); break;
        case 19: *reinterpret_cast< QVariantList*>(_v) = _t->mountableDrives(); break;
        case 20: *reinterpret_cast< QStringList*>(_v) = _t->mountableDriveOptions(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 4: _t->setFrameSizeMB(*reinterpret_cast< double*>(_v)); break;
        case 5: _t->setFps(*reinterpret_cast< double*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *MediaStatusBridge::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MediaStatusBridge::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN17MediaStatusBridgeE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MediaStatusBridge::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 22)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 22;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 22)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 22;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    return _id;
}

// SIGNAL 0
void MediaStatusBridge::mediaMountedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MediaStatusBridge::statusTextChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MediaStatusBridge::lastActionErrorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void MediaStatusBridge::frameSizeMBChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void MediaStatusBridge::fpsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void MediaStatusBridge::freeBytesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void MediaStatusBridge::totalBytesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void MediaStatusBridge::remainingSecondsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void MediaStatusBridge::remainingMinutesTextChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void MediaStatusBridge::remainingStillsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void MediaStatusBridge::writeSpeedStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void MediaStatusBridge::mediaInfoChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void MediaStatusBridge::mountableDrivesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}
QT_WARNING_POP
