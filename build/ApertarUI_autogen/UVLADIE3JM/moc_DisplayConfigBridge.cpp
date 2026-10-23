/****************************************************************************
** Meta object code from reading C++ file 'DisplayConfigBridge.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/DisplayConfigBridge.hpp"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DisplayConfigBridge.hpp' doesn't include <QObject>."
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
struct qt_meta_tag_ZN19DisplayConfigBridgeE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN19DisplayConfigBridgeE = QtMocHelpers::stringData(
    "DisplayConfigBridge",
    "uiWidth",
    "uiHeight",
    "uiLayout",
    "uiRotationDegrees",
    "controlsOpacity",
    "zoomToggleFactor",
    "windowed",
    "hdmiExternalOnly",
    "squareLayout",
    "landscapeLayout",
    "compactLandscapeLayout",
    "regularLandscapeLayout",
    "portraitLayout",
    "compactPortraitLayout",
    "configPath"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN19DisplayConfigBridgeE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
      15,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // properties: name, type, flags, notifyId, revision
       1, QMetaType::Int, 0x00015401, uint(-1), 0,
       2, QMetaType::Int, 0x00015401, uint(-1), 0,
       3, QMetaType::QString, 0x00015401, uint(-1), 0,
       4, QMetaType::Int, 0x00015401, uint(-1), 0,
       5, QMetaType::Double, 0x00015401, uint(-1), 0,
       6, QMetaType::Double, 0x00015401, uint(-1), 0,
       7, QMetaType::Bool, 0x00015401, uint(-1), 0,
       8, QMetaType::Bool, 0x00015401, uint(-1), 0,
       9, QMetaType::Bool, 0x00015401, uint(-1), 0,
      10, QMetaType::Bool, 0x00015401, uint(-1), 0,
      11, QMetaType::Bool, 0x00015401, uint(-1), 0,
      12, QMetaType::Bool, 0x00015401, uint(-1), 0,
      13, QMetaType::Bool, 0x00015401, uint(-1), 0,
      14, QMetaType::Bool, 0x00015401, uint(-1), 0,
      15, QMetaType::QString, 0x00015401, uint(-1), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject DisplayConfigBridge::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN19DisplayConfigBridgeE.offsetsAndSizes,
    qt_meta_data_ZN19DisplayConfigBridgeE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN19DisplayConfigBridgeE_t,
        // property 'uiWidth'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'uiHeight'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'uiLayout'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'uiRotationDegrees'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'controlsOpacity'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'zoomToggleFactor'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'windowed'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'hdmiExternalOnly'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'squareLayout'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'landscapeLayout'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'compactLandscapeLayout'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'regularLandscapeLayout'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'portraitLayout'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'compactPortraitLayout'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'configPath'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<DisplayConfigBridge, std::true_type>
    >,
    nullptr
} };

void DisplayConfigBridge::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DisplayConfigBridge *>(_o);
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< int*>(_v) = _t->uiWidth(); break;
        case 1: *reinterpret_cast< int*>(_v) = _t->uiHeight(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->uiLayout(); break;
        case 3: *reinterpret_cast< int*>(_v) = _t->uiRotationDegrees(); break;
        case 4: *reinterpret_cast< double*>(_v) = _t->controlsOpacity(); break;
        case 5: *reinterpret_cast< double*>(_v) = _t->zoomToggleFactor(); break;
        case 6: *reinterpret_cast< bool*>(_v) = _t->windowed(); break;
        case 7: *reinterpret_cast< bool*>(_v) = _t->hdmiExternalOnly(); break;
        case 8: *reinterpret_cast< bool*>(_v) = _t->squareLayout(); break;
        case 9: *reinterpret_cast< bool*>(_v) = _t->landscapeLayout(); break;
        case 10: *reinterpret_cast< bool*>(_v) = _t->compactLandscapeLayout(); break;
        case 11: *reinterpret_cast< bool*>(_v) = _t->regularLandscapeLayout(); break;
        case 12: *reinterpret_cast< bool*>(_v) = _t->portraitLayout(); break;
        case 13: *reinterpret_cast< bool*>(_v) = _t->compactPortraitLayout(); break;
        case 14: *reinterpret_cast< QString*>(_v) = _t->configPath(); break;
        default: break;
        }
    }
}

const QMetaObject *DisplayConfigBridge::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DisplayConfigBridge::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN19DisplayConfigBridgeE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int DisplayConfigBridge::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}
QT_WARNING_POP
