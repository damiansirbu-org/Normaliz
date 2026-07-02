# Normaliz GUI - Coverage vs jNormaliz

Parity checklist against the jNormaliz 1.7 feature inventory. Goal: cover 100% of
what jNormaliz did, then go beyond.

Legend: [x] done, [~] partial, [ ] todo, [-] not applicable by design.

## File menu

| jNormaliz | Status | Notes |
|---|---|---|
| Open | [x] | QFileDialog, reads .in into the editor |
| New (rows/cols dialog) | [x] | size dialog builds a zero-filled cone template |
| Close | [ ] | close the current input |
| Save | [x] | writes the editor to file |
| Save as | [x] | getSaveFileName |
| Print | [ ] | print the current view |
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
| Physical memory gauge | [ ] | process memory |
| Running indicator | [x] | Computing/Stopping/Ready status + Stop button |

## Help menu

| jNormaliz | Status | Notes |
|---|---|---|
| Help | [ ] | this documentation |
| Open Normaliz manual | [x] | opens the online manual PDF |
| Open Normaliz website | [x] | opens the Normaliz GitHub |
| Mathematical background | [x] | dialog with a short description and links |
| About | [x] | about box |

## Computation goals

jNormaliz drives these through the mode box; we expose them as checkboxes.

| Goal | Status | Notes |
|---|---|---|
| Hilbert basis | [x] | getHilbertBasis |
| Extreme rays | [x] | getExtremeRays |
| Support hyperplanes | [x] | getSupportHyperplanes |
| Hilbert series | [x] | getHilbertSeries |
| Multiplicity | [x] | getMultiplicity |
| Volume | [x] | getVolume |
| Lattice points | [x] | getNumberLatticePoints |
| Class group | [x] | getClassGroup |
| Rank, embedding dim, and more | [ ] | further ConeProperty targets |

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

Parity done: File (Open/New/Save/Save As/Exit), Edit menu, Run and Stop, the
toolbar (algorithm/mode/precision), the Help menu (website/manual/math
background/About), the Output/Console/Options tabs (thread control, font size),
eight computation goals, elapsed-time status. Remaining for 100% parity:
Close/Print, the memory gauge, Help "this documentation", Ignore-in-file and
NmzIntegrate options, and more computation goals. Beyond jNormaliz: in-process
engine, installers and the Local backend are done; visualization, observability,
autotuning and provenance are the research pillars.
