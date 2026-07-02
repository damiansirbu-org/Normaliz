#pragma once

#include <QMainWindow>
#include <QFutureWatcher>
#include <QString>
#include <QElapsedTimer>
#include <string>

class QPlainTextEdit;
class QPushButton;
class QCheckBox;
class QRadioButton;
class QCloseEvent;
class QLabel;
class QTimer;

// Clean 3-zone layout: input (.in) editor, computation-goal selector, output,
// plus a File menu. The computation runs off the GUI thread (QtConcurrent) and
// libnormaliz exceptions are caught in the worker (none may reach the event loop).
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void startCompute();
    void stopCompute();
    void computeFinished();
    void newFile();
    void openFile();
    void saveFile();
    void saveFileAs();

private:
    struct Goals { bool hilbert; bool extreme; bool support; bool hseries; bool mult;
                   bool volume; bool latpts; bool classgrp; };
    struct Result { bool ok = false; bool stopped = false; std::string text; };
    // Worker thread. Parses the .in text and computes; catches every exception.
    static Result runCompute(std::string inputText, Goals goals);

    void buildMenu();
    void updateTitle();
    bool maybeSave();   // prompt to save if modified; false = caller should abort

    QPlainTextEdit* input_;
    QPlainTextEdit* output_;
    QPushButton* compute_;
    QPushButton* stop_;
    QLabel* elapsedLabel_;
    QTimer* tick_;
    QElapsedTimer elapsed_;
    QCheckBox* cbHilbert_;
    QCheckBox* cbExtreme_;
    QCheckBox* cbSupport_;
    QCheckBox* cbHSeries_;
    QCheckBox* cbMult_;
    QCheckBox* cbVolume_;
    QCheckBox* cbLatPts_;
    QCheckBox* cbClassGrp_;
    QRadioButton* backendLocal_;
    QRadioButton* backendRemote_;
    QString currentPath_;
    QFutureWatcher<Result> watcher_;
};
