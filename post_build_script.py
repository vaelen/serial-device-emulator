import shutil
from pathlib import Path

Import("env")

def copy_firmware_files(source, target, env):
    """Copy all firmware files to the firmware directory."""
    build_dir = Path(env.subst("$BUILD_DIR"))
    env_name = env.subst("$PIOENV")

    destination_dir = Path(env.subst("$PROJECT_DIR")) / "firmware"
    destination_dir.mkdir(exist_ok=True)

    # Firmware file extensions to copy (if they exist)
    extensions = [".elf", ".bin", ".hex", ".uf2"]

    copied = False
    for ext in extensions:
        firmware_path = build_dir / f"firmware{ext}"
        if firmware_path.exists():
            destination_file = destination_dir / f"firmware-{env_name}{ext}"
            print(f"Copying {firmware_path.name} to {destination_file}...")
            shutil.copy(firmware_path, destination_file)
            copied = True

    if copied:
        print("Done copying firmware files.")

# Hook into multiple targets to catch all generated files
# The .bin is generated after .elf, and .uf2 after .bin (on Pico)
env.AddPostAction("$BUILD_DIR/firmware.bin", copy_firmware_files)
env.AddPostAction("$BUILD_DIR/firmware.hex", copy_firmware_files)
env.AddPostAction("$BUILD_DIR/firmware.uf2", copy_firmware_files)