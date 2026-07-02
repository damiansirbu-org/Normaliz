#include "MainWindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QStatusBar>
#include <QtConcurrent>

#include <sstream>
#include <vector>

#include "libnormaliz/cone.h"

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
    gLay->addWidget(cbHilbert_);
    gLay->addWidget(cbExtreme_);
    gLay->addWidget(cbSupport_);
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
    setWindowTitle("Normaliz");
    statusBar()->showMessage("Ready");

    connect(compute_, &QPushButton::clicked, this, &MainWindow::startCompute);
    connect(&watcher_, &QFutureWatcher<Result>::finished, this, &MainWindow::computeFinished);
}

// Worker thread. All exceptions are caught here; none may escape into Qt.
MainWindow::Result MainWindow::runCompute(Goals g) {
    Result r;
    try {
        // Input is fixed to the 2cone example for now; parsing the .in editor
        // (readNormalizInput) is the next step.
        std::vector<std::vector<mpz_class> > rays;
        rays.push_back(std::vector<mpz_class>{1, 3});
        rays.push_back(std::vector<mpz_class>{2, 1});
        Cone<mpz_class> cone(Type::cone, rays);

        if (!(g.hilbert || g.extreme || g.support)) g.hilbert = true;

        ConeProperties props;
        if (g.hilbert) props.set(ConeProperty::HilbertBasis);
        if (g.extreme) props.set(ConeProperty::ExtremeRays);
        if (g.support) props.set(ConeProperty::SupportHyperplanes);
        cone.compute(props);

        std::ostringstream oss;
        auto dump = [&oss](const char* title, const std::vector<std::vector<mpz_class> >& m) {
            oss << m.size() << " " << title << ":\n";
            for (const std::vector<mpz_class>& v : m) {
                for (const mpz_class& x : v) oss << x << " ";
                oss << "\n";
            }
            oss << "\n";
        };
        if (g.hilbert) dump("Hilbert basis elements", cone.getHilbertBasis());
        if (g.extreme) dump("extreme rays", cone.getExtremeRays());
        if (g.support) dump("support hyperplanes", cone.getSupportHyperplanes());

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
    Goals g{ cbHilbert_->isChecked(), cbExtreme_->isChecked(), cbSupport_->isChecked() };
    compute_->setEnabled(false);
    statusBar()->showMessage("Computing...");
    output_->setPlainText("");
    watcher_.setFuture(QtConcurrent::run(&MainWindow::runCompute, g));
}

void MainWindow::computeFinished() {
    const Result r = watcher_.result();
    output_->setPlainText(QString::fromStdString(r.text));
    statusBar()->showMessage(r.ok ? "Ready" : "Error");
    compute_->setEnabled(true);
}
