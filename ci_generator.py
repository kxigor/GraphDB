import os
import sys
import argparse

# Настройки
TESTS_DIR = "tests"

def add_block_to_file_begin(file_path, block):
    with open(file_path, 'r') as file:
        content = file.read()

    with open(file_path, 'w') as file:
        file.write(block + content)


def find_tests(tests_dir, branch):
    """Находит все тестовые файлы вида *test.<extension>
    в поддиректориях tests/<ветка>"""
    branch_dir = os.path.join(tests_dir, branch)
    if not os.path.exists(branch_dir):
        print(f"No tests found for branch: {branch}")
        return []


    test_files = []
    for root, dirs, files in os.walk(branch_dir):
        for file in files:
            if file.split(".")[0].endswith("test"):
                relative_path = os.path.relpath(root, branch_dir)
                base_name = os.path.splitext(file)[0]
                test_name = (
                    "_".join([relative_path.replace(os.sep, "_"), base_name])
                    if relative_path != "."
                    else base_name
                )
                test_files.append((os.path.join(root, file), test_name))
    return test_files


def find_explicitly_added_tests(tests_dir, branch):
    tests = []
    with open(os.path.join(tests_dir, branch, "CMakeLists.txt")) as cmakelist:
        test_name_on_current_line = False
        for line in cmakelist:
            lower_line = line.lower()
            if test_name_on_current_line:
                if lower_line.find("name") != -1:
                    tests.append((os.path.join(tests_dir, branch, "CMakeLists.txt"), 
                                  line.split()[lower_line.split().index("name") + 1]))
                    test_name_on_current_line = False
                else:
                    continue
            if lower_line.find("add_test") != -1:
                if lower_line.find("name") != -1:
                    tests.append((os.path.join(tests_dir, branch, "CMakeLists.txt"),
                                  line.split()[lower_line.split().index("name") + 1]))
                    test_name_on_current_line = False
                else:
                    test_name_on_current_line = True
    return tests

def generate_test_function_call(test_source, test_name, branch_name):
    cur = "simple"
    with open(test_source) as test_file:
        for i, f in enumerate(test_file):
            if f.find("gtest") != -1:
                cur = "google"
                break
            if i > 30:
                break
    return f"add_{cur}_test({test_name} {test_source.split(branch_name)[1][1:]})\n"


def update_cmake_file(cmake_path, new_test_blocks):
    updated_content = ""
    for block in new_test_blocks:
        parts = block.split("(")[1].split(")")[0].split()
        print(block)
        if len(parts) > 0:
            test_name = parts[0]
            updated_content += block

    with open(
        cmake_path,
        "a" if os.path.exists(cmake_path) else "w",
        encoding="utf-8",
    ) as f:
        f.write(updated_content)

ci_template = """{name}:
  stage: test
  cache:
    key: "{pipeline_id}"
    paths:
      - build/
      - tests/
  script: "echo 'Running test: {test_name}' && ./run.sh -T {test_name} --keep-tests"

"""

def main():
    parser = argparse.ArgumentParser(description="Generate test configurations.")
    parser.add_argument("branch_name", help="The name of the branch to process.")
    parser.add_argument(
        "--generate_ci", help="Optional to generate CI", default="false"
    )
    parser.add_argument(
        "--pipeline_id", help="Optional to generate CI", default=""
    )
    parser.add_argument(
        "--artifact_job", help="Optional to generate CI", default=""
    )

    args = parser.parse_args()

    branch_name = args.branch_name
    generate_ci = args.generate_ci
    pipeline_id = args.pipeline_id
    artifact_job = args.artifact_job

    test_files = find_tests(TESTS_DIR, branch_name)

    if not test_files:
        print("No test files found.")
        return

    test_function_calls = [
        generate_test_function_call(src, name, branch_name) for src, name in test_files
    ]

    cmake_path = os.path.join(TESTS_DIR, branch_name, "CMakeLists.txt")
    update_cmake_file(cmake_path, test_function_calls)
    add_block_to_file_begin(cmake_path, "# generated\n")

    cmake_path = os.path.join(TESTS_DIR, "CMakeLists.txt")
    update_cmake_file(cmake_path, [f"add_subdirectory({branch_name})"])
    add_block_to_file_begin(cmake_path, "# generated\n")

    test_files.extend(find_explicitly_added_tests(TESTS_DIR, branch_name))

    if generate_ci == "true":
        with open("generated-ci.yml", "w") as dyn_ci:
            for _, test_name in test_files:
                dyn_ci.write(ci_template.format(name=test_name.lower(), 
                                                  test_name=test_name,
                                                  pipeline_id=pipeline_id,
                                                  artifact_job=artifact_job))
    
        with open("generated-ci.yml", "r") as dyn_ci:
            for line in dyn_ci:
                print(line)


if __name__ == "__main__":
    main()
