# `pd::metro`: sample-timed polling

[pd::metro](../../include/klang/pd.h) adapts the millisecond period and clock arithmetic from PD 0.55-2 `x_time.c` / `m_sched.c` to Chris's preferred polling interface. The sound model owns its counter and held gate.

```cpp
pd::metro metro;
metro = 1;
// Once per sample, including while stopped:
if (metro(100))
    gate = count++ % 2;
```

## Contract

- `operator=(param)` accepts zero to stop and any nonzero value to arm/restart. `start(param)`, `bang()` and `stop()` are explicit equivalents.
- Starting arms the first bang for the next evaluation. The first `metro(100)` emits immediately and schedules the next bang 100 ms later, including after default construction. Stopping before evaluation cancels the pending start; several starts before evaluation coalesce into one restart.
- `operator()(param milliseconds)` caches the period and advances one sample. `operator()()` uses the stored period. Poll while stopped too, so the clock follows sample time.
- Changing the period leaves an already scheduled deadline in place. The new period determines subsequent deadlines. Fractional deadlines accumulate without rounding each interval to an integer sample count.
- Nonpositive periods become 1 ms, as in current PD. `legacy = true` also clamps positive periods below 1 ms, matching the pre-0.45 rule. The clock uses PD's double-precision logical units.
- The return value is the **number of bangs in this sample**. Ordinary periods such as 100 ms can use `if`. For a period shorter than one sample, consume the count, for example `count += metro(period)`.

This is the requested sample-timed interpretation, not a complete PD message scheduler. PD invokes start bangs synchronously and delivers all control events in a 64-sample interval before rendering that block. Here model code observes a start at its next poll and later events in their individual sample intervals. Synchronous outlet feedback, ordering between independent clocks, named tempo units and PD block delivery remain separate work. No core Klang change was needed.

## Validation

```text
python tools/check_pd_metro.py
python tools/render_artificial.py --cases pedestrians pedestrians-controls
python tools/render_artificial.py --cases pedestrians pedestrians-controls --rate 44100
```

[Sixteen fixtures](metro-results.json), eight at each of 48/44.1 kHz, compare cumulative counts against the isolated [PD patch](metro.pd): default, fractional and fast periods; zero/negative periods; initially stopped state; interval change with a pending deadline; stop/restart/bang; numeric assignment including negative nonzero; and the legacy minimum-period rule. The first-poll `metro(100)` path is tested from a default-constructed object.

For comparison only, the sample-timed count at the end of each 64-sample block is projected across that block. Every decoded sample then matches PD exactly. This projection is analysis only: it is not applied to the metro or model. Raw counts remain under `build/metro-review/`.

Pedestrians' default and control fixtures are sample-identical to PD at 48 kHz. At 44.1 kHz, its gate edges occur 0–63 samples later than PD's block-advanced edges; the active carrier samples are identical. The runner checks edge count, direction and bounds, plus carrier identity, and retains raw residuals. Block delivery is deliberately deferred following Chris's choice of the pure model form. The control recipe now starts explicitly stopped, without an additional hidden start before its initial `set(0)`.

CPU/RSS observations include startup and file I/O, not just DSP. Tested build: MSVC 19.51.36257, CMake x64 Release, installed PD 0.55.2. The legacy interval fixture checks its clamp against `[metro 1]`, not an original historical executable.
