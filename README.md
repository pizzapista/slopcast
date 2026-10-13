# Slopcast — Dreamcast emulator for PS Vita

AI DISCLOSURE: This was made entirely on betterdeepseek with a bit of troubleshooting by me. Use at your own risk 

A from-scratch, minimalist Dreamcast emulator targeting the PlayStation Vita
(Cortex-A9 @ 444 MHz, 512 MB RAM, PowerVR SGX543MP4+). Written in C11 with a
hand-rolled Makefile build against [VitaSDK](https://vitasdk.org).

> **Status: scaffold / pre-alpha.** The core loop boots, the memory map is
> wired, the SH-4 interpreter executes a meaningful instruction subset, and the
> Vita platform layer (audio out, framebuffer, input, power management) works.
> The PowerVR2 rasterizer, AICA ARM7 DSP, GD-ROM disc image decoder, and the
> SH-4 dynamic recompiler are **structural stubs** with clearly marked
> `TODO(impl)` boundaries. This is not yet a playable emulator — it is the
> correct skeleton to grow one from.

---

## Design goals

| Goal              | How it drives the design                                                       |
| ----------------- | ------------------------------------------------------------------------------ |
| **Accuracy**      | Cycle-ish SH-4 interpreter is the reference; the dynarec must match it 1:1.    |
| **Battery/therm.**| Dynamic clock control, adaptive frame skip, no busy-wait. Render only what's dirty. |
| **Portability**   | Platform layer is a thin, swap-in interface. The core doesn't know about Vita.  |
| **Minimalism**    | No C++ RTTI, no exceptions, no heap churn in the hot path. Static pools only.  |

## Requirements

- [VitaSDK](https://vitasdk.org/#installation) with `VITASDK` set.
- `make`, a POSIX shell, and ~200 MB of free disk.
- A Dreamcast BIOS (`dc_boot.bin`, `dc_flash.bin`) — **you must supply these
  yourself.** They are copyrighted and are not distributed here.
- A `.gdi` or `.cdi` disc image of a game you own.

Place BIOS files at `ux0:data/dc-vita/bios/`:

```

ux0:data/dc-vita/bios/dc_boot.bin   (2 MB)
ux0:data/dc-vita/bios/dc_flash.bin  (128 KB)

```

Place disc images at `ux0:data/dc-vita/roms/` (`.gdi`, `.cdi`, `.chd`).

## Build

```sh a bit of troubleshooti
export VITASDK=/usr/local/vitasdk
export PATH="$VITASDK/bin:$PATH"
make -j$(nproc)
```

Produces `dc-vita.vpk`. Copy to the Vita and install with VitaShell.

## Running

The emulator scans `ux0:data/dc-vita/roms/` for the first supported image.
Controls:

| Input ↕▾ | Mapping ↕▾ |
|---|---|
| −D-Pad / Analog | Dreamcast D-Pad |
| −Cross / A | A button |
| −Circle / B | B button |
| −Square / X | X button |
| −Triangle / Y | Y button |
| −L / R | L / R triggers |
| −Start | Start |
| −Select | Mode select |
| −**L + R + Start** | Open emulator menu |
| −**L + R + Select** | Quit to LiveArea |
⚙

## Architecture

```
src/
├── vita/          Vita platform layer (swap this out for another host)
│   ├── vita_main.c           boot, BIOS load, main frame loop
│   ├── vita_platform.c       init/teardown aggregator
│   ├── vita_audio.c          SceAudioOut backend, 44.1 kHz stereo
│   ├── vita_video.c          framebuffer / dirty-rect present
│   ├── vita_input.c          SceCtrl -> Dreamcast Maple bus
│   └── vita_power.c          clock scaling + battery-aware frame skip
├── core/          Dreamcast system integration
│   ├── dc.c                  top-level dc_t; owns all subsystems
│   ├── memory.c              address decode, RAM/VRAM/regions, DMA
│   └── scheduler.c           cycle budget, VBlank/HBlank IRQ timing
├── sh4/           Hitachi SH-4 CPU
│   ├── sh4.c                 register file, reset, IRQ entry
│   ├── sh4_interpreter.c     reference interpreter (subset implemented)
│   └── sh4_dynarec.c         ARMv7 codegen  [STUB — see ROADMAP]
├── pvr2/          PowerVR2 GPU  [STUB]
├── aica/          AICA audio + ARM7  [STUB]
├── gdrom/         GD-ROM drive  [STUB]
├── maple/         controller bus
└── util/          logging + frame-time profiler
```

## Memory map (as implemented in `core/memory.c`)

| Range ↕▾ | Region ↕▾ |
|---|---|
| −`0x00000000` – `0x001FFFFF` | Boot ROM (2 MB) |
| −`0x005F6800` – `0x005F69FF` | System ASIC |
| −`0x005F6C00` – `0x005F6CFF` | System boot ROM |
| −`0x005F7000` – `0x005F7FFF` | System control registers |
| −`0x005F8000` – `0x005F9FFF` | Holly (PVR2 / GD-ROM / Maple) |
| −`0x00700000` – `0x007007FF` | AICA registers |
| −`0x00800000` – `0x009FFFFF` | AICA wave RAM (2 MB) |
| −`0x0C000000` – `0x0CFFFFFF` | Main RAM (16 MB) |
| −`0x10000000` – `0x11FFFFFF` | VRAM (8 MB, 4 access widths) |
| −`0xFF000000` – `0xFFFFFFFF` | SH-4 on-chip peripherals |
⚙

Mirroring of the Boot ROM at `0x20000000` and the SH-4 on-chip register block
are handled as catch-all ranges; exact mirror semantics need verification
against a hardware reference (see `ROADMAP.md`).

## Roadmap

See [`ROADMAP.md`](https://roadmap.md/). Highlights in order of impact:

1. **SH-4 dynarec** — without it, real-time is impossible on the Vita's CPU.
2. **PVR2 tile-deferred rasterizer** — the single biggest battery win.
3. **AICA ARM7 interpreter + DSP** — needed for any game to have sound.
4. **GD-ROM + GDI/CDI image parsing** — currently only stubbed to a null device.

## License

MIT. See `LICENSE`.

This project contains no copyrighted Sega material. The Dreamcast BIOS and any
commercial game images you supply remain the property of their respective
owners and are your responsibility to obtain legally.

</BDS:create_file>

<BDS:create_file fileName="vita-dreamcast/ROADMAP.md">

```markdown
# Roadmap

Ordered by impact-per-line-of-code. Each milestone ends with a *verifiable
observable* — the thing that must be true before the milestone is considered
done.

## M0 — Scaffold (this repo) ✅

- [x] VitaSDK + Makefile build produces a loadable `.vpk`.
- [x] Platform layer: framebuffer present, 44.1 kHz audio out, ctrl input,
      clock scaling, battery telemetry.
- [x] Memory map decoder with all Dreamcast regions named.
- [x] SH-4 interpreter executes a documented subset and traps the rest.
- [x] Scheduler emits VBlank at NTSC/PAL rates.
- [x] Subsystems (PVR2, AICA, GD-ROM, Maple) have stable interfaces.

**Observable:** the VPK launches, shows a dark "no disc / no BIOS" screen at
60 Hz, plays silence, and exits cleanly on L+R+Select.

---

## M1 — SH-4 interpreter correctness

Implement the remaining instruction classes and validate each against a
reference table. Unimplemented opcodes must trap with a diagnostic that
records PC, opcode, and the fetched bytes.

- [ ] Full arithmetic: `DMULS.L`, `DMULU.L`, `MULS.W`, `MULU.W`, `MAC.W/L`.
- [ ] Full shifts: `SHAD`, `SHLD`, `ROTL`, `ROTR` with carry.
- [ ] All `MOV` variants including `@(R0,Rn)`, `@(disp,GBR)`, PC-relative.
- [ ] `TAS.B`, `AND.B`, `OR.B`, `XOR.B` (byte ops with side effects).
- [ ] `LDC`, `LDC.L`, `STC`, `STC.L` for `SR/GBR/VBR/SSR/SPC/SGR/DBR`.
- [ ] Exceptions: `TRAPA` → VBR handler; `RTE` restores SR/SSR/SPC.
- [ ] Interrupt entry at VBR+0x600 with correct SR masking.
- [ ] `DIV0U`, `DIV0S`, `DIV1` bit-exact against reference.
- [ ] Full FPU: `FADD`–`FDIV`, `FMAC`, `FSQRT`, `FCMP`, `FLDI`, `FSTS`,
      `FMOV` matrix variants, `FTRC`, `FLOAT`, `FIPR`, `FSRRA`.

**Observable:** a public SH-4 test ROM (e.g. `sh4_test`) passes all integer
tests and FPU tests, and `dc_boot.bin` runs past its second-stage loader.

---

## M2 — SH-4 dynamic recompiler

The interpreter is the ground truth; the dynarec must be indistinguishable
from it under a differential test harness.

- [ ] Basic-block discovery + hash-based block cache.
- [ ] ARMv7 emitter for: ALU, loads/stores, direct branches.
- [ ] Register allocation: pin `R0`–`R7`, `SP`, `PR` in ARM regs; leave the
      rest in the SH-4 struct.
- [ ] FLAGS → ARM `CPSR` mapping for `T` bit; SR `Q`, `M`, `S` bits shadowed.
- [ ] Self-modifying-code detection via page-level write-protect.
- [ ] Block chaining (direct jump without returning to dispatch).
- [ ] Underclock-safe re-entry (see `vita_power.c`).
- [ ] **Differential fuzzer:** run the same random program through interp
      and dynarec, compare registers + memory every 10 000 cycles.

**Observable:** the dynarec runs a 100 M-instruction randomized workload with
zero divergence from the interpreter, and a real game boots 3× faster.

---

## M3 — PowerVR2 rasterizer

Flycast does this in software first, then with a tile-deferred architecture.
We go straight to tile-deferred because the Vita's GPU is the wrong shape for
an immediate-mode port.

- [ ] Parse the PVR2 command FIFO (`0x005F8000`), dispatch by `(cmd >> 24)`.
- [ ] Register file: `FB_R_CTRL`, `FB_W_CTRL`, `FB_W_LINESTRIDE`, `SCL`, … 
- [ ] Tile binning: 32×32 tiles, per-tile list of (primitive, z).
- [ ] Sort by `(tile, z)` — this is where battery efficiency comes from.
- [ ] Rasterize opaque pass with NEON (4 pixels/cycle, 16-bit packed).
- [ ] Rasterize punch-through (alpha test) pass.
- [ ] Rasterize translucent (alpha blend) pass, back-to-front.
- [ ] Texture decode: `TW`/`VQ`/`TW16`/`VQ16`, twiddled, mipmaps.
- [ ] **Texture cache:** hash on (VRAM page digest, format); LRU 64 entries.
- [ ] Modifier volumes + shadow volumes + bump maps (later).
- [ ] Present: dirty-rect blit from VRAM to the Vita framebuffer.

**Observable:** the Sega logo and BIOS menu render correctly; a known game
shows a stable 60 Hz picture with textures.

---

## M4 — AICA

- [ ] ARM7TDMI interpreter (32-bit ARM + Thumb decode).
- [ ] AICA register file at `0x00700000`.
- [ ] 64-channel sample playback, 44.1 kHz.
- [ ] ADPCM decoder.
- [ ] DSP (the "AICA DSP" 128-step program) — reverse-engineered opcode set.
- [ ] ARM7 IRQ from the SH-4 side.
- [ ] Mix into the Vita audio callback.

**Observable:** the BIOS startup chime plays cleanly; a game's music
loops without dropouts for 10 minutes.

---

## M5 — GD-ROM + disc images

- [ ] GD-ROM register decode at `0x00600000`.
- [ ] SCSI command subset (`REQ_SENSE`, `READ_TOC`, `READ_CD`, …).
- [ ] GDI parser (multi-track GD-ROM images).
- [ ] CDI parser (single-file).
- [ ] CHD parser (compressed, via libchdr or a minimal reimplementation).
- [ ] Sector cache with a 4 MB LRU (fits in Vita RAM).
- [ ] High-density (GD) vs low-density (CD) track routing.

**Observable:** any retail GDI boots past the licensing screen.

---

## M6 — Maple bus

- [ ] Bus protocol framing (DMA at `0x005F6C00`).
- [ ] Port enumeration.
- [ ] Controller: digital + analog + VMU + rumble.
- [ ] VMU: 128 KB EEPROM image, file system (FAT-like), LCD framebuffer.
- [ ] Keyboard, mouse, light gun (later).

**Observable:** the BIOS controller test screen reads input correctly and
VMU contents persist across power cycles.

---

## M7 — Polish and battery work

- [ ] Save states (serialize `dc_t` with a version tag).
- [ ] On-screen menu overlay (built from the same framebuffer we present).
- [ ] Adaptive frame skip driven by measured frame time (see `vita_power.c`).
- [ ] "Eco mode": clamp SH-4 to 111 MHz, skip every other frame, disable
      texture filtering. Target: 2× battery life on the Slim.
- [ ] Frame-time histogram export to `ux0:data/dc-vita/perf.csv` for offline
      tuning.

**Observable:** a 30-minute session reports < 5% jitter in frame time and the
Vita's battery indicator drops less than half as fast as in "Performance".

---

## Non-goals (explicitly)

- **Not** aiming for Naomilib / arcade hardware emulation.
- **Not** implementing the modem, BBA, or Ethernet.
- **Not** supporting the Karaoke add-on.
- **Not** running without a BIOS — the BIOS is a prerequisite, not a
  licensing workaround.





