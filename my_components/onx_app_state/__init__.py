from pathlib import Path

import esphome
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.core import CORE

onx_app_state_ns = cg.esphome_ns.namespace("onx_app_state")
OnxAppState = onx_app_state_ns.class_("OnxAppState", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(OnxAppState),
}).extend(cv.COMPONENT_SCHEMA)

_API_BATCH_ORIGINAL = """\
    // After first message, set remaining size to MAX_BATCH_PACKET_SIZE to avoid fragmentation
    if (items_processed == 1) {
      remaining_size = MAX_BATCH_PACKET_SIZE;
    }
    remaining_size -= payload_size;
"""

_API_BATCH_PATCHED = """\
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


def _patch_esphome_api_batch_source(path: Path) -> None:
    try:
        source = path.read_text(encoding="utf-8")
    except OSError:
        return
    if _API_BATCH_PATCHED in source:
        print(f"ESPHome API large-message batch patch already applied: {path}")
        return
    if _API_BATCH_ORIGINAL not in source:
        print(f"ESPHome API large-message batch patch not applicable (source changed): {path}")
        return
    try:
        path.write_text(source.replace(_API_BATCH_ORIGINAL, _API_BATCH_PATCHED, 1), encoding="utf-8")
        print(f"Applied ESPHome API large-message batch patch: {path}")
    except OSError as err:
        print(f"Could not apply ESPHome API large-message batch patch to {path}: {err}")


def _patch_esphome_api_batch() -> None:
    package_dir = Path(esphome.__file__).resolve().parent
    _patch_esphome_api_batch_source(package_dir / "components/api/api_connection.cpp")
    _patch_esphome_api_batch_source(
        Path(CORE.build_path) / "src/esphome/components/api/api_connection.cpp"
    )


async def to_code(config):
    try:
        _patch_esphome_api_batch()
    except Exception as err:  # noqa: BLE001 - a patch failure must not break codegen
        print(f"WARNING: could not apply the ESPHome API large-message batch patch: {err}")
    patch_script = CORE.relative_config_path("scripts/patch_esphome_api_batch.py")
    cg.add_platformio_option("extra_scripts", [f"pre:{patch_script}"])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
