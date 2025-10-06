#include "include/window.hpp"
#include "include/window_util.hpp"

void Window::update_time_labels() {
    this->update_overall_time_labels();

    for (const QString &file : this->active_progress_bars.keys()) {
        this->update_file_time_labels(file);
    }
}

void Window::update_overall_time_labels() {
    if (!this->start_time.has_value()) {
        return;
    }

    auto start_time = this->start_time.value();

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()
    )
                  .count();

    auto elapsed = ms - start_time;
    auto elapsed_str = "Elapsed: " + time_to_str(elapsed);
    this->elapsed_label->setText(QString::fromStdString(elapsed_str));

    auto value = this->pages_processed;

    // Overall ETA
    if (value > 0) {
        auto total = this->total_pages;
        auto per_unit = static_cast<double>(elapsed) / value;
        auto remaining = static_cast<double>(total - value) * per_unit;
        auto eta_overall_str
            = "ETA (overall): " + time_to_str(static_cast<int64_t>(remaining));
        this->eta_overall_label->setText(
            QString::fromStdString(eta_overall_str)
        );
    }

    // Recent ETA
    if (this->last_eta_recent_time.has_value()
        && ms - this->last_eta_recent_time.value() >= 1000
        && this->images_since_last_eta_recent > 0) {
        auto interval = ms - this->last_eta_recent_time.value();
        auto speed = static_cast<double>(this->images_since_last_eta_recent)
                   / static_cast<double>(interval);
        auto total_remaining = this->total_pages - value;
        auto remaining = static_cast<double>(total_remaining) / speed;

        this->eta_recent_intervals.push_front(static_cast<int64_t>(remaining));

        this->last_eta_recent_time = ms;
        this->images_since_last_eta_recent = 0;
    }

    if (!this->eta_recent_intervals.dq.empty()) {
        auto dq = this->eta_recent_intervals.dq;
        auto sum = std::accumulate(dq.begin(), dq.end(), 0LL);
        auto avg_eta = static_cast<double>(sum) / dq.size();
        auto eta_recent_str
            = "ETA (recent): " + time_to_str(static_cast<int64_t>(avg_eta));
        this->eta_recent_label->setText(QString::fromStdString(eta_recent_str));
    }
}

void Window::update_file_time_labels(const QString &file) {
    if (!this->file_timers.contains(file)) {
        return;
    }

    auto &file_timer = this->file_timers[file];

    if (!file_timer.start_time.has_value()) {
        return;
    }

    auto start_time = file_timer.start_time.value();

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()
    )
                  .count();

    auto elapsed = ms - start_time;
    auto elapsed_str = "Elapsed: " + time_to_str(elapsed);
    if (this->file_elapsed_labels.contains(file)) {
        this->file_elapsed_labels[file]->setText(
            QString::fromStdString(elapsed_str)
        );
    }

    auto value = this->pages_processed_per_archive.value(file, 0);
    auto total = this->total_pages_per_archive.value(file, 0);

    // Overall ETA
    if (value > 0) {
        auto per_unit = static_cast<double>(elapsed) / value;
        auto remaining = static_cast<double>(total - value) * per_unit;
        auto eta_overall_str
            = "ETA (overall): " + time_to_str(static_cast<int64_t>(remaining));
        if (this->file_eta_overall_labels.contains(file)) {
            this->file_eta_overall_labels[file]->setText(
                QString::fromStdString(eta_overall_str)
            );
        }
    }

    // Recent ETA
    if (file_timer.last_eta_recent_time.has_value()
        && ms - file_timer.last_eta_recent_time.value() >= 1000
        && file_timer.images_since_last_eta_recent > 0) {
        auto interval = ms - file_timer.last_eta_recent_time.value();
        auto speed
            = static_cast<double>(file_timer.images_since_last_eta_recent)
            / static_cast<double>(interval);
        auto total_remaining = total - value;
        auto remaining = static_cast<double>(total_remaining) / speed;

        file_timer.eta_recent_intervals.push_front(
            static_cast<int64_t>(remaining)
        );

        file_timer.last_eta_recent_time = ms;
        file_timer.images_since_last_eta_recent = 0;
    }

    if (!file_timer.eta_recent_intervals.dq.empty()) {
        auto dq = file_timer.eta_recent_intervals.dq;
        auto sum = std::accumulate(dq.begin(), dq.end(), 0LL);
        auto avg_eta = static_cast<double>(sum) / dq.size();
        auto eta_recent_str
            = "ETA (recent): " + time_to_str(static_cast<int64_t>(avg_eta));
        if (this->file_eta_recent_labels.contains(file)) {
            this->file_eta_recent_labels[file]->setText(
                QString::fromStdString(eta_recent_str)
            );
        }
    }
}
