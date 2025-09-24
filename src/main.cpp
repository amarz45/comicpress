#include <QApplication>
#include <QWidget>
#include <fpdfview.h>

#include "window.hpp"

int main(int argc, char **argv) {
    // PDFium is needed here to discover the number of pages in PDF files.
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
