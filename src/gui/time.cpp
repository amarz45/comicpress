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

    if (this->last_eta_time.has_value()
        && ms - this->last_eta_time.value() >= 1000
        && this->images_since_last_eta > 0) {
        auto images = this->images_since_last_eta;
        auto interval = ms - this->last_eta_time.value();

        this->eta_samples.push_front({images, interval});

        this->last_eta_time = ms;
        this->images_since_last_eta = 0;
    }

    if (!this->eta_samples.dq.empty()) {
        int64_t images = 0;
        int64_t interval = 0;

        for (const auto &[i, d] : this->eta_samples.dq) {
            images += i;
            interval += d;
        }

        if (images > 0) {
            auto speed
                = static_cast<double>(images) / static_cast<double>(interval);
            auto remaining
                = static_cast<double>(this->total_pages - value) / speed;
            auto eta_str
                = "ETA: " + time_to_str(static_cast<int64_t>(remaining));
            this->eta_label->setText(QString::fromStdString(eta_str));
        }
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

    if (file_timer.last_eta_time.has_value()
        && ms - file_timer.last_eta_time.value() >= 1000
        && file_timer.images_since_last_eta > 0) {
        auto images = file_timer.images_since_last_eta;
        auto interval = ms - file_timer.last_eta_time.value();

        file_timer.eta_samples.push_front({images, interval});

        file_timer.last_eta_time = ms;
        file_timer.images_since_last_eta = 0;
    }

    if (!file_timer.eta_samples.dq.empty()) {
        int64_t images = 0;
        int64_t interval = 0;
        for (const auto &[i, d] : file_timer.eta_samples.dq) {
            images += i;
            interval += d;
        }

        if (images > 0) {
            auto speed
                = static_cast<double>(images) / static_cast<double>(interval);
            auto remaining = static_cast<double>(total - value) / speed;
            auto eta_str
                = "ETA: " + time_to_str(static_cast<int64_t>(remaining));

            if (this->file_eta_labels.contains(file)) {
                this->file_eta_labels[file]->setText(
                    QString::fromStdString(eta_str)
                );
            }
        }
    }
}
