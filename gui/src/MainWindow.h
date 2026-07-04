#pragma once

#include <QMainWindow>
#include <QFutureWatcher>
#include <QString>
#include <QElapsedTimer>
#include <string>
#include <vector>

class QPlainTextEdit;
class QPushButton;
class QCheckBox;
class QRadioButton;
class QCloseEvent;
class QLabel;
class QTimer;
class QComboBox;
class QSpinBox;
class QTabWidget;

// Layout: input (.in) editor + a scrollable computation-goal selector on top; a
// tabbed panel (Output / Console / Options) below; File/Edit/Help menus and a
// run toolbar. Computation runs off the GUI thread (QtConcurrent); libnormaliz
// exceptions are caught in the worker and never reach the Qt event loop.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    // Headless test/batch hook (hidden `--run <in> <out>` in main.cpp): parse
    // `inputText`, compute the goals named in it, and return the rendered Output
    // text - the exact worker path the Compute button uses, without a window.
    static std::string runHeadless(const std::string& inputText, bool defaultMode = false);

    // Demo/screenshot hook: load input, run the real Compute path, screenshot
    // the Output tab and quit. Used by main.cpp's hidden --demo flag.
    void demoShot(const QString& inputText, const QString& pngPath);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void startCompute();
    void stopCompute();
    void computeFinished();
    void newFile();
    void openFile();
    void closeFile();
    void saveFile();
    void saveFileAs();
    void printCurrent();

private:
    // idx = indices into the goal table (see MainWindow.cpp); the rest are the
    // toolbar/options combo values. Passed by value to the worker thread.
    struct Goals { std::vector<int> idx; int algo; int mode; int prec; int threads; };
    struct Result { bool ok = false; bool stopped = false; std::string text; std::string console; };
    // Worker thread. Parses the .in text and computes; catches every exception.
    static Result runCompute(std::string inputText, Goals goals);

    void buildMenu();
    void updateTitle();
    bool maybeSave();
    bool busyGuard();   // true (and refuses) if a computation is running

    QPlainTextEdit* input_;
    QPlainTextEdit* output_;
    QPlainTextEdit* console_;
    QTabWidget* tabs_;
    QPushButton* compute_;
    QPushButton* stop_;
    std::vector<QCheckBox*> cbGoals_;   // parallel to the goal table
    QComboBox* algoCombo_;
    QComboBox* modeCombo_;
    QComboBox* precCombo_;
    QSpinBox* threadsSpin_;
    QSpinBox* fontSpin_;
    QRadioButton* backendLocal_;
    QRadioButton* backendRemote_;
    QLabel* elapsedLabel_;
    QLabel* memLabel_;
    QTimer* tick_;
    QTimer* memTick_;
    QElapsedTimer elapsed_;
    QString currentPath_;
    QFutureWatcher<Result> watcher_;
};
