CXX      ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS += -Iinclude
LDFLAGS  += -pthread

CORE_SRC   := src/json.cpp src/media.cpp src/library.cpp src/playlist.cpp src/storage.cpp \
              src/scanner.cpp src/seeder.cpp
SERVER_SRC := $(CORE_SRC) src/http.cpp src/app.cpp src/main.cpp
TEST_SRC   := $(CORE_SRC) tests/test_core.cpp

BIN := bin/playlist-builder
TEST_BIN := bin/tests

.PHONY: all run test clean report

all: $(BIN)

$(BIN): $(SERVER_SRC) $(wildcard include/*.hpp)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(SERVER_SRC) -o $@ $(LDFLAGS)

$(TEST_BIN): $(TEST_SRC) $(wildcard include/*.hpp)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TEST_SRC) -o $@ $(LDFLAGS)

run: $(BIN)
	./$(BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

report:
	@test -x report/build/venv/bin/python || python3 -m venv report/build/venv
	@report/build/venv/bin/python -c "import reportlab,PIL" 2>/dev/null || report/build/venv/bin/pip install -q reportlab pillow
	report/build/venv/bin/python report/build_report.py

clean:
	rm -rf bin
