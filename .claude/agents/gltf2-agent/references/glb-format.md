# glTF 2.0 — GLB Binary Container Format

## Overview

**GLB** (Binary glTF) packs the JSON scene description, binary buffer, and optionally
images into a single self-contained binary file. No base64 encoding overhead.

A GLB can still reference external resources (e.g., external images) alongside its
embedded buffer.

* Extension: `.glb`
* Media type: `model/gltf-binary`
* Byte order: **little-endian throughout**

---

## High-Level Structure

```
[ 12-byte header ]
[ Chunk 0: JSON  ]   — REQUIRED, MUST be first
[ Chunk 1: BIN   ]   — optional, 0 or 1 occurrences
```

Chunks MUST appear in the order given. Clients MUST ignore chunks with unknown type
(to support future extensions).

---

## Header (12 bytes)

```
Offset  Size  Field    Description
0       4     magic    MUST equal 0x46546C67 ("glTF" in ASCII, little-endian)
4       4     version  GLB container version. This spec = 2
8       4     length   Total file length in bytes (header + all chunks)
```

⚠️ The GLB header `version` refers to the  **container format version** , not the glTF asset
version. Clients MUST also read `asset.version` from the JSON chunk.

---

## Chunk Structure

Each chunk:

```
Offset  Size       Field        Description
0       4          chunkLength  Length of chunkData in bytes
4       4          chunkType    Type identifier
8       chunkLength chunkData   Raw payload
```

Chunks MUST be 4-byte aligned:

* Start of each chunk: aligned to 4-byte boundary.
* End of each chunk: padded to 4-byte boundary.

---

## Chunk Types

| chunkType  | ASCII     | Description             | Count     |
| ---------- | --------- | ----------------------- | --------- |
| 0x4E4F534A | `JSON`  | Structured JSON content | Exactly 1 |
| 0x004E4942 | `BIN\0` | Binary buffer           | 0 or 1    |

---

## Chunk 0: JSON (0x4E4F534A)

* MUST be the **first** chunk.
* Contains the glTF JSON exactly as it would appear in a `.gltf` file.
* Padded with trailing **Space characters (0x20)** to reach 4-byte alignment.

Reading in JavaScript:

```javascript
const jsonText = new TextDecoder().decode(
  new Uint8Array(arrayBuffer, 12 + 8, jsonChunkLength)
);
const gltf = JSON.parse(jsonText);
```

---

## Chunk 1: BIN (0x004E4942)

* MUST be the **second** chunk (if present).
* Contains the binary buffer for `buffers[0]`.
* Padded with trailing **null bytes (0x00)** to reach 4-byte alignment.
* The BIN chunk length MAY be up to 3 bytes larger than `buffer.byteLength` due to padding.
  (Strict equality not required — simplifies GLB conversion.)
* If the binary buffer is empty or stored externally, this chunk SHOULD be omitted.

---

## GLB-Stored Buffer in JSON

The first buffer in the `buffers` array, with  **no `uri`** , refers to the BIN chunk:

```json
{
  "buffers": [
    { "byteLength": 35884 },          // no "uri" → BIN chunk
    { "byteLength": 504, "uri": "external.bin" }  // additional external buffer
  ]
}
```

Any buffer without a `uri` that is NOT the first buffer has **undefined behavior** (reserved
for future extensions).

---

## Parsing Algorithm

```
1. Read 12-byte header:
   - Verify magic == 0x46546C67
   - Read version (must be 2)
   - Read total length

2. Offset = 12
   While offset < total_length:
     Read uint32 chunkLength at offset+0
     Read uint32 chunkType  at offset+4
     chunkData = bytes[offset+8 .. offset+8+chunkLength]
   
     If chunkType == 0x4E4F534A (JSON):
       Parse JSON from chunkData
     Else if chunkType == 0x004E4942 (BIN):
       Store as buffers[0] binary data
     Else:
       Ignore (future extension)
   
     offset += 8 + chunkLength  // chunkLength already padded to 4-byte boundary

3. Process glTF JSON normally, resolving bufferViews/accessors
```

---

## Writing a GLB

```
1. Serialize glTF JSON to UTF-8.
   - Pad with spaces to next 4-byte boundary.
   - jsonChunkLength = padded length.

2. Pad binary buffer to next 4-byte boundary with 0x00 bytes.
   - binChunkLength = padded length.

3. totalLength = 12 + 8 + jsonChunkLength + (8 + binChunkLength if binary present)

4. Write header:
   - magic:   0x46546C67
   - version: 2
   - length:  totalLength

5. Write JSON chunk:
   - chunkLength: jsonChunkLength
   - chunkType:   0x4E4F534A
   - chunkData:   padded JSON bytes

6. Write BIN chunk (if binary data exists):
   - chunkLength: binChunkLength
   - chunkType:   0x004E4942
   - chunkData:   padded binary bytes
```

---

## Comparison: .gltf vs .glb

| Aspect              | .gltf                       | .glb                   |
| ------------------- | --------------------------- | ---------------------- |
| Human readable      | Yes (JSON)                  | No                     |
| External assets     | Yes (`.bin`, images)      | Optional               |
| Single file         | No (unless using data URIs) | Yes                    |
| Encoding overhead   | base64 if embedded (+33%)   | None                   |
| Progressive loading | Possible (separate files)   | Limited                |
| GPU upload          | After base64 decode         | Direct from BIN chunk  |
| Use case            | Development, debugging      | Production, deployment |

---

## Common GLB Pitfalls

1. **Forgetting to pad** : JSON chunk must be space-padded; BIN chunk must be null-padded.
2. **Wrong magic** : 0x46546C67 is ASCII `glTF` in little-endian — bytes: `67 6C 54 46`.
3. **Relying on GLB header version** : Always read `asset.version` from JSON for glTF version.
4. **buffers[0].uri still present** : Must be absent for GLB-stored buffer.
5. **Missing BIN chunk declaration check** : If JSON has `buffers[0]` with no `uri`, a BIN
   chunk MUST be present.
