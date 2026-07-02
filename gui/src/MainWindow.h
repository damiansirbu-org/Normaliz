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
class QComboBox;
class QSpinBox;

// Layout: input (.in) editor + computation-goal selector on top; a tabbed panel
// (Output / Console / Options) below; a File/Edit/Help menu and a run toolbar.
// The computation runs off the GUI thread (QtConcurrent); libnormaliz exceptions
// are caught in the worker and never reach the Qt event loop.
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
                   bool volume; bool latpts; bool classgrp;
                   int algo; int mode; int prec; int threads; };
    struct Result { bool ok = false; bool stopped = false; std::string text; std::string console; };
    // Worker thread. Parses the .in text and computes; catches every exception.
    static Result runCompute(std::string inputText, Goals goals);

    void buildMenu();
    void updateTitle();
    bool maybeSave();

    QPlainTextEdit* input_;
    QPlainTextEdit* output_;
    QPlainTextEdit* console_;
    QPushButton* compute_;
    QPushButton* stop_;
    QCheckBox* cbHilbert_;
    QCheckBox* cbExtreme_;
    QCheckBox* cbSupport_;
    QCheckBox* cbHSeries_;
    QCheckBox* cbMult_;
    QCheckBox* cbVolume_;
    QCheckBox* cbLatPts_;
    QCheckBox* cbClassGrp_;
    QComboBox* algoCombo_;
    QComboBox* modeCombo_;
    QComboBox* precCombo_;
    QSpinBox* threadsSpin_;
    QSpinBox* fontSpin_;
    QRadioButton* backendLocal_;
    QRadioButton* backendRemote_;
    QLabel* elapsedLabel_;
    QTimer* tick_;
    QElapsedTimer elapsed_;
    QString currentPath_;
    QFutureWatcher<Result> watcher_;
};
