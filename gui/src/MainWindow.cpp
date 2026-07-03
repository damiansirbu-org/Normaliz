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
#include <QFont>
#include <QPrinter>
#include <QPrintDialog>
#include <QtConcurrent>

#include <sstream>
#include <vector>
#include <map>
#include <iostream>

#include "libnormaliz/cone.h"
#include "libnormaliz/input.h"
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

namespace {

// RAII: send libnormaliz verbose output to a buffer for the Console tab, and
// restore the previous state (even on exception) so no dangling stream remains.
struct VerboseCapture {
    bool old_;
    explicit VerboseCapture(std::ostream& s) {
        old_ = setVerboseDefault(true);
        setVerboseOutput(s);
    }
    ~VerboseCapture() {
        setVerboseDefault(old_);
        setVerboseOutput(std::cout);
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

// The computation goals offered in the UI. Each maps a checkbox to a
// ConeProperty; the worker sets them on compute() and formatGoal() prints the
// result. Add a goal here and add a case in formatGoal(); nothing else changes.
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
    };
    return t;
}

void dumpMatrix(std::ostream& o, const char* title, const std::vector<std::vector<mpz_class> >& m) {
    o << m.size() << " " << title << ":\n";
    for (const std::vector<mpz_class>& v : m) {
        for (const mpz_class& x : v) o << x << " ";
        o << "\n";
    }
    o << "\n";
}

void dumpVector(std::ostream& o, const char* title, const std::vector<mpz_class>& v) {
    o << title << ":";
    for (const mpz_class& x : v) o << " " << x;
    o << "\n\n";
}

// Format a computed goal. Called only for goals that were requested and thus
// computed, so the getters do not throw.
std::string formatGoal(Cone<mpz_class>& cone, ConeProperty::Enum p) {
    std::ostringstream o;
    switch (p) {
        case ConeProperty::HilbertBasis:        dumpMatrix(o, "Hilbert basis elements", cone.getHilbertBasis()); break;
        case ConeProperty::ExtremeRays:         dumpMatrix(o, "extreme rays", cone.getExtremeRays()); break;
        case ConeProperty::SupportHyperplanes:  dumpMatrix(o, "support hyperplanes", cone.getSupportHyperplanes()); break;
        case ConeProperty::ModuleGenerators:    dumpMatrix(o, "module generators", cone.getModuleGenerators()); break;
        case ConeProperty::Deg1Elements:        dumpMatrix(o, "degree 1 elements", cone.getDeg1Elements()); break;
        case ConeProperty::MaximalSubspace:     dumpMatrix(o, "maximal subspace generators", cone.getMaximalSubspace()); break;
        case ConeProperty::HilbertSeries:       o << "Hilbert series:\n" << cone.getHilbertSeries() << "\n\n"; break;
        case ConeProperty::EhrhartSeries:       o << "Ehrhart series:\n" << cone.getEhrhartSeries() << "\n\n"; break;
        case ConeProperty::Multiplicity:        o << "multiplicity: " << cone.getMultiplicity() << "\n\n"; break;
        case ConeProperty::Volume:              o << "volume: " << cone.getVolume() << "\n\n"; break;
        case ConeProperty::NumberLatticePoints: o << "number of lattice points: " << cone.getNumberLatticePoints() << "\n\n"; break;
        case ConeProperty::TriangulationSize:   o << "triangulation size: " << cone.getTriangulationSize() << "\n\n"; break;
        case ConeProperty::ClassGroup:          dumpVector(o, "class group", cone.getClassGroup()); break;
        case ConeProperty::Grading:             dumpVector(o, "grading", cone.getGrading()); break;
        case ConeProperty::Dehomogenization:    dumpVector(o, "dehomogenization", cone.getDehomogenization()); break;
        case ConeProperty::Rank:                o << "rank: " << cone.getRank() << "\n\n"; break;
        case ConeProperty::EmbeddingDim:        o << "embedding dimension: " << cone.getEmbeddingDim() << "\n\n"; break;
        case ConeProperty::RecessionRank:       o << "recession rank: " << cone.getRecessionRank() << "\n\n"; break;
        case ConeProperty::IsPointed:           o << "pointed: " << (cone.isPointed() ? "yes" : "no") << "\n\n"; break;
        case ConeProperty::IsGorenstein:        o << "Gorenstein: " << (cone.isGorenstein() ? "yes" : "no") << "\n\n"; break;
        case ConeProperty::IsDeg1ExtremeRays:   o << "degree-1 extreme rays: " << (cone.isDeg1ExtremeRays() ? "yes" : "no") << "\n\n"; break;
        case ConeProperty::Automorphisms: {
            const AutomorphismGroup<mpz_class>& A = cone.getAutomorphismGroup();
            const std::vector<std::vector<key_t> >& perms = A.getGensPerms();
            o << "automorphism group order: " << A.getOrder() << "\n";
            o << perms.size() << " generating permutation(s) of the extreme rays:\n";
            for (const std::vector<key_t>& perm : perms) {
                for (key_t x : perm) o << x << " ";
                o << "\n";
            }
            o << "\n";
            break;
        }
        default:                                o << "computed.\n\n"; break;
    }
    return o.str();
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
    auto* tabs = new QTabWidget();
    output_ = new QPlainTextEdit();
    output_->setReadOnly(true);
    console_ = new QPlainTextEdit();
    console_->setReadOnly(true);
    tabs->addTab(output_, "Output");
    tabs->addTab(console_, "Console");

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
        "Output-file options and NmzIntegrate are not applicable here: the engine "
        "runs in process (no .out files) and this build has no CoCoALib.");
    optNote->setWordWrap(true);
    optForm->addRow(optNote);
    tabs->addTab(optWidget, "Options");

    root->addWidget(tabs, 2);

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

void MainWindow::closeFile() {
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
    QPrinter printer;
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() != QDialog::Accepted) return;
    input_->print(&printer);   // print the .in input
}

// Worker thread. All exceptions are caught here; none may escape into Qt.
MainWindow::Result MainWindow::runCompute(std::string inputText, Goals g) {
    Result r;
    std::ostringstream vlog;
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

        // Resolve the requested goals (default to Hilbert basis when none are
        // ticked and DefaultMode is off).
        const std::vector<GoalDef>& tbl = goalTable();
        std::vector<ConeProperty::Enum> selected;
        for (int i : g.idx)
            if (i >= 0 && i < (int)tbl.size()) selected.push_back(tbl[i].prop);
        if (selected.empty() && g.mode != 1)
            selected.push_back(ConeProperty::HilbertBasis);

        ConeProperties props;
        for (ConeProperty::Enum p : selected) props.set(p);
        // Toolbar options.
        if (g.algo == 1)      props.set(ConeProperty::PrimalMode);
        else if (g.algo == 2) props.set(ConeProperty::DualMode);
        if (g.mode == 1) props.set(ConeProperty::DefaultMode);
        if (g.prec == 1) props.set(ConeProperty::BigInt);

        if (g.threads > 0)
            set_thread_limit(g.threads);

        {
            VerboseCapture vc(vlog);   // capture verbose output for the Console tab
            cone.compute(props);
        }

        std::ostringstream oss;
        for (ConeProperty::Enum p : selected)
            oss << formatGoal(cone, p);

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

void MainWindow::computeFinished() {
    tick_->stop();
    elapsedLabel_->setText(QString("Elapsed: %1s").arg(elapsed_.elapsed() / 1000.0, 0, 'f', 1));
    const Result r = watcher_.result();
    output_->setPlainText(QString::fromStdString(r.text));
    console_->setPlainText(QString::fromStdString(r.console));
    statusBar()->showMessage(r.ok ? "Ready" : (r.stopped ? "Stopped" : "Error"));
    compute_->setEnabled(true);
    stop_->setEnabled(false);
}
