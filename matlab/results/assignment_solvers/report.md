# Assignment Solver Performance Report

Comparative analysis of the **Exhaustive Search** and **Branch-and-Bound (BB)** assignment solvers across four configuration scenarios. Missing metrics indicate the configuration exceeded real-time feasibility.

## Scenarios

| | Agents | Units per agent |
|---------------|--------|---------------|
| Benchmark-1 | 2 | 3 |
| Benchmark-2 | 3 | 2 |
| Benchmark-3 | 2 | 4 |
| Benchmark-4 | 4 | 2 |

## Results

### Benchmark-1 — 2 agents × 3 windows

| Solver | Mean duration | Max duration | Mean nodes | Max nodes |
|--------|--------------|--------------|------------|-----------|
| Exhaustive | 2651.69 ms | 3174.08 ms | 840 | 840 |
| Branch-and-Bound | 1733.03 ms | 2695.46 ms | 544 | 696 |

### Benchmark-2 — 3 agents × 2 windows

| Solver | Mean duration | Max duration | Mean nodes | Max nodes |
|--------|--------------|--------------|------------|-----------|
| Exhaustive | 2497.52 ms | 2870.67 ms | 1110 | 1110 |
| Branch-and-Bound | 724.37 ms | 1321.16 ms | 296 | 494 |

### Benchmark-3 — 2 agents × 4 windows

Both solvers exceeded real-time constraints. ❌

### Benchmark-4 — 4 agents × 2 windows

| Solver | Mean duration | Max duration | Mean nodes | Max nodes |
|--------|--------------|--------------|------------|-----------|
| Exhaustive | ❌ timeout | — | — | — |
| Branch-and-Bound | 26461.91 ms | 33797.57 ms | 9788 | 13456 |

## Summary

- **BB consistently outperforms Exhaustive** in both solve time and explored nodes across all feasible benchmarks.
- **Benchmarks 1 & 2** are the only configurations where both solvers complete, with BB being 1.5–3.5× faster and exploring 35–73% fewer nodes.
- **Benchmark-3** (4 windows) hits an exponential complexity wall for both solvers.
- **Benchmark-4** (4 agents) is only reachable by BB, but at ~26 s mean solve time it is not suitable for real-time use either.
- Neither solver meets real-time requirements beyond the small-scale (≤ 3 agents, ≤ 3 windows) configurations.