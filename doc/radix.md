# Radix-Sort Pipeline

## Input
- `values[N_KEYS]` – unsorted values (e.g., instance IDs)
- `keys[N_KEYS]` – unsorted integer keys (`uint8`, `uint16`, or `uint32`)

## Buffers
- `tmp_values[N_KEYS]` – temporary swap buffer
- `histogram[N_BINS * N_WGS]` – per-bucket counts/offsets
  - Sequential: global histogram (`N_WGS = 1`)
  - Parallel: one histogram per work group (local)

## Process

### 1. Histogram
- Compute the bucket for each key
- Increment the corresponding counter
- Parallel: work groups maintain local histograms

### 2. Prefix Scan
- Compute offsets for each bucket
- CPU: serial prefix sum per histogram
- GPU: Blelloch scan or hierarchical scan
  - Step 1: local prefix (per work group)
  - Step 2: global prefix (block offsets)
  - Step 3: scatter phase

### 3. Scatter
- Use bucket offsets to reorder `values` into output buffer

## Notes
- `RADIX_BITS` – bits per pass (e.g., 4 → 16 buckets)
- `PASSES` – `KEY_BITS / RADIX_BITS`
- CPU: serial prefix sum is sufficient
- GPU: hierarchical scan avoids large local memory
