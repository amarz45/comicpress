#include "include/conversion.hpp"
#include "archive.hpp"

#include <QCoreApplication>
#include <QObject>
#include <QString>
#include <QStringList>

#if defined(PDF_ENABLED)
#include <fpdfview.h>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <format>
#include <iomanip>
#include <ranges>
#include <sstream>

static bool create_output_dir(fs::path &output_dir) {
    auto output_parent = output_dir.parent_path();
    auto err_code = std::error_code{};
    if (!output_parent.empty()) {
        fs::create_directories(output_parent, err_code);
        if (err_code) {
            return false;
        }
    }

    auto output_base_dir = output_dir;
    auto i = 1;
    while (!fs::create_directory(output_dir, err_code)) {
        if (i > 1000 || (err_code && err_code != std::errc::file_exists)) {
            return false;
        }
        output_dir = output_base_dir;
        output_dir += "_" + std::to_string(i);
        i += 1;
    }

    return true;
}

Conversion::Conversion(ConversionRequest request, ConversionListener &listener)
    : request_(std::move(request)), listener_(listener) {
}

Conversion::~Conversion() {
    stop_workers();
    // Silent: this can run while the listener itself is being destroyed.
    if (!temp_dir_.empty()) {
        auto err_code = std::error_code{};
        fs::remove_all(temp_dir_, err_code);
    }
}

void Conversion::start() {
    // Create unique base temp directory for this run
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch()
    )
                      .count();
    auto temp_dir
        = fs::temp_directory_path() / ("comicpress_" + std::to_string(now_ms));

    try {
        fs::create_directories(temp_dir);
        temp_dir_ = temp_dir;
    }
    catch (const std::exception &e) {
        log_error(
            std::format("Failed to create temporary directory: {}", e.what())
        );
        finish();
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    auto local_time = std::localtime(&now_time);
    auto oss = std::ostringstream{};
    oss << std::put_time(local_time, "%Y-%m-%d %H-%M-%S");

    output_path_ = request_.output_dir / oss.str();
    if (!create_output_dir(output_path_)) {
        log_error("Failed to create output directory.");
        finish();
        return;
    }

    discover_tasks();
    if (task_queue_.empty()) {
        log_error("No pages found to process.");
        finish();
        return;
    }

    listener_.started(pages_total_);
    for (auto i = 0; i < request_.max_workers; i += 1) {
        start_next_task();
    }
}

void Conversion::cancel() {
    task_queue_ = {};
    stop_workers();
    remove_temp_dir();

    // Clean up empty output dir
    auto err_code = std::error_code{};
    if (fs::is_directory(output_path_, err_code)
        && fs::is_empty(output_path_, err_code)) {
        fs::remove(output_path_, err_code);
    }
}

void Conversion::discover_tasks() {
    for (const auto &source_file : request_.input_files) {
        auto extension = source_file.extension().string();
        std::transform(
            extension.begin(), extension.end(), extension.begin(), ::tolower
        );

        // Collected first, so a file that fails partway queues nothing.
        auto tasks = std::vector<PageTask>{};
        try {
            if (extension == ".cbz" || extension == ".cbr") {
                fs::create_directories(temp_dir_ / source_file.stem());

                auto archive
                    = ArchiveReader::open(source_file.string().c_str());
                if (!archive) {
                    log_error(
                        std::format(
                            "Error: failed to read archive '{}'.",
                            source_file.string()
                        )
                    );
                    continue;
                }

                auto page_num = 0;
                while (auto entry = archive->next()) {
                    if (!entry->is_regular_file()) {
                        continue;
                    }

                    auto task = create_task(source_file, page_num);
                    task.path_in_archive = entry->pathname();
                    task.output_base_name = fs::path(task.path_in_archive)
                                                .replace_extension("")
                                                .string();
                    tasks.push_back(std::move(task));

                    page_num += 1;
                }

                if (tasks.empty()) {
                    log_error(
                        std::format(
                            "Archive {} contains no files.",
                            source_file.string()
                        )
                    );
                    continue;
                }
            }
#if defined(PDF_ENABLED)
            else if (extension == ".pdf") {
                auto doc
                    = FPDF_LoadDocument(source_file.string().c_str(), nullptr);
                if (!doc) {
                    log_error(
                        std::format(
                            "Error: Cannot open PDF document {}. Error code: "
                            "{}",
                            source_file.string(),
                            FPDF_GetLastError()
                        )
                    );
                    continue;
                }

                auto page_count = FPDF_GetPageCount(doc);
                FPDF_CloseDocument(doc);
                if (page_count == 0) {
                    log_error(
                        std::format(
                            "PDF {} contains no pages.", source_file.string()
                        )
                    );
                    continue;
                }

                fs::create_directories(temp_dir_ / source_file.stem());

                auto padding_width
                    = static_cast<int>(std::floor(std::log10(page_count))) + 1;
                for (auto i = 0; i < page_count; i += 1) {
                    auto task = create_task(source_file, i);
                    task.output_base_name
                        = QString("%1")
                              .arg(i + 1, padding_width, 10, QChar('0'))
                              .toStdString();
                    tasks.push_back(std::move(task));
                }
            }
#endif
        }
        catch (const std::exception &e) {
            log_error(
                std::format(
                    "Error discovering tasks in {}: {}",
                    source_file.string(),
                    e.what()
                )
            );
            continue;
        }

        auto page_count = static_cast<int>(tasks.size());
        files_[source_file].pages_total = page_count;
        pages_total_ += page_count;
        for (auto &task : tasks) {
            task_queue_.push(std::move(task));
        }
    }
}

PageTask Conversion::create_task(const fs::path &source_file, int page_num) {
    auto task = PageTask{};
    task.source_file = source_file;
    task.output_dir = temp_dir_ / source_file.stem();
    task.page_number = page_num;
    task.processing_settings = request_.processing_settings;
    return task;
}

void Conversion::start_next_task() {
    if (task_queue_.empty()
        || std::ssize(running_tasks_) >= request_.max_workers) {
        return;
    }

    auto task = std::move(task_queue_.front());
    task_queue_.pop();

    // Tasks are queued in page order, so this is the file's first.
    if (task.page_number == 0) {
        listener_.file_started(
            task.source_file, files_[task.source_file].pages_total
        );
    }

    auto arguments = QStringList{};
    for (const auto &argument : encode_task(task)) {
        arguments << QString::fromStdString(argument);
    }

    auto process = new QProcess();
    running_tasks_.emplace(process, std::move(task));

    QObject::connect(
        process, &QProcess::readyReadStandardOutput, process, [this, process] {
            // Read line by line to prevent partial messages
            while (process->canReadLine()) {
                listener_.log(process->readLine().trimmed().toStdString());
            }
        }
    );
    QObject::connect(
        process,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        process,
        [this, process](int exit_code, QProcess::ExitStatus exit_status) {
            if (exit_status == QProcess::CrashExit || exit_code != 0) {
                log_error(
                    std::format(
                        "Worker process failed or crashed. Exit code: {}",
                        exit_code
                    )
                );
            }
            handle_worker_exit(process);
        }
    );
    // Queued: on Windows, `start()` reports a failed launch synchronously, and
    // handling it inline would re-enter `start_next_task` once per queued page.
    QObject::connect(
        process,
        &QProcess::errorOccurred,
        process,
        [this, process](QProcess::ProcessError error) {
            // `finished` handles the cases where the process actually exists.
            if (error != QProcess::FailedToStart) {
                return;
            }
            log_error("Worker process failed to start.");
            handle_worker_exit(process);
        },
        Qt::QueuedConnection
    );

    process->start(QCoreApplication::applicationFilePath(), arguments);
}

void Conversion::handle_worker_exit(QProcess *process) {
    process->deleteLater();
    auto node = running_tasks_.extract(process);
    if (node.empty()) {
        return;
    }

    auto &source = node.mapped().source_file;
    auto &file = files_[source];
    file.pages_done += 1;
    pages_done_ += 1;
    listener_.page_finished(source, file.pages_done);

    if (file.pages_done == file.pages_total) {
        create_archive(source);
        listener_.file_finished(source);
    }

    if (pages_done_ == pages_total_) {
        finish();
    }
    else {
        start_next_task();
    }
}

void Conversion::create_archive(const fs::path &source) {
    auto title = source.stem();
    auto temp_dir = temp_dir_ / title;
    auto output_filename = source.filename();

    try {
        if (request_.output_format == OutputFormat::EPUB) {
            output_filename.replace_extension(".epub");
            create_epub(
                temp_dir, output_path_ / output_filename, title.generic_string()
            );
        }
        else {
            output_filename.replace_extension(".cbz");
            create_cbz(temp_dir, output_path_ / output_filename);
        }
    }
    catch (const NoImagesError &) {
        log_error(
            std::format(
                "Error: source '{}' does not contain any images.",
                source.string()
            )
        );
        return;
    }
    catch (const ArchiveError &) {
        log_error("Error: failed to create archive.");
        return;
    }
    catch (const std::exception &e) {
        log_error(std::format("Error: {}", e.what()));
        return;
    }

    try {
        fs::remove_all(temp_dir);
    }
    catch (const std::exception &e) {
        log_error(
            std::format(
                "Error cleaning up temp directory {}: {}",
                temp_dir.string(),
                e.what()
            )
        );
    }
}

// Every exit from a run goes through here, so a failure can’t leave the temp
// directory behind.
void Conversion::finish() {
    remove_temp_dir();
    listener_.finished(ok_);
}

void Conversion::stop_workers() {
    for (auto *process : running_tasks_ | std::views::keys) {
        // A killed process lives on until it exits, which can be after `this`
        // is gone, so it must not call back into it.
        process->disconnect();
        if (process->state() == QProcess::NotRunning) {
            delete process;
            continue;
        }
        QObject::connect(
            process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            process,
            &QObject::deleteLater
        );
        process->kill();
    }
    running_tasks_.clear();
}

void Conversion::remove_temp_dir() {
    if (temp_dir_.empty()) {
        return;
    }

    auto err_code = std::error_code{};
    fs::remove_all(temp_dir_, err_code);
    if (err_code) {
        listener_.log(
            std::format(
                "Warning: failed to delete temp directory '{}': {}",
                temp_dir_.string(),
                err_code.message()
            )
        );
    }
    temp_dir_.clear();
}

void Conversion::log_error(const std::string &message) {
    ok_ = false;
    listener_.log(message);
}
