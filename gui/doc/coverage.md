# Normaliz GUI - Coverage vs jNormaliz

Parity checklist against the jNormaliz 1.7 feature inventory. Goal: cover 100% of
what jNormaliz did, then go beyond.

Legend: [x] done, [~] partial, [ ] todo, [-] not applicable by design.

## File menu

| jNormaliz | Status | Notes |
|---|---|---|
| Open | [x] | QFileDialog, reads .in into the editor |
| New (rows/cols dialog) | [x] | size dialog builds a zero-filled cone template |
| Close | [x] | clears the input (with unsaved-changes prompt) |
| Save | [x] | writes the editor to file |
| Save as | [x] | getSaveFileName |
| Print | [x] | prints the input via QPrinter |
| Exit | [x] | prompts to save when modified |

## Edit menu

| jNormaliz | Status | Notes |
|---|---|---|
| Undo | [x] | Edit menu; targets the input editor |
| Cut | [x] | Edit menu; targets the input editor |
| Copy | [x] | Edit menu; targets the input editor |
| Paste | [x] | Edit menu; targets the input editor |

## Normaliz menu / Run

| jNormaliz | Status | Notes |
|---|---|---|
| Run | [x] | Compute button, in-process |
| Stop | [x] | cancels via nmz_interrupted (InterruptException) |

## Toolbar

| jNormaliz | Status | Notes |
|---|---|---|
| Algorithm box | [x] | Default / Primal / Dual (PrimalMode/DualMode) |
| Computational mode box | [x] | Goals only / DefaultMode |
| Precision box | [x] | Default / BigInt |

## Tabbed panel

| jNormaliz | Status | Notes |
|---|---|---|
| Input tab | [x] | editor pane |
| Output tab | [x] | output pane (text) |
| Console tab | [x] | libnormaliz verbose output (shown on completion) |
| Options tab | [x] | thread control + font size |

## Options (jNormaliz Options tab)

| jNormaliz | Status | Notes |
|---|---|---|
| Output file options (.out/.gen/.inv/.typ/.cst, triangulation, Stanley) | [-] | in-process; no .out files. Structured results instead |
| Ignore in-file options | [ ] | OptionsHandler flag |
| Control parallel threads | [x] | Options tab spinbox (set_thread_limit) |
| Font size | [x] | Options tab spinbox |
| NmzIntegrate (Ehrhart series, quasipolynomial leading coeff, Lebesgue integral) | [ ] | needs NmzIntegrate / CoCoALib |

## Status line

| jNormaliz | Status | Notes |
|---|---|---|
| Elapsed time | [x] | timer in the status bar |
| Physical memory gauge | [x] | process RSS in the status bar (Windows/Linux) |
| Running indicator | [x] | Computing/Stopping/Ready status + Stop button |

## Help menu

| jNormaliz | Status | Notes |
|---|---|---|
| Help | [x] | usage dialog |
| Open Normaliz manual | [x] | opens the online manual PDF |
| Open Normaliz website | [x] | opens the Normaliz GitHub |
| Mathematical background | [x] | dialog with a short description and links |
| About | [x] | about box |

## Computation goals

jNormaliz drives these through the mode box; we expose them as checkboxes.

| Goal | Status | Notes |
|---|---|---|
22 goals are offered through a goal table (add a ConeProperty + a formatGoal case).

| Hilbert basis, extreme rays, support hyperplanes | [x] | matrices |
| Module generators, degree-1 elements, maximal subspace | [x] | matrices |
| Hilbert series, Ehrhart series | [x] | series |
| Multiplicity, volume, lattice points, triangulation size | [x] | scalars |
| Class group, grading, dehomogenization | [x] | vectors |
| Rank, embedding dim, recession rank | [x] | scalars |
| Is pointed / Gorenstein / deg-1 extreme rays | [x] | boolean tests |
| Automorphism groups | [x] | nauty enabled; group order + generating permutations |
| Algebraic polyhedra (renf) | [~] | e-antic enabled in the engine and verified (dodecahedron over Q(sqrt5)); GUI algebraic-input path pending |
| Integrals / weighted Ehrhart | [ ] | still need CoCoALib (non-NAKED) |

## Beyond jNormaliz (what jNormaliz did not cover)

| Item | Status | Notes |
|---|---|---|
| In-process engine (no .in/.out round-trip) | [x] | Cone in process, no temp files |
| Cross-platform native installers | [x] | CI: setup.exe / AppImage / dmg |
| Backend selector Local / Cloud | [~] | Local done; Cloud (distributed) is WIP |
| Structured, typed result views (tables) | [ ] | QTableView for matrices |
| Visualization (2D/3D cones, lattice points, triangulation) | [ ] | research pillar |
| Observability (live progress, partial results) | [ ] | research pillar |
| Autotuning (automatic algorithm/mode/threads) | [ ] | research pillar |
| Provenance / reproducibility records | [ ] | research pillar |

## Summary

Parity: essentially complete. Done - the full File menu (New/Open/Close/Save/Save
As/Print/Exit), Edit menu, Run/Stop, the toolbar (algorithm/mode/precision), the
Help menu (help/website/manual/math background/About), the Output/Console/Options
tabs (thread control, font size), eight computation goals, and the status line
(elapsed time, memory gauge). Not applicable to this build: Ignore-in-file and
NmzIntegrate options (NmzIntegrate needs CoCoALib; output-file options are moot
for the in-process engine). Refinements left: live-streaming Console (the log is
shown on completion) and further computation goals. Beyond jNormaliz: in-process
engine, installers and the Local backend are done; visualization, observability,
autotuning and provenance are the research pillars.
