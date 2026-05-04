const { useEffect, useMemo, useState } = React;

function verdictClass(speedup) {
  if (speedup > 1.05) return "good";
  if (speedup < 0.95) return "bad";
  return "warn";
}

function verdictText(speedup) {
  if (speedup > 1.05) return "NEON faster";
  if (speedup < 0.95) return "Scalar faster";
  return "About equal";
}

function formatInt(value) {
  return new Intl.NumberFormat("en-US").format(value);
}

function formatFloat(value, digits = 2) {
  return Number(value).toFixed(digits);
}

function formatNs(value) {
  if (value >= 1_000_000) return `${formatFloat(value / 1_000_000, 2)} ms`;
  if (value >= 1_000) return `${formatFloat(value / 1_000, 2)} µs`;
  return `${formatInt(Math.round(value))} ns`;
}

function formatUsFromNs(value) {
  return formatFloat(Number(value) / 1_000, 3);
}

function formatUsLabel(valueUs) {
  if (valueUs >= 1000) return `${formatFloat(valueUs, 0)} µs`;
  if (valueUs >= 100) return `${formatFloat(valueUs, 1)} µs`;
  if (valueUs >= 1) return `${formatFloat(valueUs, 2)} µs`;
  return `${formatFloat(valueUs, 3)} µs`;
}

function toBarPercent(speedup) {
  const clamped = Math.max(0.6, Math.min(1.5, speedup));
  return ((clamped - 0.6) / (1.5 - 0.6)) * 100;
}

function App() {
  const [data, setData] = useState(null);
  const [error, setError] = useState("");
  const [notice, setNotice] = useState("");
  const [running, setRunning] = useState(false);
  const [hoveredIndex, setHoveredIndex] = useState(null);

  const loadReport = async (silence = false) => {
    const response = await fetch(`./benchmark_results.json?t=${Date.now()}`);
    if (!response.ok) {
      throw new Error("No benchmark_results.json found. Generate report first.");
    }
    const json = await response.json();
    setData(json);
    setError("");
    if (!silence) setNotice("Report loaded.");
  };

  useEffect(() => {
    loadReport(true).catch((e) => setError(e.message));
  }, []);

  const runBenchmark = async () => {
    setRunning(true);
    setError("");
    setNotice("Running ./go ...");
    try {
      const response = await fetch("/api/run", { method: "POST" });
      const payload = await response.json();
      if (!response.ok || !payload.ok) {
        throw new Error(payload.error || "Failed to run benchmark.");
      }
      await loadReport(true);
      setNotice("Benchmark completed and table refreshed.");
    } catch (e) {
      setError(e.message || "Failed to run benchmark.");
      setNotice("");
    } finally {
      setRunning(false);
    }
  };

  const onUpload = async (event) => {
    const file = event.target.files?.[0];
    if (!file) return;
    try {
      const text = await file.text();
      const json = JSON.parse(text);
      setData(json);
      setError("");
      setNotice("Loaded report from local file.");
    } catch (e) {
      setError("Invalid JSON file. Please upload report from ./go --json ...");
      setNotice("");
    }
  };

  const rows = data?.rows ?? [];

  const chart = useMemo(() => {
    const safeRows = rows
      .map((row) => ({
        ...row,
        size: Number(row.size),
        basic_time_ns: Number(row.basic_time_ns),
        neon_time_ns: Number(row.neon_time_ns),
        speedup: Number(row.speedup)
      }))
      .filter(
        (row) =>
          Number.isFinite(row.size) &&
          Number.isFinite(row.basic_time_ns) &&
          Number.isFinite(row.neon_time_ns) &&
          row.size > 0
      )
      .slice()
      .sort((a, b) => a.size - b.size);

    if (!safeRows.length) return null;

    const width = 960;
    const height = 360;
    const margin = { top: 34, right: 26, bottom: 58, left: 86 };
    const plotW = width - margin.left - margin.right;
    const plotH = height - margin.top - margin.bottom;

    const sizes = safeRows.map((row) => row.size);
    const allTimesUs = safeRows.flatMap((row) => [row.basic_time_ns / 1000, row.neon_time_ns / 1000]);

    const minSize = Math.min(...sizes);
    const maxSize = Math.max(...sizes);
    const minLog = Math.log10(minSize);
    const maxLog = Math.log10(maxSize);
    const minTimeUs = Math.max(0.001, Math.min(...allTimesUs));
    const maxTimeUs = Math.max(...allTimesUs);
    const yBottom = minTimeUs * 0.8;
    const yTop = maxTimeUs * 1.12;
    const minYLog = Math.log10(yBottom);
    const maxYLog = Math.log10(yTop);

    const scaleX = (size) => {
      if (minSize === maxSize) return margin.left + plotW / 2;
      const t = (Math.log10(size) - minLog) / (maxLog - minLog);
      return margin.left + t * plotW;
    };

    const scaleY = (timeNs) => {
      const valueUs = Math.max(0.000001, timeNs / 1000);
      const t = (Math.log10(valueUs) - minYLog) / Math.max(maxYLog - minYLog, 1e-9);
      return margin.top + (1 - t) * plotH;
    };

    const toPath = (key) =>
      safeRows
        .map((row, idx) => `${idx === 0 ? "M" : "L"} ${scaleX(row.size)} ${scaleY(row[key])}`)
        .join(" ");

    const yTicks = Array.from({ length: 6 }, (_, i) => {
      const exponent = minYLog + ((maxYLog - minYLog) * i) / 5;
      const value = 10 ** exponent;
      return {
        value,
        y: scaleY(value * 1000)
      };
    });

    return {
      rows: safeRows,
      width,
      height,
      margin,
      plotW,
      scaleX,
      scaleY,
      basicPath: toPath("basic_time_ns"),
      neonPath: toPath("neon_time_ns"),
      yTicks
    };
  }, [rows]);

  const summary = useMemo(() => {
    if (!rows.length) return null;
    const average = rows.reduce((acc, row) => acc + row.speedup, 0) / rows.length;
    const best = rows.reduce((acc, row) => (row.speedup > acc.speedup ? row : acc), rows[0]);
    return {
      averageSpeedup: average,
      bestSpeedup: best.speedup,
      bestSize: best.size
    };
  }, [rows]);

  return (
    <main className="wrap">
      <section className="hero">
        <h1>Array Processing Benchmark Dashboard</h1>
        <p>Visual report for scalar vs NEON performance.</p>

        <div className="toolbar">
          <button className="run-btn" onClick={runBenchmark} disabled={running}>
            {running ? "Running..." : "Run benchmark"}
          </button>
          <label className="file-label">
            Upload JSON report
            <input type="file" accept="application/json" onChange={onUpload} />
          </label>
          <span className="pill">Metric: average time per call (µs)</span>
          <span className="pill">NEON: {data ? (data.neon_enabled ? "enabled" : "fallback/scalar") : "unknown"}</span>
        </div>
      </section>

      {notice && <div className="notice">{notice}</div>}
      {error && <div className="error">{error}</div>}

      {summary && (
        <>
          <section className="meta-grid">
            <article className="meta-card">
              <div className="label">Average Speedup</div>
              <div className="value">{formatFloat(summary.averageSpeedup)}x</div>
            </article>
            <article className="meta-card">
              <div className="label">Best Speedup</div>
              <div className="value">{formatFloat(summary.bestSpeedup)}x</div>
            </article>
            <article className="meta-card">
              <div className="label">Best Array Size</div>
              <div className="value">{formatInt(summary.bestSize)}</div>
            </article>
            <article className="meta-card">
              <div className="label">Rows</div>
              <div className="value">{rows.length}</div>
            </article>
          </section>

          {chart && (
            <section className="chart-card">
              <div className="chart-head">
                <div>
                  <h2>Time Curve: NEON + BASIC</h2>
                  <p>Log-scale X and Y, Y axis in µs.</p>
                </div>
                <div className="legend">
                  <span className="legend-item neon">NEON</span>
                  <span className="legend-item basic">BASIC</span>
                </div>
              </div>

              <div className="chart-wrap">
                <svg viewBox={`0 0 ${chart.width} ${chart.height}`} role="img" aria-label="NEON and BASIC timing chart">
                  <defs>
                    <linearGradient id="chartBg" x1="0" x2="0" y1="0" y2="1">
                      <stop offset="0%" stopColor="#0d2533" />
                      <stop offset="100%" stopColor="#0b1723" />
                    </linearGradient>
                    <linearGradient id="basicLine" x1="0" x2="1" y1="0" y2="0">
                      <stop offset="0%" stopColor="#ffc44d" />
                      <stop offset="100%" stopColor="#ff8f3f" />
                    </linearGradient>
                    <linearGradient id="neonLine" x1="0" x2="1" y1="0" y2="0">
                      <stop offset="0%" stopColor="#57f6ff" />
                      <stop offset="100%" stopColor="#2db6ff" />
                    </linearGradient>
                    <filter id="neonGlow" x="-40%" y="-40%" width="180%" height="180%">
                      <feGaussianBlur stdDeviation="5" result="coloredBlur" />
                      <feMerge>
                        <feMergeNode in="coloredBlur" />
                        <feMergeNode in="SourceGraphic" />
                      </feMerge>
                    </filter>
                  </defs>

                  <rect x="0" y="0" width={chart.width} height={chart.height} fill="url(#chartBg)" rx="18" />

                  {chart.yTicks.map((tick) => (
                    <g key={tick.y}>
                      <line
                        x1={chart.margin.left}
                        y1={tick.y}
                        x2={chart.margin.left + chart.plotW}
                        y2={tick.y}
                        className="grid-line"
                      />
                      <text x={chart.margin.left - 14} y={tick.y + 4} className="axis-label y-label">
                        {formatUsLabel(tick.value)}
                      </text>
                    </g>
                  ))}

                  <line
                    x1={chart.margin.left}
                    y1={chart.margin.top + (chart.height - chart.margin.top - chart.margin.bottom)}
                    x2={chart.margin.left + chart.plotW}
                    y2={chart.margin.top + (chart.height - chart.margin.top - chart.margin.bottom)}
                    className="axis-line"
                  />
                  <line
                    x1={chart.margin.left}
                    y1={chart.margin.top}
                    x2={chart.margin.left}
                    y2={chart.margin.top + (chart.height - chart.margin.top - chart.margin.bottom)}
                    className="axis-line"
                  />

                  <path d={chart.neonPath} className="line-neon-glow" />
                  <path d={chart.neonPath} className="line-neon" />
                  <path d={chart.basicPath} className="line-basic" />

                  {chart.rows.map((row, idx) => {
                    const x = chart.scaleX(row.size);
                    const neonY = chart.scaleY(row.neon_time_ns);
                    const basicY = chart.scaleY(row.basic_time_ns);
                    const isActive = hoveredIndex === idx;
                    return (
                      <g key={`point-${row.size}-${idx}`}>
                        <text x={x} y={chart.height - 24} className="axis-label x-label">
                          {formatInt(row.size)}
                        </text>
                        <circle cx={x} cy={basicY} r={isActive ? 5 : 4} className="dot-basic" />
                        <circle cx={x} cy={neonY} r={isActive ? 5 : 4} className="dot-neon" />
                        <circle
                          cx={x}
                          cy={(basicY + neonY) / 2}
                          r={16}
                          className="hover-area"
                          onMouseEnter={() => setHoveredIndex(idx)}
                          onMouseLeave={() => setHoveredIndex(null)}
                        />
                      </g>
                    );
                  })}
                </svg>
              </div>

              <div className="chart-foot">
                {hoveredIndex == null ? (
                  <span>Hover a point to inspect values.</span>
                ) : (
                  <span>
                    Size {formatInt(chart.rows[hoveredIndex].size)}:
                    {" "}
                    BASIC {formatUsFromNs(chart.rows[hoveredIndex].basic_time_ns)} µs
                    {" • "}
                    NEON {formatUsFromNs(chart.rows[hoveredIndex].neon_time_ns)} µs
                    {" • "}
                    Speedup {formatFloat(chart.rows[hoveredIndex].speedup)}x
                  </span>
                )}
              </div>
            </section>
          )}

          <section className="table-card">
            <div className="scroll">
              <table>
                <thead>
                  <tr>
                    <th>Array Size</th>
                    <th>Basic (µs)</th>
                    <th>NEON (µs)</th>
                    <th>Speedup</th>
                    <th>Repeats</th>
                    <th>Verdict</th>
                    <th>Visual</th>
                  </tr>
                </thead>
                <tbody>
                  {rows.map((row, index) => {
                    const cls = verdictClass(row.speedup);
                    return (
                      <tr key={`${row.size}-${index}`}>
                        <td className="num">{formatInt(row.size)}</td>
                        <td className="num">{formatUsFromNs(row.basic_time_ns)}</td>
                        <td className="num">{formatUsFromNs(row.neon_time_ns)}</td>
                        <td className={`num speedup ${cls}`}>{formatFloat(row.speedup)}x</td>
                        <td className="num">{formatInt(row.repetitions)}</td>
                        <td className={`verdict ${cls}`}>{verdictText(row.speedup)}</td>
                        <td>
                          <div className="bar">
                            <div className="fill" style={{ width: `${toBarPercent(row.speedup)}%` }} />
                          </div>
                        </td>
                      </tr>
                    );
                  })}
                </tbody>
              </table>
            </div>
          </section>
        </>
      )}
    </main>
  );
}

ReactDOM.createRoot(document.getElementById("root")).render(<App />);
