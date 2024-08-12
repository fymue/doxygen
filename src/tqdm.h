#ifndef TQDM_H_
#define TQDM_H_

/*
 *Copyright (c) 2018-2019 <Miguel Raggi> <mraggi@gmail.com>
 *
 *Permission is hereby granted, free of charge, to any person
 *obtaining a copy of this software and associated documentation
 *files (the "Software"), to deal in the Software without
 *restriction, including without limitation the rights to use,
 *copy, modify, merge, publish, distribute, sublicense, and/or sell
 *copies of the Software, and to permit persons to whom the
 *Software is furnished to do so, subject to the following
 *conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *OTHER DEALINGS IN THE SOFTWARE.
 */

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace tqdm {

using time_point_t = std::chrono::time_point<std::chrono::steady_clock>;

inline double elapsed_seconds(time_point_t from, time_point_t to)
{
    using seconds = std::chrono::duration<double>;
    return std::chrono::duration_cast<seconds>(to - from).count();
}

class Chronometer
{
public:
    Chronometer() : start_(std::chrono::steady_clock::now()) {}

    double reset()
    {
        auto previous = start_;
        start_ = std::chrono::steady_clock::now();

        return elapsed_seconds(previous, start_);
    }

    [[nodiscard]] double peek() const
    {
        auto now = std::chrono::steady_clock::now();

        return elapsed_seconds(start_, now);
    }

    [[nodiscard]] time_point_t get_start() const { return start_; }

private:
    time_point_t start_;
};

class ProgressBar {
 public:
  ProgressBar() = delete;

  explicit ProgressBar(unsigned int total_steps) :
    min_time_per_update_(0.1),
    bar_size_(40),
    current_step_(0),
    total_steps_(total_steps) {}

    void restart()
    {
        chronometer_.reset();
        refresh_.reset();
    }

    void update(unsigned elapsed_steps, bool force = false)
    {
        if (current_step_ + elapsed_steps > total_steps_) {
            current_step_ = total_steps_;
        } else {
            current_step_ += elapsed_steps;
        }

        if (time_since_refresh() > min_time_per_update_ || force) {
            reset_refresh_timer();
            update_display();
        }
    }

    void update() {
        update(1);
    }

    void fill() {
        update(total_steps_ - current_step_, true);
    }

    void set_ostream(std::ostream& os) { os_ = &os; }
    void set_prefix(const std::string &s) { prefix_ = s; }
    void set_bar_size(unsigned int size) { bar_size_ = size; }
    void set_min_update_time(double time) { min_time_per_update_ = time; }

    double elapsed_time() const { return chronometer_.peek(); }

private:
    void update_display()
    {
        double progress = static_cast<double>(current_step_) / total_steps_;
        auto flags = os_->flags();

        double t = chronometer_.peek();
        double eta = t / progress - t;

        std::stringstream bar;

        bar << '\r' << prefix_ << ' ' << std::fixed << std::setprecision(1)
            << std::setw(5) << progress * 100 << '%';

        print_bar(bar, progress);

        bar << " [" << t << "s<" << eta << "s]";

        (*os_) << bar.str() << std::flush;
        os_->flags(flags);
    }

    void print_bar(std::stringstream& ss, double fill_percentage) const
    {
        unsigned int num_filled = static_cast<int>(
            std::round(fill_percentage * bar_size_));

        ss << '|';

        for (unsigned int i = 0; i < num_filled; ++i) {
            ss << "\u2588";
        }

        int last_fill_idx = static_cast<int>(fill_percentage * 100)
                          - static_cast<int>(std::floor(fill_percentage * 10) * 10);
        last_fill_idx = std::min(last_fill_idx, 6);
        ss << get_fill_value(last_fill_idx);
        num_filled++;

        if (num_filled < bar_size_) {
          ss << std::string(bar_size_ - num_filled, ' ');
        }

        ss << '|';
    }

    constexpr const char* get_fill_value(int idx) const {
        switch (idx) {
            case 0: return "\u258F";
            case 1: return "\u258E";
            case 2: return "\u258D";
            case 3: return "\u258C";
            case 4: return "\u258B";
            case 5: return "\u258A";
            case 6: return "\u2589";
            default: return "\u2588";
        }
    }

    double time_since_refresh() const { return refresh_.peek(); }
    void reset_refresh_timer() { refresh_.reset(); }

    Chronometer chronometer_;
    Chronometer refresh_;
    double min_time_per_update_;
    std::ostream* os_{&std::cerr};
    std::string prefix_;
    unsigned int bar_size_, current_step_, total_steps_;
};

}  // namespace tqdm

#endif  // TQDM_H_
