#pragma once

#include "output_formats.hpp"
#include "task.hpp"

#include <QProcess>

#include <filesystem>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

struct ConversionRequest {
    std::vector<fs::path> input_files;
    // Parent of the timestamped folder the run writes into.
    fs::path output_dir;
    OutputFormat output_format;
    ProcessingSettings processing_settings;
    int max_workers;
};

class ConversionListener {
  public:
    virtual ~ConversionListener() = default;

    virtual void started(int pages_total) = 0;
    virtual void file_started(const fs::path &source, int pages_total) = 0;
    virtual void page_finished(const fs::path &source, int pages_done) = 0;
    virtual void file_finished(const fs::path &source) = 0;
    virtual void log(const std::string &message) = 0;
    // `ok` is false if anything was logged as an error.
    virtual void finished(bool ok) = 0;
};

struct FileProgress {
    int pages_total = 0;
    int pages_done = 0;
};

class Conversion {
  public:
    Conversion(ConversionRequest request, ConversionListener &listener);
    ~Conversion();

    void start();
    // Doesn't call `finished`, since the caller already knows.
    void cancel();

  private:
    void discover_tasks();
    PageTask create_task(const fs::path &source_file, int page_num);
    void start_next_task();
    void handle_worker_exit(QProcess *process);
    void create_archive(const fs::path &source);
    void finish();
    void stop_workers();
    void remove_temp_dir();
    void log_error(const std::string &message);

    ConversionRequest request_;
    ConversionListener &listener_;
    std::queue<PageTask> task_queue_;
    std::unordered_map<QProcess *, PageTask> running_tasks_;
    std::unordered_map<fs::path, FileProgress> files_;
    int pages_total_ = 0;
    int pages_done_ = 0;
    bool ok_ = true;
    fs::path temp_dir_;
    fs::path output_path_;
};
