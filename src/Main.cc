#include <App.hh>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    /// Initialise app data.
    smyth::App Smyth;

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );

    auto ctx = engine.rootContext();
    ctx->setContextProperty("SmythContext", &Smyth);
    engine.loadFromModule("Smyth", "Main");
    emit Smyth.init();
    return app.exec();
}
