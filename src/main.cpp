#include <QApplication>
#include <QWidget>

#include "window.h"

#include <vips/vips8>
#include <mupdf/fitz.h>
#include <archive.h>
#include <archive_entry.h>

int main(int argc, char** argv) {
    if (VIPS_INIT(argv[0])) {
        vips_error_exit(nullptr);
    }
    vips_concurrency_set(1);

    QApplication app(argc, argv);

    auto font = app.font();
    font.setPointSize(14);
    app.setFont(font);

    Window window;
    window.show();
    return app.exec();
}
