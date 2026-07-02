#include "MainWindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QLabel>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QInputDialog>
#include <QKeySequence>
#include <QStringList>
#include <QTextDocument>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QToolBar>
#include <QComboBox>
#include <QtConcurrent>

#include <sstream>
#include <vector>
#include <map>

#include "libnormaliz/cone.h"
#include "libnormaliz/input.h"
#include "libnormaliz/HilbertSeries.h"
#include "libnormaliz/normaliz_exception.h"
#include "libnormaliz/general.h"

using namespace libnormaliz;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* top = new QHBoxLayout();
    top->setSpacing(12);

    auto* inputBox = new QGroupBox("Input (.in)");
    auto* inLay = new QVBoxLayout(inputBox);
    input_ = new QPlainTextEdit();
    input_->setPlainText("amb_space 2\ncone 2\n1 3\n2 1");
    inLay->addWidget(input_);
    top->addWidget(inputBox, 3);

    auto* goalsBox = new QGroupBox("Computation goals");
    auto* gLay = new QVBoxLayout(goalsBox);
    cbHilbert_ = new QCheckBox("Hilbert basis");
    cbHilbert_->setChecked(true);
    cbExtreme_ = new QCheckBox("Extreme rays");
    cbSupport_ = new QCheckBox("Support hyperplanes");
    cbHSeries_ = new QCheckBox("Hilbert series");
    cbMult_ = new QCheckBox("Multiplicity");
    cbVolume_ = new QCheckBox("Volume");
    cbLatPts_ = new QCheckBox("Lattice points");
    cbClassGrp_ = new QCheckBox("Class group");
    gLay->addWidget(cbHilbert_);
    gLay->addWidget(cbExtreme_);
    gLay->addWidget(cbSupport_);
    gLay->addWidget(cbHSeries_);
    gLay->addWidget(cbMult_);
    gLay->addWidget(cbVolume_);
    gLay->addWidget(cbLatPts_);
    gLay->addWidget(cbClassGrp_);

    // Backend selector. Local (embedded libnormaliz) is the default and the only
    // implemented mode; Cloud (the distributed remote backend) is shown but
    // disabled - the architecture is in place, the implementation is WIP.
    // See gui/doc/architecture.md, section "Shared core (desktop and web)".
    auto* backendLabel = new QLabel("Backend");
    backendLabel->setObjectName("sectionLabel");
    gLay->addSpacing(8);
    gLay->addWidget(backendLabel);
    backendLocal_ = new QRadioButton("Local");
    backendLocal_->setChecked(true);
    backendLocal_->setToolTip("Compute in this application (embedded engine)");
    backendRemote_ = new QRadioButton("Cloud (WIP)");
    backendRemote_->setEnabled(false);
    backendRemote_->setToolTip("Distributed cloud backend - work in progress");
    gLay->addWidget(backendLocal_);
    gLay->addWidget(backendRemote_);

    gLay->addStretch();
    compute_ = new QPushButton("Compute");
    compute_->setObjectName("compute");
    gLay->addWidget(compute_);
    stop_ = new QPushButton("Stop");
    stop_->setEnabled(false);
    gLay->addWidget(stop_);
    top->addWidget(goalsBox, 2);

    root->addLayout(top, 3);

    auto* outBox = new QGroupBox("Output");
    auto* oLay = new QVBoxLayout(outBox);
    output_ = new QPlainTextEdit();
    output_->setReadOnly(true);
    oLay->addWidget(output_);
    root->addWidget(outBox, 2);

    setCentralWidget(central);
    buildMenu();

    // Toolbar: run options, mapped to libnormaliz ConeProperty flags.
    QToolBar* tb = addToolBar("Options");
    tb->setMovable(false);
    tb->addWidget(new QLabel(" Algorithm: "));
    algoCombo_ = new QComboBox();
    algoCombo_->addItems({"Default", "Primal", "Dual"});
    algoCombo_->setToolTip("Compute algorithm (PrimalMode / DualMode)");
    tb->addWidget(algoCombo_);
    tb->addWidget(new QLabel("   Mode: "));
    modeCombo_ = new QComboBox();
    modeCombo_->addItems({"Goals only", "DefaultMode"});
    modeCombo_->setToolTip("DefaultMode also computes Normaliz's default properties");
    tb->addWidget(modeCombo_);
    tb->addWidget(new QLabel("   Precision: "));
    precCombo_ = new QComboBox();
    precCombo_->addItems({"Default", "BigInt"});
    precCombo_->setToolTip("BigInt forces arbitrary-precision arithmetic");
    tb->addWidget(precCombo_);

    elapsedLabel_ = new QLabel("Elapsed: 0.0s");
    statusBar()->addPermanentWidget(elapsedLabel_);
    statusBar()->showMessage("Ready");
    tick_ = new QTimer(this);
    tick_->setInterval(100);

    input_->document()->setModified(false);   // the preloaded example is not "unsaved"
    updateTitle();

    connect(compute_, &QPushButton::clicked, this, &MainWindow::startCompute);
    connect(stop_, &QPushButton::clicked, this, &MainWindow::stopCompute);
    connect(&watcher_, &QFutureWatcher<Result>::finished, this, &MainWindow::computeFinished);
    connect(tick_, &QTimer::timeout, this, [this] {
        elapsedLabel_->setText(QString("Elapsed: %1s").arg(elapsed_.elapsed() / 1000.0, 0, 'f', 1));
    });
    connect(input_->document(), &QTextDocument::modificationChanged,
            this, [this](bool) { updateTitle(); });
}

void MainWindow::buildMenu() {
    QMenu* fileMenu = menuBar()->addMenu("&File");
    QAction* aNew = fileMenu->addAction("&New...", this, &MainWindow::newFile);
    aNew->setShortcut(QKeySequence::New);
    QAction* aOpen = fileMenu->addAction("&Open...", this, &MainWindow::openFile);
    aOpen->setShortcut(QKeySequence::Open);
    QAction* aSave = fileMenu->addAction("&Save", this, &MainWindow::saveFile);
    aSave->setShortcut(QKeySequence::Save);
    QAction* aSaveAs = fileMenu->addAction("Save &As...", this, &MainWindow::saveFileAs);
    aSaveAs->setShortcut(QKeySequence::SaveAs);
    fileMenu->addSeparator();
    QAction* aExit = fileMenu->addAction("E&xit", this, &QWidget::close);
    aExit->setShortcut(QKeySequence::Quit);

    QMenu* editMenu = menuBar()->addMenu("&Edit");
    QAction* aUndo = editMenu->addAction("&Undo", input_, &QPlainTextEdit::undo);
    aUndo->setShortcut(QKeySequence::Undo);
    QAction* aRedo = editMenu->addAction("&Redo", input_, &QPlainTextEdit::redo);
    aRedo->setShortcut(QKeySequence::Redo);
    editMenu->addSeparator();
    QAction* aCut = editMenu->addAction("Cu&t", input_, &QPlainTextEdit::cut);
    aCut->setShortcut(QKeySequence::Cut);
    QAction* aCopy = editMenu->addAction("&Copy", input_, &QPlainTextEdit::copy);
    aCopy->setShortcut(QKeySequence::Copy);
    QAction* aPaste = editMenu->addAction("&Paste", input_, &QPlainTextEdit::paste);
    aPaste->setShortcut(QKeySequence::Paste);
    editMenu->addSeparator();
    QAction* aSelAll = editMenu->addAction("Select &All", input_, &QPlainTextEdit::selectAll);
    aSelAll->setShortcut(QKeySequence::SelectAll);

    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("Normaliz &website", this, [] {
        QDesktopServices::openUrl(QUrl("https://github.com/Normaliz/Normaliz"));
    });
    helpMenu->addAction("Normaliz &manual", this, [] {
        QDesktopServices::openUrl(QUrl("https://github.com/Normaliz/Normaliz/blob/master/doc/Normaliz.pdf"));
    });
    helpMenu->addSeparator();
    helpMenu->addAction("&About", this, [this] {
        QMessageBox::about(this, "About Normaliz GUI",
            "<b>Normaliz GUI</b><br>"
            "A desktop interface for Normaliz, built on libnormaliz.<br><br>"
            "Normaliz by W. Bruns, B. Ichim, Ch. Soeger, U. v. d. Ohe.<br>"
            "GPL v3.");
    });
}

void MainWindow::updateTitle() {
    QString name = currentPath_.isEmpty() ? "untitled.in" : QFileInfo(currentPath_).fileName();
    setWindowTitle(QString("Normaliz - %1[*]").arg(name));
    setWindowModified(input_->document()->isModified());
}

// Prompt to save when there are unsaved edits. Returns false if the caller
// should abort (user chose Cancel, or a requested save did not complete).
bool MainWindow::maybeSave() {
    if (!input_->document()->isModified())
        return true;
    QMessageBox::StandardButton ret = QMessageBox::warning(
        this, "Normaliz",
        "The input has unsaved changes.\nDo you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save) {
        saveFile();
        return !input_->document()->isModified();  // false if Save As was cancelled
    }
    return ret != QMessageBox::Cancel;  // Discard -> true, Cancel -> false
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}

// New: ask for a matrix size and load a zero-filled cone template to edit,
// mirroring jNormaliz's "New input" dialog.
void MainWindow::newFile() {
    if (!maybeSave()) return;
    bool ok = false;
    int cols = QInputDialog::getInt(this, "New input", "Ambient dimension (columns):",
                                    2, 1, 100000, 1, &ok);
    if (!ok) return;
    int rows = QInputDialog::getInt(this, "New input", "Number of generators (rows):",
                                    2, 1, 1000000, 1, &ok);
    if (!ok) return;
    QString t = QString("amb_space %1\ncone %2\n").arg(cols).arg(rows);
    for (int i = 0; i < rows; ++i) {
        QStringList z;
        for (int j = 0; j < cols; ++j) z << "0";
        t += z.join(' ') + "\n";
    }
    input_->setPlainText(t);
    currentPath_.clear();
    input_->document()->setModified(false);
    updateTitle();
}

void MainWindow::openFile() {
    if (!maybeSave()) return;
    QString path = QFileDialog::getOpenFileName(this, "Open input", QString(),
                                                "Normaliz input (*.in);;All files (*)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        statusBar()->showMessage("Cannot open " + path);
        return;
    }
    input_->setPlainText(QString::fromUtf8(f.readAll()));
    currentPath_ = path;
    input_->document()->setModified(false);
    updateTitle();
    statusBar()->showMessage("Opened " + path);
}

void MainWindow::saveFile() {
    if (currentPath_.isEmpty()) { saveFileAs(); return; }
    QFile f(currentPath_);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        statusBar()->showMessage("Cannot save " + currentPath_);
        return;
    }
    f.write(input_->toPlainText().toUtf8());
    input_->document()->setModified(false);
    updateTitle();
    statusBar()->showMessage("Saved " + currentPath_);
}

void MainWindow::saveFileAs() {
    QString start = currentPath_.isEmpty() ? "untitled.in" : currentPath_;
    QString path = QFileDialog::getSaveFileName(this, "Save input", start,
                                                "Normaliz input (*.in);;All files (*)");
    if (path.isEmpty()) return;
    currentPath_ = path;
    saveFile();
}

// Worker thread. All exceptions are caught here; none may escape into Qt.
MainWindow::Result MainWindow::runCompute(std::string inputText, Goals g) {
    Result r;
    try {
        // Parse the .in text with Normaliz's own parser (same path as the CLI):
        // text -> InputMap<mpq_class> -> Cone<mpz_class>. Handles every input
        // type (cone, vertices, inequalities, equations, congruences, grading...).
        std::istringstream in(inputText);
        OptionsHandler options;
        std::map<NumParam::Param, long> num_param_input;
        std::map<PolyParam::Param, std::vector<std::string> > poly_param_input;
        renf_class_shared number_field = nullptr;
        InputMap<mpq_class> input =
            readNormalizInput<mpq_class>(in, options, num_param_input, poly_param_input, number_field);
        Cone<mpz_class> cone(input);

        if (!(g.hilbert || g.extreme || g.support || g.hseries || g.mult
              || g.volume || g.latpts || g.classgrp) && g.mode != 1)
            g.hilbert = true;

        ConeProperties props;
        if (g.hilbert) props.set(ConeProperty::HilbertBasis);
        if (g.extreme) props.set(ConeProperty::ExtremeRays);
        if (g.support) props.set(ConeProperty::SupportHyperplanes);
        if (g.hseries) props.set(ConeProperty::HilbertSeries);
        if (g.mult)    props.set(ConeProperty::Multiplicity);
        if (g.volume)   props.set(ConeProperty::Volume);
        if (g.latpts)   props.set(ConeProperty::NumberLatticePoints);
        if (g.classgrp) props.set(ConeProperty::ClassGroup);
        // Toolbar options.
        if (g.algo == 1)      props.set(ConeProperty::PrimalMode);
        else if (g.algo == 2) props.set(ConeProperty::DualMode);
        if (g.mode == 1) props.set(ConeProperty::DefaultMode);
        if (g.prec == 1) props.set(ConeProperty::BigInt);
        cone.compute(props);

        std::ostringstream oss;
        auto dumpMatrix = [&oss](const char* title, const std::vector<std::vector<mpz_class> >& m) {
            oss << m.size() << " " << title << ":\n";
            for (const std::vector<mpz_class>& v : m) {
                for (const mpz_class& x : v) oss << x << " ";
                oss << "\n";
            }
            oss << "\n";
        };
        if (g.hilbert) dumpMatrix("Hilbert basis elements", cone.getHilbertBasis());
        if (g.extreme) dumpMatrix("extreme rays", cone.getExtremeRays());
        if (g.support) dumpMatrix("support hyperplanes", cone.getSupportHyperplanes());
        if (g.hseries) oss << "Hilbert series:\n" << cone.getHilbertSeries() << "\n\n";
        if (g.mult)    oss << "multiplicity: " << cone.getMultiplicity() << "\n\n";
        if (g.volume)  oss << "volume: " << cone.getVolume() << "\n\n";
        if (g.latpts)  oss << "number of lattice points: " << cone.getNumberLatticePoints() << "\n\n";
        if (g.classgrp) {
            oss << "class group:";
            for (const mpz_class& x : cone.getClassGroup()) oss << " " << x;
            oss << "\n\n";
        }

        r.ok = true;
        r.text = oss.str();
    } catch (const InterruptException&) {
        r.stopped = true;
        r.text = "Computation stopped.";
    } catch (const std::exception& e) {
        r.text = std::string("Error: ") + e.what();
    } catch (...) {
        r.text = "Error: unknown exception in libnormaliz";
    }
    return r;
}

void MainWindow::startCompute() {
    Goals g{ cbHilbert_->isChecked(), cbExtreme_->isChecked(), cbSupport_->isChecked(),
             cbHSeries_->isChecked(), cbMult_->isChecked(),
             cbVolume_->isChecked(), cbLatPts_->isChecked(), cbClassGrp_->isChecked(),
             algoCombo_->currentIndex(), modeCombo_->currentIndex(), precCombo_->currentIndex() };
    // Capture the editor text on the GUI thread; the worker must not touch widgets.
    std::string inputText = input_->toPlainText().toStdString();
    nmz_interrupted = 0;   // clear any stale interrupt request from a previous Stop
    compute_->setEnabled(false);
    stop_->setEnabled(true);
    statusBar()->showMessage("Computing...");
    output_->setPlainText("");
    elapsedLabel_->setText("Elapsed: 0.0s");
    elapsed_.start();
    tick_->start();
    watcher_.setFuture(QtConcurrent::run(&MainWindow::runCompute, inputText, g));
}

void MainWindow::stopCompute() {
    // Request interruption. libnormaliz checks nmz_interrupted inside its loops
    // and throws InterruptException, which the worker catches.
    nmz_interrupted = 1;
    stop_->setEnabled(false);
    statusBar()->showMessage("Stopping...");
}

void MainWindow::computeFinished() {
    tick_->stop();
    elapsedLabel_->setText(QString("Elapsed: %1s").arg(elapsed_.elapsed() / 1000.0, 0, 'f', 1));
    const Result r = watcher_.result();
    output_->setPlainText(QString::fromStdString(r.text));
    statusBar()->showMessage(r.ok ? "Ready" : (r.stopped ? "Stopped" : "Error"));
    compute_->setEnabled(true);
    stop_->setEnabled(false);
}
