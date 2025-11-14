import os
import shutil
import subprocess
import platform
import re

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
    "network.py",
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

def assemble_python_server():
    print("\n[--- Assembling Python Server ---]")
    
    all_code = []
    external_imports = set()
    
    internal_modules = [os.path.splitext(f)[0] for f in PYTHON_FILES_ORDER]
    
    for fname in PYTHON_FILES_ORDER:
        fpath = os.path.join(PYTHON_SRC_DIR, fname)
        with open(fpath, "r", encoding="utf-8") as infile:
            lines = infile.readlines()
            
            file_code = []
            for line in lines:
                # Проверяем, является ли строка внутренним импортом
                is_internal_import = False
                for module_name in internal_modules:
                    if re.match(rf"from {module_name} import|import {module_name}", line.strip()):
                        is_internal_import = True
                        break
                
                if is_internal_import:
                    continue # Пропускаем внутренний импорт
                
                # Собираем внешние импорты
                if line.strip().startswith(("import ", "from ")):
                    external_imports.add(line.strip())
                else:
                    file_code.append(line)
            
            all_code.append(f"# --- Content from {fname} ---\n\n{''.join(file_code)}\n\n")

    # Сборка финального файла
    final_server_path = os.path.join(BUILD_DIR, FINAL_SERVER_FILE)
    print(f"Concatenating Python files into '{final_server_path}'...")
    
    with open(final_server_path, "w", encoding="utf-8") as outfile:
        # Сначала записываем все уникальные внешние импорты
        outfile.write("# --- External Imports ---\n\n")
        outfile.write("\n".join(sorted(list(external_imports))))
        outfile.write("\n\n")
        
        # Затем записываем весь код
        outfile.write("".join(all_code))
        
    print("✅ Python server assembled successfully.")

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
        assemble_python_server()
        
        print("\n🎉 Build process completed successfully!")
        print(f"All files are located in the '{BUILD_DIR}' directory.")
        print(f"To run the server, navigate to '{BUILD_DIR}' and execute: python {FINAL_SERVER_FILE}")

    except (subprocess.CalledProcessError, FileNotFoundError, Exception) as e:
        print(f"❌ ERROR: Build process failed: {e}")

if __name__ == "__main__":
    main()
