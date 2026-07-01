#pragma once

#include <QMainWindow>
#include <QFutureWatcher>
#include <string>

class QTextEdit;
class QPushButton;

// Skeleton GUI. Computes the Hilbert basis of the 2cone example in-process via
// Cone<mpz_class>. Two robustness properties proven necessary by review:
//   - libnormaliz throws NormalizException (bad input, missing optional lib, ...);
//     an exception must NEVER escape into Qt (undefined behaviour), so the worker
//     catches everything and returns a Result.
//   - compute() can run for a long time, so it runs off the GUI thread via
//     QtConcurrent and the UI updates when the QFutureWatcher finishes.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void startCompute();
    void computeFinished();

private:
    struct Result {
        bool ok = false;
        std::string text;  // output on success, error message on failure
    };
    // Runs in a worker thread. Catches ALL exceptions - none may reach Qt.
    static Result runHilbertBasis();

    QPushButton* btn_;
    QTextEdit* output_;
    QFutureWatcher<Result> watcher_;
};
