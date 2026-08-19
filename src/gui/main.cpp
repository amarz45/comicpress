#include "../worker/include/worker.hpp"
#include "include/window.hpp"

int main(int argc, char **argv) {
    if (argc > 1) {
        return worker_main(argc, argv);
    }

    // libvips is needed to get the page dimensions for EPUBs.
    if (VIPS_INIT(argv[0])) {
        vips_error_exit(nullptr);
    }

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
        QIcon::fromTheme("io.github.amarz45.Comicpress")
    );

    auto font = app.font();
    // font.setPointSize(14);
    app.setFont(font);

    Window window;
    window.show();
    auto result = app.exec();

#if defined(PDF_ENABLED)
    FPDF_DestroyLibrary();
#endif
    return result;
}
