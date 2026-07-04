#include "MainWindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QFrame>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QLabel>
#include <QSpinBox>
#include <QTabWidget>
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
#include <QApplication>
#include <QFont>
#include <QPrinter>
#include <QPrintDialog>
#include <QTemporaryDir>
#include <QDir>
#include <QFileInfoList>
#include <QtConcurrent>

#include <sstream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#ifdef _OPENMP
#include <omp.h>
#endif

#include "libnormaliz/cone.h"
#include "libnormaliz/input.h"
#include "libnormaliz/input_type.h"
#include "libnormaliz/output.h"
#include "libnormaliz/HilbertSeries.h"
#include "libnormaliz/normaliz_exception.h"
#include "libnormaliz/general.h"

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <psapi.h>
#elif defined(__linux__)
#  include <cstdio>
#  include <unistd.h>
#endif

using namespace libnormaliz;

#ifndef NMZ_GUI_VERSION
#  define NMZ_GUI_VERSION "dev"
#endif

namespace {

// RAII: send libnormaliz verbose output to a buffer for the Console tab, and
// restore the previous state (even on exception) so no dangling stream remains.
// The error stream is captured too: the engine prints "ERROR: ..." details to
// errorOutput() before throwing, and this app has no console to show cerr.
struct VerboseCapture {
    bool old_;
    explicit VerboseCapture(std::ostream& s) {
        old_ = setVerboseDefault(true);
        setVerboseOutput(s);
        setErrorOutput(s);
    }
    ~VerboseCapture() {
        setVerboseDefault(old_);
        setVerboseOutput(std::cout);
        setErrorOutput(std::cerr);
    }
};

// Resident memory of this process, for the status-bar gauge (empty on macOS).
QString processMemoryText() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return QString("Mem: %1 MB").arg(pmc.WorkingSetSize / (1024 * 1024));
    return QString();
#elif defined(__linux__)
    long rssPages = 0, totalPages = 0;
    if (FILE* f = std::fopen("/proc/self/statm", "r")) {
        if (std::fscanf(f, "%ld %ld", &totalPages, &rssPages) != 2) rssPages = 0;
        std::fclose(f);
    }
    long bytes = rssPages * sysconf(_SC_PAGESIZE);
    return QString("Mem: %1 MB").arg(bytes / (1024 * 1024));
#else
    return QString();
#endif
}

// The computation goals offered as checkboxes in the UI. Each maps a checkbox to
// a ConeProperty; the worker adds them to compute(). This is a convenience subset
// - any ConeProperty can also be requested by typing its name in the .in editor.
// Results are rendered in full by renderFullOutput(), so no per-goal formatter is
// needed; add a row here only to surface a goal as a checkbox.
struct GoalDef { ConeProperty::Enum prop; const char* label; bool byDefault; };
const std::vector<GoalDef>& goalTable() {
    static const std::vector<GoalDef> t = {
        {ConeProperty::HilbertBasis,        "Hilbert basis",         true},
        {ConeProperty::ExtremeRays,         "Extreme rays",          false},
        {ConeProperty::SupportHyperplanes,  "Support hyperplanes",   false},
        {ConeProperty::ModuleGenerators,    "Module generators",     false},
        {ConeProperty::Deg1Elements,        "Degree-1 elements",     false},
        {ConeProperty::MaximalSubspace,     "Maximal subspace",      false},
        {ConeProperty::HilbertSeries,       "Hilbert series",        false},
        {ConeProperty::EhrhartSeries,       "Ehrhart series",        false},
        {ConeProperty::Multiplicity,        "Multiplicity",          false},
        {ConeProperty::Volume,              "Volume",                false},
        {ConeProperty::NumberLatticePoints, "Lattice points",        false},
        {ConeProperty::TriangulationSize,   "Triangulation size",    false},
        {ConeProperty::ClassGroup,          "Class group",           false},
        {ConeProperty::Grading,             "Grading",               false},
        {ConeProperty::Dehomogenization,    "Dehomogenization",      false},
        {ConeProperty::Rank,                "Rank",                  false},
        {ConeProperty::EmbeddingDim,        "Embedding dimension",   false},
        {ConeProperty::RecessionRank,       "Recession rank",        false},
        {ConeProperty::IsPointed,           "Is pointed",            false},
        {ConeProperty::IsGorenstein,        "Is Gorenstein",         false},
        {ConeProperty::IsDeg1ExtremeRays,   "Deg-1 extreme rays",    false},
        {ConeProperty::Automorphisms,       "Automorphism group",    false},
        {ConeProperty::Integral,            "Integral (of polynomial)", false},
        {ConeProperty::VirtualMultiplicity, "Virtual multiplicity",  false},
        {ConeProperty::WeightedEhrhartSeries, "Weighted Ehrhart series", false},
    };
    return t;
}

// Algebraic (real embedded number field) input runs on Cone<renf_elem_class>,
// where only part of the goals make sense. Ask the engine's own check
// (ConeProperties::check_Q_permissible, the one Cone::compute enforces) per
// property instead of keeping a hand-written whitelist - the hand-kept list
// used to drop goals the engine does support on renf (LatticePoints,
// Triangulation, Automorphisms, ModuleGenerators, EuclideanVolume, ...).
bool renfApplicable(ConeProperty::Enum p) {
    ConeProperties one;
    one.set(p);
    std::ostringstream mute;   // the check prints the offender before throwing
    setErrorOutput(mute);
    bool ok = true;
    try {
        one.check_Q_permissible(true);
    } catch (const BadInputException&) {
        ok = false;
    }
    setErrorOutput(std::cerr);
    return ok;
}

// Extra input (add_inequalities / add_cone / ...) is stripped before the Cone
// is built and applied after the main computation via modifyCone, exactly as
// the CLI does (extract_additional_input in normaliz.cpp). Left in place, the
// Cone constructor silently ignores it and the result differs from the CLI.
template <class Number>
InputMap<Number> extractAdditionalInput(InputMap<Number>& input) {
    static const std::pair<Type::InputType, Type::InputType> moves[] = {
        {Type::add_inequalities, Type::inequalities},
        {Type::add_equations, Type::equations},
        {Type::add_inhom_inequalities, Type::inhom_inequalities},
        {Type::add_inhom_equations, Type::inhom_equations},
        {Type::add_cone, Type::cone},
        {Type::add_subspace, Type::subspace},
        {Type::add_vertices, Type::vertices},
    };
    InputMap<Number> add;
    for (const auto& m : moves) {
        auto it = input.find(m.first);
        if (it != input.end()) {
            add[m.second] = it->second;
            input.erase(it);
        }
    }
    return add;
}

// Thread limit for a run; 0 = auto. set_thread_limit is sticky in the engine
// (parallelization_set stays true), so going back to auto must restore the
// engine's own default cap explicitly - otherwise the last explicit value
// silently persists for the rest of the session.
void applyThreadLimit(int threads) {
    // Captured on the first run, before anything called omp_set_num_threads.
    static const int autoLimit =
#ifdef _OPENMP
        std::min(omp_get_max_threads(), default_thread_limit);
#else
        default_thread_limit;
#endif
    static bool everSet = false;
    if (threads > 0) {
        set_thread_limit(threads);
        everSet = true;
    }
    else if (everSet) {
        set_thread_limit(autoLimit);
        everSet = false;
    }
}

// Render the complete Normaliz result the same way the CLI writes the .out file,
// so every computed property shows. The Output class only writes to files, so
// render into a fresh QTemporaryDir (system temp via QDir::tempPath -> TMPDIR /
// GetTempPath, valid on all three platforms) and read everything back:
//  - a unique directory per run: two GUI instances cannot collide;
//  - write_files also emits side files for some properties (.tri/.tgn for
//    Triangulation, .aut for Automorphisms, .fac, .dec, fusion files, ...);
//    they are appended to the text so the data is actually visible, and the
//    directory (RAII) removes every file even if write_files throws.
template <class Integer>
std::string renderFullOutput(Cone<Integer>& cone, renf_class_shared nf) {
    QTemporaryDir tmp;
    if (!tmp.isValid())
        return "Error: cannot create a temporary directory for rendering the result.";
    const QString base = tmp.path() + "/result";
    Output<Integer> Out;
    Out.set_name(std::string(base.toLocal8Bit().constData()));
    Out.set_write_out(true);
    Out.set_renf(nf);            // no-op when nf is null (rational input)
    Out.setCone(cone);
    Out.write_files();

    std::string text;
    QFile outFile(base + ".out");
    if (outFile.open(QIODevice::ReadOnly))
        text = outFile.readAll().toStdString();

    // Side files (everything except result.out), e.g. the triangulation itself.
    const QFileInfoList side = QDir(tmp.path()).entryInfoList(QDir::Files, QDir::Name);
    for (const QFileInfo& fi : side) {
        if (fi.fileName() == "result.out")
            continue;
        QFile f(fi.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly))
            continue;
        text += "\n==================== additional output (";
        text += fi.fileName().toStdString();
        text += ") ====================\n";
        text += f.readAll().toStdString();
    }
    if (text.empty())
        text = "Error: the computation produced no output.";
    return text;
}

}  // namespace

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

    // The goal checkboxes live in a scroll area (there are many).
    auto* goalsScroll = new QScrollArea();
    goalsScroll->setWidgetResizable(true);
    goalsScroll->setFrameShape(QFrame::NoFrame);
    goalsScroll->setMinimumHeight(240);
    auto* goalsInner = new QWidget();
    auto* giLay = new QVBoxLayout(goalsInner);
    giLay->setContentsMargins(0, 0, 0, 0);
    for (const GoalDef& gd : goalTable()) {
        auto* cb = new QCheckBox(gd.label);
        cb->setChecked(gd.byDefault);
        giLay->addWidget(cb);
        cbGoals_.push_back(cb);
    }
    giLay->addStretch();
    goalsScroll->setWidget(goalsInner);
    gLay->addWidget(goalsScroll, 1);

    // Backend selector. Local (embedded libnormaliz) is the default and the only
    // implemented mode; Cloud (the distributed remote backend) is shown but
    // disabled - the architecture is in place, the implementation is WIP.
    // See gui/doc/architecture.md, section "Shared core (desktop and web)".
    auto* backendLabel = new QLabel("Backend");
    backendLabel->setObjectName("sectionLabel");
    gLay->addWidget(backendLabel);
    backendLocal_ = new QRadioButton("Local");
    backendLocal_->setChecked(true);
    backendLocal_->setToolTip("Compute in this application (embedded engine)");
    backendRemote_ = new QRadioButton("Cloud (WIP)");
    backendRemote_->setEnabled(false);
    backendRemote_->setToolTip("Distributed cloud backend - work in progress");
    gLay->addWidget(backendLocal_);
    gLay->addWidget(backendRemote_);

    compute_ = new QPushButton("Compute");
    compute_->setObjectName("compute");
    gLay->addWidget(compute_);
    stop_ = new QPushButton("Stop");
    stop_->setEnabled(false);
    gLay->addWidget(stop_);
    top->addWidget(goalsBox, 2);

    root->addLayout(top, 3);

    // Tabbed panel: Output / Console / Options.
    tabs_ = new QTabWidget();
    output_ = new QPlainTextEdit();
    output_->setReadOnly(true);
    console_ = new QPlainTextEdit();
    console_->setReadOnly(true);
    tabs_->addTab(output_, "Output");
    tabs_->addTab(console_, "Console");

    auto* optWidget = new QWidget();
    auto* optForm = new QFormLayout(optWidget);
    threadsSpin_ = new QSpinBox();
    threadsSpin_->setRange(0, 256);
    threadsSpin_->setValue(0);
    threadsSpin_->setToolTip("Maximum parallel threads; 0 = automatic");
    optForm->addRow("Max threads (0 = auto):", threadsSpin_);
    fontSpin_ = new QSpinBox();
    fontSpin_->setRange(8, 28);
    fontSpin_->setValue(10);
    optForm->addRow("Font size:", fontSpin_);
    auto* optNote = new QLabel(
        "The engine runs fully in process (libnormaliz with nauty, e-antic and "
        "CoCoALib). Besides the checkboxes, any ConeProperty can be requested by "
        "typing its name in the input editor; the complete Normaliz output is shown.");
    optNote->setWordWrap(true);
    optForm->addRow(optNote);
    tabs_->addTab(optWidget, "Options");

    root->addWidget(tabs_, 2);

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

    memLabel_ = new QLabel();
    statusBar()->addPermanentWidget(memLabel_);
    elapsedLabel_ = new QLabel("Elapsed: 0.0s");
    statusBar()->addPermanentWidget(elapsedLabel_);
    statusBar()->showMessage("Ready");
    tick_ = new QTimer(this);
    tick_->setInterval(100);
    memTick_ = new QTimer(this);
    memTick_->setInterval(1000);

    input_->document()->setModified(false);   // the preloaded example is not "unsaved"
    updateTitle();

    connect(compute_, &QPushButton::clicked, this, &MainWindow::startCompute);
    connect(stop_, &QPushButton::clicked, this, &MainWindow::stopCompute);
    connect(&watcher_, &QFutureWatcher<Result>::finished, this, &MainWindow::computeFinished);
    connect(tick_, &QTimer::timeout, this, [this] {
        elapsedLabel_->setText(QString("Elapsed: %1s").arg(elapsed_.elapsed() / 1000.0, 0, 'f', 1));
    });
    connect(memTick_, &QTimer::timeout, this, [this] { memLabel_->setText(processMemoryText()); });
    memLabel_->setText(processMemoryText());
    memTick_->start();
    connect(fontSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int pt) {
        for (QPlainTextEdit* w : {input_, output_, console_}) {
            QFont f = w->font();
            f.setPointSize(pt);
            w->setFont(f);
        }
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
    fileMenu->addAction("&Close", this, &MainWindow::closeFile);
    QAction* aSave = fileMenu->addAction("&Save", this, &MainWindow::saveFile);
    aSave->setShortcut(QKeySequence::Save);
    QAction* aSaveAs = fileMenu->addAction("Save &As...", this, &MainWindow::saveFileAs);
    aSaveAs->setShortcut(QKeySequence::SaveAs);
    fileMenu->addSeparator();
    QAction* aPrint = fileMenu->addAction("&Print...", this, &MainWindow::printCurrent);
    aPrint->setShortcut(QKeySequence::Print);
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
    helpMenu->addAction("&Help", this, [this] {
        QMessageBox::information(this, "Normaliz GUI - Help",
            "<b>How to use</b><br>"
            "1. Enter a Normaliz input in the Input (.in) editor (or File &gt; Open).<br>"
            "2. Tick the computation goals.<br>"
            "3. Press Compute; use Stop to cancel.<br><br>"
            "Any of libnormaliz's ~150 computation goals (ConeProperty) can also be "
            "requested by typing its name on a line in the input editor, exactly as "
            "on the normaliz command line - the checkboxes are just a convenience "
            "subset.<br><br>"
            "The toolbar sets algorithm, mode and precision. The Console tab shows "
            "the engine log; the Options tab sets threads and font size.<br><br>"
            "Full documentation is in the gui/doc folder.");
    });
    helpMenu->addSeparator();
    helpMenu->addAction("Normaliz &website", this, [] {
        QDesktopServices::openUrl(QUrl("https://github.com/Normaliz/Normaliz"));
    });
    helpMenu->addAction("Normaliz &manual", this, [] {
        QDesktopServices::openUrl(QUrl("https://github.com/Normaliz/Normaliz/blob/master/doc/Normaliz.pdf"));
    });
    helpMenu->addAction("Mathematical &background", this, [this] {
        QMessageBox::information(this, "Mathematical background",
            "<b>Normaliz</b> computes Hilbert bases of rational cones and the "
            "normalization of affine monoids, Hilbert/Ehrhart series, volumes and "
            "lattice points.<br><br>"
            "Key algorithm: pyramid decomposition (Bruns, Ichim, Soeger).<br><br>"
            "Papers and documentation: "
            "<a href='https://github.com/Normaliz/Normaliz'>github.com/Normaliz/Normaliz</a>.");
    });
    helpMenu->addSeparator();
    helpMenu->addAction("&About", this, [this] {
        QMessageBox::about(this, "About Normaliz GUI",
            "<b>Normaliz GUI</b> version " NMZ_GUI_VERSION "<br>"
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
    // Do not tear the process down under a running worker: stop the engine and
    // wait for it (jNormaliz offered the same interrupt-on-exit).
    if (watcher_.isRunning()) {
        QMessageBox::StandardButton ret = QMessageBox::question(
            this, "Normaliz", "A computation is running.\nStop it and exit?",
            QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) {
            event->ignore();
            return;
        }
        nmz_interrupted = 1;
        watcher_.waitForFinished();
    }
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}

// New: ask for a matrix size and load a zero-filled cone template to edit,
// mirroring jNormaliz's "New input" dialog.
// Changing the input while the worker holds a copy and is about to fill the
// output pane is a confusing workflow (jNormaliz disabled it too); refuse until
// the run finishes or is stopped.
bool MainWindow::busyGuard() {
    if (watcher_.isRunning()) {
        statusBar()->showMessage("A computation is running - stop it first.");
        return true;
    }
    return false;
}

void MainWindow::newFile() {
    if (busyGuard()) return;
    if (!maybeSave()) return;
    // Caps keep the generated template at a size the editor can handle; a
    // hand-edited skeleton beyond 500 x 5000 is not a realistic workflow.
    bool ok = false;
    int cols = QInputDialog::getInt(this, "New input", "Ambient dimension (columns):",
                                    2, 1, 500, 1, &ok);
    if (!ok) return;
    int rows = QInputDialog::getInt(this, "New input", "Number of generators (rows):",
                                    2, 1, 5000, 1, &ok);
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
    if (busyGuard()) return;
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

void MainWindow::closeFile() {
    if (busyGuard()) return;
    if (!maybeSave()) return;
    input_->clear();
    output_->clear();
    console_->clear();
    currentPath_.clear();
    input_->document()->setModified(false);
    updateTitle();
    statusBar()->showMessage("Closed");
}

void MainWindow::printCurrent() {
    // jNormaliz printed the selected tab; print the visible Output/Console tab
    // when it has content, otherwise the input editor.
    QPlainTextEdit* target = input_;
    if (tabs_->currentWidget() == output_ && !output_->toPlainText().isEmpty())
        target = output_;
    else if (tabs_->currentWidget() == console_ && !console_->toPlainText().isEmpty())
        target = console_;
    QPrinter printer;
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() != QDialog::Accepted) return;
    target->print(&printer);
}

// Worker thread. All exceptions are caught here; none may escape into Qt.
MainWindow::Result MainWindow::runCompute(std::string inputText, Goals g) {
    Result r;
    std::ostringstream vlog;
    try {
        // Parse the .in text with Normaliz's own parser (same path as the CLI).
        // The mpq parse handles every rational input type; if the input declares a
        // number_field it throws NumberFieldInputException and we re-parse on the
        // algebraic (renf_elem_class) path, exactly as normaliz.cpp dispatches.
        OptionsHandler options;
        std::map<NumParam::Param, long> num_param_input;
        std::map<PolyParam::Param, std::vector<std::string> > poly_param_input;
        renf_class_shared number_field = nullptr;

        const std::vector<GoalDef>& tbl = goalTable();
        std::vector<ConeProperty::Enum> selected;
        for (int i : g.idx)
            if (i >= 0 && i < (int)tbl.size()) selected.push_back(tbl[i].prop);

        bool algebraic = false;
        InputMap<mpq_class> input;
        InputMap<renf_elem_class> renf_input;
        try {
            std::istringstream in(inputText);
            input = readNormalizInput<mpq_class>(in, options, num_param_input, poly_param_input, number_field);
        } catch (const NumberFieldInputException&) {
            std::istringstream in(inputText);
            renf_input = readNormalizInput<renf_elem_class>(in, options, num_param_input, poly_param_input, number_field);
            algebraic = true;
        }

        // A Stop pressed during parsing must hold: Cone::compute clears
        // nmz_interrupted on entry, so it has to be honored here.
        if (nmz_interrupted)
            throw InterruptException("stopped before the computation started");

        applyThreadLimit(g.threads);

        std::ostringstream oss;
        std::string notComputable;

        // Everything the user asked for: the ticked goals plus any goal names
        // typed directly in the .in editor (options.getToCompute()) - the same
        // console-style access the CLI gives, so every ConeProperty is reachable
        // by name without a checkbox for each. The result is rendered with
        // Normaliz's own Output writer, so all computed properties are shown.
        ConeProperties requested;
        for (ConeProperty::Enum p : selected) requested.set(p);
        requested.set(options.getToCompute());

        if (algebraic) {
            InputMap<renf_elem_class> add_input = extractAdditionalInput(renf_input);
            Cone<renf_elem_class> cone(renf_input);
            cone.setRenf(number_field);
            // Polynomial / numerical parameters apply on the algebraic path too
            // (the CLI sets them for both cone types in compute_and_output).
            cone.setPolyParams(poly_param_input);
            cone.setNumericalParams(num_param_input);
            // Only the engine-permitted (geometric) goals apply on renf.
            ConeProperties props;
            int skipped = 0;
            for (size_t i = 0; i < ConeProperty::EnumSize; ++i) {
                ConeProperty::Enum e = static_cast<ConeProperty::Enum>(i);
                if (!requested.test(e)) continue;
                if (renfApplicable(e)) props.set(e); else ++skipped;
            }
            if (!props.any() && g.mode != 1) props.set(ConeProperty::SupportHyperplanes);
            if (g.algo == 1)      props.set(ConeProperty::PrimalMode);
            else if (g.algo == 2) props.set(ConeProperty::DualMode);
            if (g.mode == 1) props.set(ConeProperty::DefaultMode);
            {
                VerboseCapture vc(vlog);
                try {
                    cone.compute(props);
                    if (!add_input.empty()) {   // apply add_* input as the CLI does
                        cone.modifyCone(add_input);
                        ConeProperties after;
                        after.set(ConeProperty::SupportHyperplanes);
                        cone.compute(after);
                    }
                } catch (const NotComputableException& e) {
                    notComputable = e.what();   // still render what was computed
                }
            }
            // A Stop arriving after compute finished must not poison the
            // rendering (Output may run extra computations); the CLI resets
            // the flag before write_files for the same reason.
            nmz_interrupted = 0;
            if (!notComputable.empty())
                oss << "Not all requested goals could be computed:\n" << notComputable
                    << "\nShowing the available results.\n\n";
            oss << renderFullOutput(cone, number_field);
            if (skipped > 0)
                oss << "\n(" << skipped << " requested goal(s) not applicable to "
                       "algebraic input were skipped.)\n";
            if (g.prec == 1)
                oss << "\n(BigInt is not applicable to algebraic input; ignored.)\n";
        } else {
            InputMap<mpq_class> add_input = extractAdditionalInput(input);
            Cone<mpz_class> cone(input);
            // An integrand polynomial / numerical parameters, if the input declared
            // them (needed for Integral, weighted Ehrhart via CoCoALib).
            cone.setPolyParams(poly_param_input);
            cone.setNumericalParams(num_param_input);
            ConeProperties props = requested;
            if (!props.any() && g.mode != 1) props.set(ConeProperty::HilbertBasis);
            if (g.algo == 1)      props.set(ConeProperty::PrimalMode);
            else if (g.algo == 2) props.set(ConeProperty::DualMode);
            if (g.mode == 1) props.set(ConeProperty::DefaultMode);
            if (g.prec == 1) props.set(ConeProperty::BigInt);
            {
                VerboseCapture vc(vlog);   // capture verbose output for the Console tab
                try {
                    cone.compute(props);
                    if (!add_input.empty()) {   // apply add_* input as the CLI does
                        cone.modifyCone(add_input);
                        ConeProperties after;
                        after.set(ConeProperty::SupportHyperplanes);
                        cone.compute(after);
                    }
                } catch (const NotComputableException& e) {
                    notComputable = e.what();   // still render what was computed
                }
            }
            nmz_interrupted = 0;   // see the comment on the algebraic branch
            if (!notComputable.empty())
                oss << "Not all requested goals could be computed:\n" << notComputable
                    << "\nShowing the available results.\n\n";
            oss << renderFullOutput(cone, nullptr);
            if (options.isUseLongLong())
                oss << "\n(LongLong from the input is ignored: the GUI always "
                       "computes with arbitrary precision.)\n";
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
    r.console = vlog.str();
    return r;
}

// Headless entry point for the hidden `--run <in> <out>` flag. Runs the same
// worker as the Compute button with default toolbar settings (goals come from
// the .in text, like the CLI) and returns the rendered Output text.
std::string MainWindow::runHeadless(const std::string& inputText, bool defaultMode) {
    Goals g;
    g.algo = 0; g.mode = defaultMode ? 1 : 0; g.prec = 0; g.threads = 0;
    nmz_interrupted = 0;
    Result r = runCompute(inputText, g);
    return r.text;
}

void MainWindow::startCompute() {
    Goals g;
    for (int i = 0; i < (int)cbGoals_.size(); ++i)
        if (cbGoals_[i]->isChecked()) g.idx.push_back(i);
    g.algo = algoCombo_->currentIndex();
    g.mode = modeCombo_->currentIndex();
    g.prec = precCombo_->currentIndex();
    g.threads = threadsSpin_->value();
    // Capture the editor text on the GUI thread; the worker must not touch widgets.
    std::string inputText = input_->toPlainText().toStdString();
    nmz_interrupted = 0;   // clear any stale interrupt request from a previous Stop
    compute_->setEnabled(false);
    stop_->setEnabled(true);
    statusBar()->showMessage("Computing...");
    output_->setPlainText("");
    console_->setPlainText("");
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

// Demo/screenshot hook (hidden `--demo <in> <png>` in main.cpp): load the
// input, run the real Compute path, and when it finishes switch to the Output
// tab, save a screenshot and quit. Reuses startCompute/computeFinished so the
// screenshot shows a genuine result, not a mock-up.
void MainWindow::demoShot(const QString& inputText, const QString& pngPath) {
    input_->setPlainText(inputText);
    input_->document()->setModified(false);
    updateTitle();
    connect(&watcher_, &QFutureWatcher<Result>::finished, this, [this, pngPath]() {
        tabs_->setCurrentWidget(output_);   // computeFinished already filled it
        QTimer::singleShot(200, this, [this, pngPath]() {
            grab().save(pngPath);
            QApplication::quit();
        });
    });
    QTimer::singleShot(300, this, &MainWindow::startCompute);
}

void MainWindow::computeFinished() {
    tick_->stop();
    elapsedLabel_->setText(QString("Elapsed: %1s").arg(elapsed_.elapsed() / 1000.0, 0, 'f', 1));
    const Result r = watcher_.result();
    // Guard against a pathologically large result freezing the UI: setPlainText
    // on hundreds of MB blocks the event loop. Show a bounded head with a note
    // (normal outputs are a few KB, so this never triggers in practice).
    const size_t kMaxDisplay = 4 * 1024 * 1024;
    if (r.text.size() > kMaxDisplay) {
        QString msg = QString::fromStdString(r.text.substr(0, kMaxDisplay)) +
            QString("\n\n[... output truncated for display: %1 of %2 characters shown; "
                    "the full result is too large to render without freezing the window. "
                    "Request fewer goals or run a specific property.]")
                .arg(kMaxDisplay).arg(r.text.size());
        output_->setPlainText(msg);
    } else {
        output_->setPlainText(QString::fromStdString(r.text));
    }
    console_->setPlainText(QString::fromStdString(r.console));
    statusBar()->showMessage(r.ok ? "Ready" : (r.stopped ? "Stopped" : "Error"));
    compute_->setEnabled(true);
    stop_->setEnabled(false);
}
