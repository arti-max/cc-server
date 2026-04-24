import os

# Настройки: какие расширения файлов искать
EXTENSIONS = {'.cpp', '.hpp', '.h', '.c', '.py', '.txt', '.md'}
# Названия конкретных файлов, которые тоже нужно включить (даже если расширение не в списке)
INCLUDE_FILES = {'CMakeLists.txt', 'README.txt'}
# Папки, которые нужно игнорировать (например, скомпилированные файлы, git и т.д.)
IGNORE_DIRS = {'.git', '__pycache__', 'build', 'cmake-build-debug', '.vscode', '.idea'}

def is_source_file(filename):
    """Проверяет, подходит ли файл по расширению или имени."""
    if filename in INCLUDE_FILES:
        return True
    _, ext = os.path.splitext(filename)
    return ext.lower() in EXTENSIONS

def collect_sources(start_dir, output_file):
    """Сканирует директорию и записывает содержимое файлов в output_file."""
    
    with open(output_file, 'w', encoding='utf-8') as outfile:
        # Заголовок для итогового файла
        outfile.write(f"=== PROJECT SOURCE CODE ===\n")
        outfile.write(f"Root: {os.path.abspath(start_dir)}\n\n")

        for root, dirs, files in os.walk(start_dir):
            # Удаляем игнорируемые папки из списка обхода
            dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]

            for file in files:
                if is_source_file(file):
                    file_path = os.path.join(root, file)
                    # Относительный путь для красоты в отчете
                    rel_path = os.path.relpath(file_path, start_dir)
                    
                    print(f"Adding: {rel_path}")
                    
                    try:
                        with open(file_path, 'r', encoding='utf-8', errors='ignore') as infile:
                            content = infile.read()
                            
                            # Разделитель и имя файла
                            outfile.write(f"\n{'='*60}\n")
                            outfile.write(f"FILE: {rel_path}\n")
                            outfile.write(f"{'='*60}\n\n")
                            
                            outfile.write(content)
                            outfile.write("\n") # Доп. перенос строки
                    except Exception as e:
                        print(f"Error reading {file_path}: {e}")
                        outfile.write(f"\n[ERROR READING FILE: {rel_path} - {e}]\n")

if __name__ == "__main__":
    # Текущая директория как корень проекта
    project_root = os.getcwd()
    output_filename = "full_project_code.txt"
    
    print(f"Scanning project in: {project_root}")
    collect_sources(project_root, output_filename)
    print(f"\nDone! All code collected in: {output_filename}")
