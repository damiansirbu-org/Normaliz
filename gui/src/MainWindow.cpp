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
#include <QtConcurrent>

#include <sstream>
#include <vector>
#include <map>

#include "libnormaliz/cone.h"
#include "libnormaliz/input.h"
#include "libnormaliz/HilbertSeries.h"

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
    gLay->addWidget(cbHilbert_);
    gLay->addWidget(cbExtreme_);
    gLay->addWidget(cbSupport_);
    gLay->addWidget(cbHSeries_);
    gLay->addWidget(cbMult_);

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
    statusBar()->showMessage("Ready");
    updateTitle();

    connect(compute_, &QPushButton::clicked, this, &MainWindow::startCompute);
    connect(&watcher_, &QFutureWatcher<Result>::finished, this, &MainWindow::computeFinished);
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
}

void MainWindow::updateTitle() {
    QString name = currentPath_.isEmpty() ? "untitled.in" : QFileInfo(currentPath_).fileName();
    setWindowTitle(QString("Normaliz - %1[*]").arg(name));
    setWindowModified(input_->document()->isModified());
}

// New: ask for a matrix size and load a zero-filled cone template to edit,
// mirroring jNormaliz's "New input" dialog.
void MainWindow::newFile() {
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

        if (!(g.hilbert || g.extreme || g.support || g.hseries || g.mult))
            g.hilbert = true;

        ConeProperties props;
        if (g.hilbert) props.set(ConeProperty::HilbertBasis);
        if (g.extreme) props.set(ConeProperty::ExtremeRays);
        if (g.support) props.set(ConeProperty::SupportHyperplanes);
        if (g.hseries) props.set(ConeProperty::HilbertSeries);
        if (g.mult)    props.set(ConeProperty::Multiplicity);
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

        r.ok = true;
        r.text = oss.str();
    } catch (const std::exception& e) {
        r.text = std::string("Error: ") + e.what();
    } catch (...) {
        r.text = "Error: unknown exception in libnormaliz";
    }
    return r;
}

void MainWindow::startCompute() {
    Goals g{ cbHilbert_->isChecked(), cbExtreme_->isChecked(), cbSupport_->isChecked(),
             cbHSeries_->isChecked(), cbMult_->isChecked() };
    // Capture the editor text on the GUI thread; the worker must not touch widgets.
    std::string inputText = input_->toPlainText().toStdString();
    compute_->setEnabled(false);
    statusBar()->showMessage("Computing...");
    output_->setPlainText("");
    watcher_.setFuture(QtConcurrent::run(&MainWindow::runCompute, inputText, g));
}

void MainWindow::computeFinished() {
    const Result r = watcher_.result();
    output_->setPlainText(QString::fromStdString(r.text));
    statusBar()->showMessage(r.ok ? "Ready" : "Error");
    compute_->setEnabled(true);
}
