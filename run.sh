#!/bin/bash

CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
CURRENT_BRANCH_WITH_DASHES="${CURRENT_BRANCH//_/-}"
PROJECT_DIR=$(pwd)
STATUS=""
FETCH=false
BUILD=false
REBUILD=false
TESTING=false
SHOW_TESTS=false
SPLIT_TERMINAL=false
GENERATE_CI=false
KEEP_TESTS=false
SPECIFIC_TEST=""
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
NC='\033[0m'

usage() {
    echo -e "${BOLD}Usage: $0 [-f|--fetch] [-b|--build] [-t|--test] [--show-available-tests] [-T <test_name>] [--split-test-terminal]${NC}"
    echo ""
    echo -e "${YELLOW}-f, --fetch${NC}  ${GREEN}check local branch 'test'. If it's not up-to-date - fetch it${NC}"
    echo -e "${YELLOW}-b, --build${NC}  ${GREEN}rebuild cached project in dir build${NC}"
    echo -e "${YELLOW}--rebuild${NC}  ${GREEN}force rebuild full project into dir build${NC}"
    echo -e "${YELLOW}-t, --test${NC}   ${GREEN}start testing in the same terminal window${NC}"
    echo -e "${YELLOW}--show-available-tests${NC}  ${GREEN}no comments${NC}"
    echo -e "${YELLOW}-T <test_name>${NC}  ${GREEN}start specific test${NC}"
    echo -e "${YELLOW}--split-test-terminal${NC}  ${GREEN}show output of tests in separate terminal${NC}"
    echo -e "${YELLOW}--generate-ci${NC}  ${GREEN}generate ci child pipeline for testing in downstream${NC}"
    echo -e "${YELLOW}--keep-tests${NC}  ${GREEN}do not remove tests dir after using this script${NC}"
    exit 1
}

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -f|--fetch) FETCH=true ;;
        -b|--build) BUILD=true ;;
        -t|--test) TESTING=true ;;
        --show-available-tests) SHOW_TESTS=true ;;
        --rebuild) REBUILD=true ;;
        --generate-ci) GENERATE_CI=true ;;
        -T) SPECIFIC_TEST="$2"; shift ;;
        --split-test-terminal) SPLIT_TERMINAL=true ;;
        --keep-tests) KEEP_TESTS=true ;;
        -*) 
            shft=false;
            for (( i=1; i<${#1}; i++ )); do
                case "${1:i:1}" in
                    f) FETCH=true ;;
                    b) BUILD=true ;;
                    T) SPECIFIC_TEST="$2"; shft=true ;;
                    t) TESTING=true ;;
                    *) usage ;;
                esac
            done
            if [[ $shft == "true" ]]; then
                shift
            fi
            ;;
        *) usage ;;
    esac
    shift
done

clear_tests_branch_files() {
  rm -rf $PROJECT_DIR/tests/ > /dev/null 2>&1 \
  rm $PROJECT_DIR/ci_generator.py > /dev/null 2>&1
}


check_build_folder() {
    if [[ -d "build" && -n "$(ls -A build)" ]]; then
        echo -e "${GREEN}${BOLD}Файлы после сборки найдены в папке 'build'.${NC}"
        return 0
    else
        echo -e "${RED}${BOLD}Папка 'build' пуста или отсутствует.${NC}"
        return 1
    fi
}


check_tests_branch_up_to_date() {
    if git rev-parse --verify tests >/dev/null 2>&1; then
        local local_hash=$(git rev-parse tests)
        local remote_hash=$(git ls-remote origin refs/heads/tests 2>/dev/null | awk '{print $1}')
        
        if [[ -z "$remote_hash" ]]; then
            echo -e "${RED}${BOLD}Не удалось получить данные об удалённой ветке 'tests'.${NC}"
            STATUS="no access"
            return 1
        fi
        if [[ "$local_hash" == "$remote_hash" ]]; then
            echo -e "${GREEN}${BOLD}Локальная ветка 'tests' актуальна.${NC}"
            STATUS="up-to-date"
            return 0
        else
            echo -e "${YELLOW}${BOLD}Локальная ветка 'tests' устарела.${NC}"
            STATUS="not up-to-date"
            return 1
        fi
    else
        echo -e "${RED}${BOLD}Локальная ветка 'tests' не найдена.${NC}"
        STATUS="no branch"
        return 1
    fi
}



fetch_tests_branch() {
    if [[ $STATUS != "up-to-date" ]]; then
        git fetch -f origin tests:tests
        echo -e "${GREEN}${BOLD}Локальная ветка 'tests' обновлена.${NC}"
    fi
}


build_project() {
    if [[ "$REBUILD" == true ]]; then 
        echo -e "${YELLOW}Очистка старых файлов сборки...${NC}"
        clear_tests_branch_files
        rm -rf build
    fi
    
    if [[ ! -d "build" ]]; then
        REBUILD=true
    fi
    mkdir -p build > /dev/null 2>&1

    local build_test_flag=$(head -n 1 "tests/CMakeLists.txt" 2>&1)

    if [[ $build_test_flag != "# generated" || $REBUILD == "true" ]]; then
        echo -e "${YELLOW}Ищем и добавляем тесты в пайплайн...${NC}"
        git restore --source=tests --worktree tests/ ci_generator.py
        python3 ci_generator.py $CURRENT_BRANCH
    fi

    if [[ "$GENERATE_CI" == "true" ]]; then
        git restore --source=tests --worktree tests/ ci_generator.py
        python3 ci_generator.py $CI_COMMIT_REF_NAME --generate_ci true --pipeline_id $CI_PIPELINE_ID --artifact_job $CI_JOB_NAME
    fi

    echo -e "${YELLOW}Сборка проекта...${NC}"
    cd build || exit 1
    cmake .. && make
    if [[ $? -ne 0 ]]; then
        echo -e "${RED}${BOLD}Ошибка сборки проекта.${NC}"
        if [[ $KEEP_TESTS == "false" ]]; then
            clear_tests_branch_files
        fi
        exit 1
    fi
    
    if [[ $KEEP_TESTS == "false" ]]; then
        clear_tests_branch_files
    fi
    cd ..
}


run_tests() {
    local build_test_flag=$(head -n 1 "tests/CMakeLists.txt" 2>&1)
    if [[ $build_test_flag != "# generated" ]]; then
        git restore --source=tests --worktree tests/
        echo "restored tests"
    fi
    local test_command="ctest --test-dir build/tests/ --output-on-failure"

    if [[ -n "$SPECIFIC_TEST" ]]; then
        test_command+=" -R $SPECIFIC_TEST"
    fi
    if [[ $KEEP_TESTS == "false" ]]; then
        test_command+=" && (rm -rf $PROJECT_DIR/tests/ || rm $PROJECT_DIR/ci_generator.py > /dev/null 2>&1)"
    fi

    if [[ "$SPLIT_TERMINAL" == true ]]; then
        if [[ "$(uname)" == "Darwin" ]]; then
            echo -e "${YELLOW}Запуск тестов в новом терминале (macOS)...${NC}"
            osascript \
                -e "tell application \"Terminal\"" \
                -e "do script \"cd $(pwd); $test_command; exec bash\"" \
                -e "end tell"
        elif [[ "$(uname)" == "Linux" ]]; then
            if command -v gnome-terminal &>/dev/null; then
                echo -e "${YELLOW}Запуск тестов в новом терминале (gnome-terminal)...${NC}"
                gnome-terminal -- bash -c "$test_command; exec bash"
            elif command -v konsole &>/dev/null; then
                echo -e "${YELLOW}Запуск тестов в новом терминале (konsole)...${NC}"
                konsole --noclose -e bash -c "$test_command"
            elif command -v xterm &>/dev/null; then
                echo -e "${YELLOW}Запуск тестов в новом терминале (xterm)...${NC}"
                xterm -hold -e bash -c "$test_command"
            else
                echo -e "${RED}Графический терминал не найден. Запуск тестов в текущем терминале...${NC}"
                eval "$test_command"
            fi
        else
            echo -e "${RED}Операционная система не поддерживается. Запуск тестов в текущем терминале...${NC}"
            eval "$test_command"
        fi
    else
        echo -e "${YELLOW}Запуск тестов в текущем терминале...${NC}"
        eval "$test_command"
    fi

    if [[ $? -ne 0 ]]; then
        echo -e "${RED}${BOLD}Тесты завершились с ошибкой.${NC}"
        exit 1
    fi
}

check_tests_branch_up_to_date

if [[ "$FETCH" == true || "$GENERATE_CI" == true ]]; then
    fetch_tests_branch
fi

if [[ "$BUILD" == true ]]; then
    build_project
else
    check_build_folder
fi

if [[ "$TESTING" == true || "$SPECIFIC_TEST" != "" ]]; then
    run_tests
fi

if [[ "$SHOW_TESTS" == true ]]; then
    git restore --source=tests --worktree tests/

    if [[ -d "build" && -n "$(ls -A build)" ]]; then
        echo -e "${YELLOW}Доступные тесты:${NC}"
        cd build/tests
    else
        echo -e "${RED}${BOLD}Сначала нужно сбилдить тесты${NC}"
        exit 1
    fi

    ctest --show-only
    if [[ $KEEP_TESTS == "false" ]]; then
        clear_tests_branch_files
    fi
    cd ../..
    exit 0
fi
