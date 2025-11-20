import os
import shutil
import subprocess
import platform
import re
import base64

BUILD_DIR = "build"
SRC_DIR = "src"
CPP_SRC_DIR = os.path.join(SRC_DIR, "cpp")
PYTHON_SRC_DIR = os.path.join(SRC_DIR, "python")
FINAL_SERVER_FILE = "server.py"

PYTHON_FILES_ORDER = [
    "ban_manager.py",
    "config.py",
    "player_manager.py",
    "packets.py",
    "world_manager.py",
    "command_handler.py",
    "admin_manager.py",
    "network.py",
    "heartbeat.py",
    "main.py",
]

def build_cpp_core():
    print("\n[--- Building C++ Core ---]")
    cmake_build_dir = os.path.join(BUILD_DIR, "cmake_temp")
    os.makedirs(cmake_build_dir)
    
    try:
        print("Running CMake...")
        subprocess.run(["cmake", "-S", ".", "-B", cmake_build_dir], check=True)
        
        print("Compiling C++ core...")
        subprocess.run(["cmake", "--build", cmake_build_dir, "--config", "Release"], check=True)
        
        print("Searching for compiled library...")
        lib_name = "core.dll" if platform.system() == "Windows" else "core.so"
        
        possible_paths = [
            os.path.join(cmake_build_dir, "lib", "Release", lib_name),
            os.path.join(cmake_build_dir, "bin", "Release", lib_name),
            os.path.join(cmake_build_dir, "Release", lib_name),
            os.path.join(cmake_build_dir, "lib", lib_name),
            os.path.join(cmake_build_dir, lib_name)
        ]
        
        found_path = next((path for path in possible_paths if os.path.exists(path)), None)
            
        if found_path:
            print(f"✅ Found C++ core at: {found_path}")
            shutil.copy(found_path, BUILD_DIR)
            print(f"Copied '{lib_name}' to '{BUILD_DIR}'")
        else:
            raise FileNotFoundError(f"Could not find compiled library '{lib_name}' in any standard paths.")

    finally:
        shutil.rmtree(cmake_build_dir)

def assemble_and_obfuscate_python_server():
    print("\n[--- Assembling and Obfuscating Python Server ---]")
    
    all_code_parts = []
    external_imports = set()
    internal_modules = [os.path.splitext(f)[0] for f in PYTHON_FILES_ORDER]
    
    for fname in PYTHON_FILES_ORDER:
        fpath = os.path.join(PYTHON_SRC_DIR, fname)
        with open(fpath, "r", encoding="utf-8") as infile:
            lines = infile.readlines()
            
            file_code = []
            for line in lines:
                is_internal_import = False
                for module_name in internal_modules:
                    if re.match(rf"from {module_name} import|import {module_name}", line.strip()):
                        is_internal_import = True
                        break
                
                if is_internal_import:
                    continue
                
                if line.strip().startswith(("import ", "from ")):
                    external_imports.add(line.strip())
                else:
                    file_code.append(line)
            
            all_code_parts.append(f"# --- Content from {fname} ---\n\n{''.join(file_code)}\n\n")

    full_script_content = "# --- External Imports ---\n\n"
    full_script_content += "\n".join(sorted(list(external_imports)))
    full_script_content += "\n\n"
    full_script_content += "".join(all_code_parts)
    
    print("Obfuscating Python code using Base64...")
    encoded_script = base64.b64encode(full_script_content.encode('utf-8')).decode('utf-8')
    
    loader_stub = f"""
# This file is auto-generated and obfuscated. Do not edit directly.
# The actual server code is encoded below to discourage casual modification.
import base64
exec(base64.b64decode('{encoded_script}'))
"""
    
    final_server_path = os.path.join(BUILD_DIR, FINAL_SERVER_FILE)
    print(f"Writing obfuscated server to '{final_server_path}'...")
    with open(final_server_path, "w", encoding="utf-8") as outfile:
        outfile.write(loader_stub.strip())
        
    print("✅ Python server assembled and obfuscated successfully.")
    
def copy_additional_files():
    """Копирует дополнительные файлы, такие как README.TXT."""
    print("\n[--- Copying Additional Files ---]")
    readme_filename = "README.TXT"
    src_readme = readme_filename # Файл лежит в корне проекта
    dest_readme = os.path.join(BUILD_DIR, readme_filename)
    
    if os.path.exists(src_readme):
        shutil.copy(src_readme, dest_readme)
        print(f"✅ Copied '{src_readme}' to '{dest_readme}'")
    else:
        print(f"⚠️ WARNING: '{src_readme}' not found. Skipping copy.")

def main():
    """Основная функция сборки проекта."""
    print("🚀 Starting CrossCraft Server build process...")

    if os.path.exists(BUILD_DIR):
        print(f"🧹 Cleaning old build directory: {BUILD_DIR}")
        shutil.rmtree(BUILD_DIR)
    print(f"✨ Creating build directory: {BUILD_DIR}")
    os.makedirs(BUILD_DIR)

    try:
        build_cpp_core()
        assemble_and_obfuscate_python_server()
        copy_additional_files()
        
        print("\n🎉 Build process completed successfully!")
        print(f"All files are located in the '{BUILD_DIR}' directory.")
        print(f"To run the server, navigate to '{BUILD_DIR}' and execute: python {FINAL_SERVER_FILE}")

    except (subprocess.CalledProcessError, FileNotFoundError, Exception) as e:
        print(f"❌ ERROR: Build process failed: {e}")

if __name__ == "__main__":
    main()
