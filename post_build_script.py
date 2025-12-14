import shutil
from pathlib import Path

Import("env")

def post_build_hook(source, target, env):
    # Get the build directory and environment name
    build_dir = Path(env.subst("$BUILD_DIR"))
    env_name = env.subst("$PIOENV")

    # Define the destination path
    destination_dir = Path(env.subst("$PROJECT_DIR")) / "firmware"
    destination_dir.mkdir(exist_ok=True)

    # Firmware file extensions to copy (if they exist)
    extensions = [".elf", ".bin", ".hex", ".uf2"]

    for ext in extensions:
        firmware_path = build_dir / f"firmware{ext}"
        if firmware_path.exists():
            destination_file = destination_dir / f"firmware-{env_name}{ext}"
            print(f"Copying {firmware_path.name} to {destination_file}...")
            shutil.copy(firmware_path, destination_file)

    print("Done copying firmware files.")

# Hook into the program path (the final ELF/binary)
env.AddPostAction("$PROGPATH", post_build_hook)