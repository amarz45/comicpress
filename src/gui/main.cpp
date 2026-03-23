#include "../worker/include/worker.hpp"
#include "include/window.hpp"

int main(int argc, char **argv) {
    if (argc > 1) {
        return worker_main(argc, argv);
    }

#if defined(PDF_ENABLED)
    // PDFium is needed here to discover the number of pages in PDF files.
    FPDF_InitLibrary();
#endif

    QApplication app(argc, argv);

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
