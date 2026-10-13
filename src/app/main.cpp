#include "media/ffmpeg_info.h"
#include "ui/main_window.h"
#include "ui/theme.h"

#include <QApplication>
#include <QTimer>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("MCS AI Editor"));
    QApplication::setApplicationVersion(QStringLiteral(MCS_VERSION));
    QApplication::setOrganizationName(QStringLiteral("MCS"));
    mcs::ui::applyDarkTheme(app);

    mcs::ui::MainWindow window;
    window.show();

    // --smoke: abre a janela e sai (teste de inicialização/deploy de DLLs).
    if (QApplication::arguments().contains(QStringLiteral("--smoke"))) {
        if (mcs::media::ffmpegVersion().isEmpty()) {
            return 2;
        }
        QTimer::singleShot(0, &app, &QApplication::quit);
    }
    return QApplication::exec();
}
