#include "include/window.hpp"
#include "include/window_util.hpp"

void Window::update_time_labels() {
    update_overall_time_labels();

    for (const QString &file : active_progress_bars_.keys()) {
        update_file_time_labels(file);
    }
}

void Window::update_overall_time_labels() {
    if (!start_time_.has_value()) {
        return;
    }

    auto start_time = start_time_.value();

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()
    )
                  .count();

    auto elapsed = ms - start_time;
    auto elapsed_str = "Elapsed: " + time_to_str(elapsed);
    elapsed_label_->setText(QString::fromStdString(elapsed_str));

    auto value = pages_processed_;

    if (last_eta_time_.has_value() && ms - last_eta_time_.value() >= 1000
        && images_since_last_eta_ > 0) {
        auto images = images_since_last_eta_;
        auto interval = ms - last_eta_time_.value();

        eta_samples_.push_front({images, interval});

        last_eta_time_ = ms;
        images_since_last_eta_ = 0;
    }

    if (!eta_samples_.dq.empty()) {
        int64_t images = 0;
        int64_t interval = 0;

        for (const auto &[i, d] : eta_samples_.dq) {
            images += i;
            interval += d;
        }

        if (images > 0) {
            auto speed
                = static_cast<double>(images) / static_cast<double>(interval);
            auto remaining = static_cast<double>(total_pages_ - value) / speed;
            auto eta_str
                = "ETA: " + time_to_str(static_cast<int64_t>(remaining));
            eta_label_->setText(QString::fromStdString(eta_str));
        }
    }
}

void Window::update_file_time_labels(const QString &file) {
    if (!file_timers_.contains(file)) {
        return;
    }

    auto &file_timer = file_timers_[file];

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
    if (file_elapsed_labels_.contains(file)) {
        file_elapsed_labels_[file]->setText(
            QString::fromStdString(elapsed_str)
        );
    }

    auto value = pages_processed_per_archive_.value(file, 0);
    auto total = total_pages_per_archive_.value(file, 0);

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

            if (file_eta_labels_.contains(file)) {
                file_eta_labels_[file]->setText(
                    QString::fromStdString(eta_str)
                );
            }
        }
    }
}
