// SPDX-License-Identifier: GPL-2.0
#include <QQmlEngine>
#include <QQuickItem>

#ifdef MAP_SUPPORT
#include "map-widget/qmlmapwidgethelper.h"
#include "qt-models/maplocationmodel.h"
#endif

#include "stats/statsview.h"
#include "core/devicedetails.h"
#include "core/errorhelper.h"
#include "core/globals.h"
#include "core/qt-gui.h"
#include "core/settings/qPref.h"

#ifdef SUBSURFACE_MOBILE
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "mobile-widgets/themeinterface.h"
#include "mobile-widgets/qmlmanager.h"
#include "mobile-widgets/qmlinterface.h"
#include "mobile-widgets/statsmanager.h"
#include "stats/chartlistmodel.h"
#include "qt-models/divesummarymodel.h"
#include "qt-models/messagehandlermodel.h"
#include "qt-models/mobilelistmodel.h"
#include "profile-widget/qmlprofile.h"
#include "core/downloadfromdcthread.h"
#include "core/subsurfacestartup.h" // for testqml
#include "core/metrics.h"
#include "qt-models/diveimportedmodel.h"
#else
#include "desktop-widgets/mainwindow.h"
#endif

#if defined(Q_OS_ANDROID)
QString getAndroidHWInfo(); // from android.cpp
#include <QApplication>
#include <QFontDatabase>

#undef stdin
#undef stdout
#undef stderr
FILE *stdin = &__sF[0];
FILE *stdout = &__sF[1];
FILE *stderr = &__sF[2];

#endif /* Q_OS_ANDROID */

QObject *qqWindowObject = NULL;

// Forward declaration
static void register_qml_types(QQmlEngine *);
static void register_meta_types();

#ifdef SUBSURFACE_MOBILE
#include <QtPlugin>
Q_IMPORT_PLUGIN(KirigamiPlugin)
#endif

void init_ui()
{
	init_qt_late();
	register_meta_types();
#ifndef SUBSURFACE_MOBILE
	register_qml_types(NULL);

	MainWindow *window = make_global<MainWindow>();
	window->setTitle();
#endif // SUBSURFACE_MOBILE
}

void exit_ui()
{
	free_globals();
}

#ifdef SUBSURFACE_MOBILE
void run_mobile_ui(double initial_font_size)
{
	QScreen *appScreen = QApplication::screens().at(0);
	int availableScreenWidth = appScreen->availableSize().width();
	QQmlApplicationEngine engine;
	QQmlContext *ctxt = engine.rootContext();

	// Register qml interface classes
	QMLInterface::setup(ctxt);
	register_qml_types(&engine);
#if defined(__APPLE__) && !defined(Q_OS_IOS)
	// when running the QML UI on a Mac the deployment of the QML Components seems
	// to fail and the search path for the components is rather odd - simply the
	// same directory the executable was started from <bundle>/Contents/MacOS/
	// To work around this we need to manually copy the components at install time
	// to Contents/Frameworks/qml and make sure that we add the correct import path
	const QStringList importPathList = engine.importPathList();
	for (QString importPath: importPathList) {
		if (importPath.contains("MacOS"))
			engine.addImportPath(importPath.replace("MacOS", "Frameworks"));
	}
#endif // __APPLE__ not Q_OS_IOS
	// this is frustrating, but we appear to need different import paths on different OSs
	engine.addImportPath(":");
	engine.addImportPath("qrc://imports");
	ctxt->setContextProperty("vendorList", vendorList);
	ctxt->setContextProperty("swipeModel", MobileModels::instance()->swipeModel());
	ctxt->setContextProperty("diveModel", MobileModels::instance()->listModel());
	set_non_bt_addresses();

	// we need to setup the initial font size before the QML UI is instantiated
	ThemeInterface *themeInterface = ThemeInterface::instance();
	themeInterface->setInitialFontSize(initial_font_size);

	ctxt->setContextProperty("connectionListModel", &connectionListModel);
	ctxt->setContextProperty("logModel", MessageHandlerModel::self());
	ctxt->setContextProperty("subsurfaceTheme", themeInterface);

	qmlRegisterUncreatableType<QMLManager>("org.subsurfacedivelog.mobile",1,0,"ExportType","Enum is not a type");

#ifdef SUBSURFACE_MOBILE_DESKTOP
	if (!testqml.empty()) {
		QString fileLoad(testqml.c_str());
		fileLoad += "/main.qml";
		engine.load(QUrl(fileLoad));
	} else {
		engine.load(QUrl(QStringLiteral("qrc:///qml/main.qml")));
	}
#else
	engine.load(QUrl(QStringLiteral("qrc:///qml/main.qml")));
#endif
	report_info("loaded main.qml");
	qqWindowObject = engine.rootObjects().value(0);
	if (!qqWindowObject) {
		report_info("can't create window object");
		exit(1);
	}
	QQuickWindow *qml_window = qobject_cast<QQuickWindow *>(qqWindowObject);
	qml_window->setIcon(QIcon(":subsurface-mobile-icon"));
	report_info("qqwindow devicePixelRatio %f %f", qml_window->devicePixelRatio(), qml_window->screen()->devicePixelRatio());
	QScreen *screen = qml_window->screen();
	int qmlWW = qml_window->width();
	int qmlSW = screen->size().width();
	report_info("qml_window reports width as %d associated screen width %d Qt screen reports width as %d", qmlWW, qmlSW, availableScreenWidth);
	QObject::connect(qml_window, &QQuickWindow::screenChanged, QMLManager::instance(), &QMLManager::screenChanged);
	QMLManager *manager = QMLManager::instance();

	manager->setDevicePixelRatio(qml_window->devicePixelRatio(), qml_window->screen());
	manager->qmlWindow = qqWindowObject;
	manager->screenChanged(screen);
	report_info("qqwindow screen has ldpi/pdpi %f %f", screen->logicalDotsPerInch(), screen->physicalDotsPerInch());
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
	int width = 800;
	int height = 1200;
	if (qEnvironmentVariableIsSet("SUBSURFACE_MOBILE_WIDTH")) {
		bool ok;
		int width_override = qEnvironmentVariableIntValue("SUBSURFACE_MOBILE_WIDTH", &ok);
		if (ok) {
			width = width_override;
			report_info("overriding window width: %d", width);
		}
	}
	if (qEnvironmentVariableIsSet("SUBSURFACE_MOBILE_HEIGHT")) {
		bool ok;
		int height_override = qEnvironmentVariableIntValue("SUBSURFACE_MOBILE_HEIGHT", &ok);
		if (ok) {
			height = height_override;
			report_info("overriding window height: %d", height);
		}
	}
	qml_window->setHeight(height);
	qml_window->setWidth(width);
#endif // not Q_OS_ANDROID and not Q_OS_IOS
	qml_window->show();
	qApp->exec();
}
#else // SUBSURFACE_MOBILE
// just run the desktop UI
void run_ui()
{
	MainWindow::instance()->show();
	qApp->exec();
}
#endif // SUBSURFACE_MOBILE

Q_DECLARE_METATYPE(duration_t)
static void register_meta_types()
{
	qRegisterMetaType<duration_t>();
	qRegisterMetaType<DeviceDetails>();
}

template <typename T>
static void register_qml_type(const char *name)
{
	if(qmlRegisterType<T>("org.subsurfacedivelog.mobile", 1, 0, name) < 0)
		qWarning("ERROR: Cannot register %s, QML will not work!!", name);
}

static void register_qml_types(QQmlEngine *engine)
{
	// register qPref*
	qPref::registerQML(engine);

#ifdef SUBSURFACE_MOBILE
	register_qml_type<QMLManager>("QMLManager");
	register_qml_type<StatsManager>("StatsManager");
	register_qml_type<QMLProfile>("QMLProfile");
	register_qml_type<DiveImportedModel>("DCImportModel");
	register_qml_type<DiveSummaryModel>("DiveSummaryModel");
	register_qml_type<ChartListModel>("ChartListModel");
#endif // not SUBSURFACE_MOBILE

#ifdef MAP_SUPPORT
	register_qml_type<MapWidgetHelper>("MapWidgetHelper");
	register_qml_type<MapLocationModel>("MapLocationModel");
#endif
	register_qml_type<StatsView>("StatsView");
}
