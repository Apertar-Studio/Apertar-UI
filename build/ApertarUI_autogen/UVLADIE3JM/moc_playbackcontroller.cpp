/****************************************************************************
** Meta object code from reading C++ file 'playbackcontroller.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/playbackcontroller.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'playbackcontroller.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN18PlaybackControllerE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN18PlaybackControllerE = QtMocHelpers::stringData(
    "PlaybackController",
    "clipChanged",
    "",
    "currentFrameChanged",
    "playingChanged",
    "fpsChanged",
    "statusTextChanged",
    "histogramChanged",
    "clipMetadataChanged",
    "setFps",
    "fps",
    "loadClip",
    "clipPath",
    "togglePlayback",
    "stop",
    "nextFrame",
    "previousFrame",
    "seekToFrame",
    "frameIndex",
    "currentClipName",
    "currentClipPath",
    "frameCount",
    "currentFrameIndex",
    "playing",
    "currentFramePath",
    "frameSource",
    "fastFrameSource",
    "statusText",
    "histogramBins",
    "QVariantList",
    "clipMetadata",
    "QVariantMap",
    "videoClip"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN18PlaybackControllerE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
      13,  118, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   98,    2, 0x06,   14 /* Public */,
       3,    0,   99,    2, 0x06,   15 /* Public */,
       4,    0,  100,    2, 0x06,   16 /* Public */,
       5,    0,  101,    2, 0x06,   17 /* Public */,
       6,    0,  102,    2, 0x06,   18 /* Public */,
       7,    0,  103,    2, 0x06,   19 /* Public */,
       8,    0,  104,    2, 0x06,   20 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       9,    1,  105,    2, 0x0a,   21 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      11,    1,  108,    2, 0x02,   23 /* Public */,
      13,    0,  111,    2, 0x02,   25 /* Public */,
      14,    0,  112,    2, 0x02,   26 /* Public */,
      15,    0,  113,    2, 0x02,   27 /* Public */,
      16,    0,  114,    2, 0x02,   28 /* Public */,
      17,    1,  115,    2, 0x02,   29 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Double,   10,

 // methods: parameters
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   18,

 // properties: name, type, flags, notifyId, revision
      19, QMetaType::QString, 0x00015001, uint(0), 0,
      20, QMetaType::QString, 0x00015001, uint(0), 0,
      21, QMetaType::Int, 0x00015001, uint(0), 0,
      22, QMetaType::Int, 0x00015001, uint(1), 0,
      23, QMetaType::Bool, 0x00015001, uint(2), 0,
      10, QMetaType::Double, 0x00015103, uint(3), 0,
      24, QMetaType::QString, 0x00015001, uint(1), 0,
      25, QMetaType::QString, 0x00015001, uint(1), 0,
      26, QMetaType::QString, 0x00015001, uint(1), 0,
      27, QMetaType::QString, 0x00015001, uint(4), 0,
      28, 0x80000000 | 29, 0x00015009, uint(5), 0,
      30, 0x80000000 | 31, 0x00015009, uint(6), 0,
      32, QMetaType::Bool, 0x00015001, uint(0), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject PlaybackController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN18PlaybackControllerE.offsetsAndSizes,
    qt_meta_data_ZN18PlaybackControllerE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN18PlaybackControllerE_t,
        // property 'currentClipName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentClipPath'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'frameCount'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'currentFrameIndex'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'playing'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'fps'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'currentFramePath'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'frameSource'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'fastFrameSource'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'statusText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'histogramBins'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'clipMetadata'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // property 'videoClip'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PlaybackController, std::true_type>,
        // method 'clipChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'currentFrameChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'playingChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'fpsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'statusTextChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'histogramChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clipMetadataChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setFps'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'loadClip'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'togglePlayback'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stop'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'nextFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'previousFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'seekToFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void PlaybackController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PlaybackController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->clipChanged(); break;
        case 1: _t->currentFrameChanged(); break;
        case 2: _t->playingChanged(); break;
        case 3: _t->fpsChanged(); break;
        case 4: _t->statusTextChanged(); break;
        case 5: _t->histogramChanged(); break;
        case 6: _t->clipMetadataChanged(); break;
        case 7: _t->setFps((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 8: _t->loadClip((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->togglePlayback(); break;
        case 10: _t->stop(); break;
        case 11: _t->nextFrame(); break;
        case 12: _t->previousFrame(); break;
        case 13: _t->seekToFrame((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (PlaybackController::*)();
            if (_q_method_type _q_method = &PlaybackController::clipChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (PlaybackController::*)();
            if (_q_method_type _q_method = &PlaybackController::currentFrameChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (PlaybackController::*)();
            if (_q_method_type _q_method = &PlaybackController::playingChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (PlaybackController::*)();
            if (_q_method_type _q_method = &PlaybackController::fpsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (PlaybackController::*)();
            if (_q_method_type _q_method = &PlaybackController::statusTextChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (PlaybackController::*)();
            if (_q_method_type _q_method = &PlaybackController::histogramChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (PlaybackController::*)();
            if (_q_method_type _q_method = &PlaybackController::clipMetadataChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->currentClipName(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->currentClipPath(); break;
        case 2: *reinterpret_cast< int*>(_v) = _t->frameCount(); break;
        case 3: *reinterpret_cast< int*>(_v) = _t->currentFrameIndex(); break;
        case 4: *reinterpret_cast< bool*>(_v) = _t->playing(); break;
        case 5: *reinterpret_cast< double*>(_v) = _t->fps(); break;
        case 6: *reinterpret_cast< QString*>(_v) = _t->currentFramePath(); break;
        case 7: *reinterpret_cast< QString*>(_v) = _t->frameSource(); break;
        case 8: *reinterpret_cast< QString*>(_v) = _t->fastFrameSource(); break;
        case 9: *reinterpret_cast< QString*>(_v) = _t->statusText(); break;
        case 10: *reinterpret_cast< QVariantList*>(_v) = _t->histogramBins(); break;
        case 11: *reinterpret_cast< QVariantMap*>(_v) = _t->clipMetadata(); break;
        case 12: *reinterpret_cast< bool*>(_v) = _t->videoClip(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 5: _t->setFps(*reinterpret_cast< double*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *PlaybackController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PlaybackController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN18PlaybackControllerE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int PlaybackController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void PlaybackController::clipChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void PlaybackController::currentFrameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void PlaybackController::playingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void PlaybackController::fpsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void PlaybackController::statusTextChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void PlaybackController::histogramChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void PlaybackController::clipMetadataChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
QT_WARNING_POP
