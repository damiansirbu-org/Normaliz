#pragma once

#include <QMainWindow>
#include <QFutureWatcher>
#include <string>

class QPlainTextEdit;
class QPushButton;
class QCheckBox;
class QRadioButton;

// Clean 3-zone layout: input (.in) editor, computation-goal selector, output.
// The computation runs off the GUI thread (QtConcurrent) and libnormaliz
// exceptions are caught in the worker (none may reach the Qt event loop).
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void startCompute();
    void computeFinished();

private:
    struct Goals { bool hilbert; bool extreme; bool support; };
    struct Result { bool ok = false; std::string text; };
    static Result runCompute(Goals goals);   // worker thread; catches everything

    QPlainTextEdit* input_;
    QPlainTextEdit* output_;
    QPushButton* compute_;
    QCheckBox* cbHilbert_;
    QCheckBox* cbExtreme_;
    QCheckBox* cbSupport_;
    QRadioButton* backendLocal_;
    QRadioButton* backendRemote_;
    QFutureWatcher<Result> watcher_;
};
