#include <QApplication>
#include <QWidget>

#include "window.h"

#include <vips/vips8>
#include <archive.h>
#include <archive_entry.h>
#include <fpdfview.h>

int main(int argc, char** argv) {
    if (VIPS_INIT(argv[0])) {
        vips_error_exit(nullptr);
    }
    vips_concurrency_set(1);

    FPDF_InitLibrary();

    QApplication app(argc, argv);

    auto font = app.font();
    font.setPointSize(14);
    app.setFont(font);

    Window window;
    window.show();
    auto result = app.exec();

    FPDF_DestroyLibrary();
    return result;
}
