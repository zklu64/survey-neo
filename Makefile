# GNU Make solution makefile autogenerated by Premake
# Type "make help" for usage help

ifndef config
  config=debug
endif
export config

PROJECTS := framework imgui lodepng Project

.PHONY: all clean help $(PROJECTS)

all: $(PROJECTS)

framework: 
	@echo "==== Building framework ($(config)) ===="
	@${MAKE} --no-print-directory -C build -f framework.make

imgui: 
	@echo "==== Building imgui ($(config)) ===="
	@${MAKE} --no-print-directory -C build -f imgui.make

lodepng: 
	@echo "==== Building lodepng ($(config)) ===="
	@${MAKE} --no-print-directory -C build -f lodepng.make

Project: 
	@echo "==== Building Project ($(config)) ===="
	@${MAKE} --no-print-directory -C build -f Project.make

clean:
	@${MAKE} --no-print-directory -C build -f framework.make clean
	@${MAKE} --no-print-directory -C build -f imgui.make clean
	@${MAKE} --no-print-directory -C build -f lodepng.make clean
	@${MAKE} --no-print-directory -C build -f Project.make clean

help:
	@echo "Usage: make [config=name] [target]"
	@echo ""
	@echo "CONFIGURATIONS:"
	@echo "   debug"
	@echo "   release"
	@echo ""
	@echo "TARGETS:"
	@echo "   all (default)"
	@echo "   clean"
	@echo "   framework"
	@echo "   imgui"
	@echo "   lodepng"
	@echo "   Project"
	@echo ""
	@echo "For more information, see http://industriousone.com/premake/quick-start"