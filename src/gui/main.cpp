#include "../worker/include/worker.hpp"
#include "include/window.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QSettings>

#include <vips/vips8>

#if defined(PDF_ENABLED)
#include <fpdfview.h>
#endif

#if defined(_WIN32)
#include <winsparkle.h>

static int can_shutdown() {
    return is_converting.load() ? 0 : 1;
}

static void request_shutdown() {
    QMetaObject::invokeMethod(
        QCoreApplication::instance(),
        &QCoreApplication::quit,
        Qt::QueuedConnection
    );
}
#endif

int main(int argc, char **argv) {
    if (argc > 1) {
        return worker_main(argc, argv);
    }

    // libvips is needed to get the page dimensions for EPUBs.
    if (VIPS_INIT(argv[0])) {
        vips_error_exit(nullptr);
    }
    // This prevents libvips from stopping the cleanup of temp directories on
    // Windows. Also, the cache isn’t needed anyway since the GUI opens each
    // page exactly once and never reuses the result.
    vips_cache_set_max(0);

#if defined(PDF_ENABLED)
    // PDFium is needed here to discover the number of pages in PDF files.
    FPDF_InitLibrary();
#endif

    QApplication app(argc, argv);

    QApplication::setDesktopFileName("io.github.amarz45.Comicpress");

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QCoreApplication::setOrganizationName("io.github.amarz45");
    QCoreApplication::setApplicationName("Comicpress");
    QApplication::setWindowIcon(
        QIcon::fromTheme(
            "io.github.amarz45.Comicpress",
            QIcon(":/icons/io.github.amarz45.Comicpress.svg")
        )
    );

    auto font = app.font();
    // font.setPointSize(14);
    app.setFont(font);

    Window window;
    window.show();

#if defined(_WIN32)
    win_sparkle_set_can_shutdown_callback(can_shutdown);
    win_sparkle_set_shutdown_request_callback(request_shutdown);
    win_sparkle_init();
#endif

    auto result = app.exec();

#if defined(_WIN32)
    win_sparkle_cleanup();
#endif

#if defined(PDF_ENABLED)
    FPDF_DestroyLibrary();
#endif
    return result;
}
