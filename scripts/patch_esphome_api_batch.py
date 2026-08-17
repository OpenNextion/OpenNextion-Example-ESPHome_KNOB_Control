Import("env")

from pathlib import Path


target = (
    Path(env.subst("$PROJECT_SRC_DIR"))
    / "esphome"
    / "components"
    / "api"
    / "api_connection.cpp"
)

original = """\
    // After first message, set remaining size to MAX_BATCH_PACKET_SIZE to avoid fragmentation
    if (items_processed == 1) {
      remaining_size = MAX_BATCH_PACKET_SIZE;
    }
    remaining_size -= payload_size;
"""

patched = """\
    // A single entity-info message (for example, a Select with many options) may be
    // larger than the preferred MTU-sized batch. Send that message by itself instead
    // of underflowing remaining_size and appending more large messages to the buffer.
    if (items_processed == 1) {
      remaining_size =
          payload_size < MAX_BATCH_PACKET_SIZE ? MAX_BATCH_PACKET_SIZE - payload_size : 0;
    } else {
      remaining_size -= payload_size;
    }
"""

source = target.read_text(encoding="utf-8")
if patched in source:
    print(f"ESPHome API large-message batch patch already applied: {target}")
elif original in source:
    target.write_text(source.replace(original, patched, 1), encoding="utf-8")
    print(f"Applied ESPHome API large-message batch patch: {target}")
else:
    raise RuntimeError(
        "Cannot apply ESPHome API large-message batch patch: expected source "
        f"fragment was not found in {target}. Review the ESPHome API implementation "
        "before building with this ESPHome version."
    )
